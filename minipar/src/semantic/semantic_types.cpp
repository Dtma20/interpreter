/**
 * @file semantic_types.cpp
 * @brief Implementação dos visitantes de tipo do SemanticAnalyzer.
 *
 * Contém: visit_Constant(), visit_ID(), visit_Access(), visit_Logical(),
 * visit_Relational(), visit_Arithmetic(), visit_Unary(), visit_Call(),
 * visit_Array().
 */

#include "../../include/semantic/semantic_core.hpp"
#include "../../include/debug.hpp"
#include <stdexcept>
#include <algorithm>

static const std::unordered_map<std::string, std::string> builtin_return = {
    {"print",     "string"},
    {"len",       "num"},
    {"to_num",    "num"},
    {"to_string", "string"},
    {"isnum",     "bool"},
    {"isalpha",   "bool"},
    {"exp",       "num"},
    {"randf",     "num"},
    {"randi",     "num"},
    {"input",     "string"},
    {"send",     "string"},
    {"close",     "void"},
};

/**
 * @brief Analisa uma constante.
 */
std::optional<std::string> SemanticAnalyzer::visit_Constant(const Constant *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Constant value " << node->getType());
    return normalize(node->getType());
}

/**
 * @brief Analisa um identificador.
 */
std::optional<std::string> SemanticAnalyzer::visit_ID(const ID *node) const
{
    std::string name = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_ID " << name);
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it)
    {
        if (it->count(name))
        {
            LOG_DEBUG("SemanticAnalyzer: ID type " << it->at(name));
            return it->at(name);
        }
    }
    throw SemanticError("ID não declarado: " + name);
}

/**
 * @brief Analisa um acesso a um elemento.
 */
std::optional<std::string> SemanticAnalyzer::visit_Access(const Access *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Access");

    std::string base = evaluate(node->getBase());
    LOG_DEBUG("SemanticAnalyzer: base type '" << base << "'");

    if (base == "string")
        return "string";

    if (base == "array") {
        LOG_DEBUG("SemanticAnalyzer: treating raw 'array' as 'array<num>'");
        return "num";
    }

    if (base.rfind("array<", 0) == 0) {
        return base.substr(6, base.size() - 7);
    }

    throw SemanticError("Tipo não indexável: " + base);
}

/**
 * @brief Analisa uma operação lógica.
 */
std::optional<std::string> SemanticAnalyzer::visit_Logical(const Logical *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Logical " << node->getToken().getValue());
    if (evaluate(node->getLeft()) != "bool" || evaluate(node->getRight()) != "bool")
        throw SemanticError("Operandos lógicos devem ser bool");
    return "bool";
}

/**
 * @brief Analisa uma operação relacional.
 */
std::optional<std::string> SemanticAnalyzer::visit_Relational(const Relational *node) const
{
    auto op = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_Relational op " << op);
    auto lt = evaluate(node->getLeft()), rt = evaluate(node->getRight());
    if ((op == "==" || op == "!=") && lt != rt)
        throw SemanticError("Comparação exige tipos iguais");
    if (op != "==" && op != "!=" && (lt != "num" || rt != "num"))
        throw SemanticError("Operadores relacionais numéricos exigem num");
    return "bool";
}

/**
 * @brief Analisa uma operação aritmética.
 */
std::optional<std::string> SemanticAnalyzer::visit_Arithmetic(const Arithmetic *node) const
{
    auto op = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_Arithmetic op " << op);
    std::string lt = evaluate(node->getLeft());
    std::string rt = evaluate(node->getRight());
    LOG_DEBUG("SemanticAnalyzer: operand types " << lt << " and " << rt);

    auto normalizeUnknown = [&](const std::string &t)
    {
        return t == "unknown" ? std::string("num") : t;
    };

    if (op == "+")
    {
        std::string l0 = normalizeUnknown(lt);
        std::string r0 = normalizeUnknown(rt);
        if (l0 != r0)
        {
            throw SemanticError("(Erro de Tipo) Operação '+' exige operandos do mesmo tipo, mas encontrou " + l0 + " e " + r0);
        }
        LOG_DEBUG("SemanticAnalyzer: '+' result type " << l0);
        return l0;
    }
    std::string l0 = normalizeUnknown(lt);
    std::string r0 = normalizeUnknown(rt);
    if (l0 != "num" || r0 != "num")
    {
        throw SemanticError("(Erro de Tipo) Operadores aritméticos exigem num, mas encontrou " + lt + " e " + rt);
    }
    LOG_DEBUG("SemanticAnalyzer: arithmetic result type num");
    return "num";
}

/**
 * @brief Analisa uma operação unária.
 */
std::optional<std::string> SemanticAnalyzer::visit_Unary(const Unary *node) const
{
    auto t = node->getToken().getTag();
    LOG_DEBUG("SemanticAnalyzer: visit_Unary op " << t);
    auto et = evaluate(node->getExpr());
    if (t == "-" && et != "num")
        throw SemanticError("Unário - exige num");
    if (t == "!" && et != "bool")
        throw SemanticError("Unário ! exige bool");
    return et;
}

/**
 * @brief Analisa uma chamada de função.
 */
std::optional<std::string> SemanticAnalyzer::visit_Call(const Call *node) const
{
    std::string fname = node->getOper().empty() ? node->getToken().getValue() : node->getOper();
    LOG_DEBUG("SemanticAnalyzer: visit_Call " << fname);

    if (inferredReturnTypes.count(fname)) {
        LOG_DEBUG("SemanticAnalyzer: Returning inferred type " << inferredReturnTypes.at(fname));
        return inferredReturnTypes.at(fname);
    }

    if (builtin_return.count(fname))
        return builtin_return.at(fname);

    if (!function_table.count(fname))
        throw SemanticError("Função não declarada: " + fname);
    auto f = function_table.at(fname);

    auto &params = f->getParams();
    if (node->getArgs().size() > params.size())
        throw SemanticError("Número excessivo de args em " + fname);

    size_t minParams = 0;
    for (auto &p : params)
        if (!p.second.second)
            ++minParams;
    if (node->getArgs().size() < minParams)
        throw SemanticError("Número insuficiente de args em " + fname);

    for (size_t i = 0; i < node->getArgs().size(); ++i) {
        std::string declaredRaw = params[i].second.first;
        std::string actual     = evaluate(node->getArgs()[i].get());

        if (declaredRaw == "array") {
            if (actual.rfind("array<", 0) != 0)
                throw SemanticError(
                    "Argumento " + std::to_string(i + 1) + " de " + fname +
                    " deve ser um array, mas recebeu " + actual
                );
        } else {
            std::string expected = normalize(declaredRaw);
            if (actual != expected)
                throw SemanticError(
                    "Argumento " + std::to_string(i + 1) + " de " + fname +
                    " deve ser " + expected + ", mas recebeu " + actual
                );
        }
    }

    if (inferredReturnTypes.count(fname))
        return inferredReturnTypes.at(fname);

    return normalize(f->getReturnType());
}

/**
 * @brief Analisa um array.
 */
std::optional<std::string> SemanticAnalyzer::visit_Array(const Array *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Array");
    std::string elemType;
    for (auto &el : node->getElements())
    {
        auto t = evaluate(el.get());
        LOG_DEBUG("SemanticAnalyzer: element type " << t);
        if (elemType.empty())
            elemType = t;
        else if (t != elemType)
            throw SemanticError("Elementos de array precisam ter mesmo tipo");
    }
    return "array<" + elemType + ">";
}
