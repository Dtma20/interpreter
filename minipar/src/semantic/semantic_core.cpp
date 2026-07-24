/**
 * @file semantic_core.cpp
 * @brief Implementação dos métodos centrais do SemanticAnalyzer.
 *
 * Contém: construtor, visit(), generic_visit(), visit_block(),
 * evaluate(), normalize().
 */

#include "../../include/semantic/semantic_core.hpp"
#include "../../include/debug.hpp"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>

/**
 * @brief Construtor do SemanticAnalyzer.
 */
SemanticAnalyzer::SemanticAnalyzer()
{
    LOG_DEBUG("SemanticAnalyzer: Construtor chamado");
    scope_stack.emplace_back();
    LOG_DEBUG("SemanticAnalyzer: default_funcs inicializadas");
}

/**
 * @brief Normaliza uma string de tipo para sua forma canônica.
 */
std::string SemanticAnalyzer::normalize(const std::string &raw) const
{
    LOG_DEBUG("SemanticAnalyzer: Normalizando tipo " << raw);
    if (raw == "NUM" || raw == "num")
        return "num";
    if (raw == "STRING" || raw == "string")
        return "string";
    if (raw == "BOOL" || raw == "bool")
        return "bool";
    return raw;
}

/**
 * @brief Avalia o tipo de um nó AST.
 */
std::string SemanticAnalyzer::evaluate(Node *node) const
{
    if (!node)
    {
        LOG_DEBUG("SemanticAnalyzer: evaluate recebido nó nulo");
        return "";
    }
    LOG_DEBUG("SemanticAnalyzer: Evaluating node type " << typeid(*node).name());
    std::string raw;
    if (auto c = dynamic_cast<Constant *>(node))
        raw = visit_Constant(c).value_or("");
    else if (auto id = dynamic_cast<ID *>(node))
        raw = visit_ID(id).value_or("");
    else if (auto a = dynamic_cast<Access *>(node))
        raw = visit_Access(a).value_or("");
    else if (auto l = dynamic_cast<Logical *>(node))
        raw = visit_Logical(l).value_or("");
    else if (auto r = dynamic_cast<Relational *>(node))
        raw = visit_Relational(r).value_or("");
    else if (auto ar = dynamic_cast<Arithmetic *>(node))
        raw = visit_Arithmetic(ar).value_or("");
    else if (auto u = dynamic_cast<Unary *>(node))
        raw = visit_Unary(u).value_or("");
    else if (auto call = dynamic_cast<Call *>(node))
        raw = visit_Call(call).value_or("");
    else if (auto arr = dynamic_cast<Array *>(node))
        raw = visit_Array(arr).value_or("");
    if (raw.empty())
        throw SemanticError(node->getLine(),
                            std::string("Tipo não suportado para nó: ") +
                                typeid(*node).name());
    std::string norm = normalize(raw);
    LOG_DEBUG("SemanticAnalyzer: Type normalized to " << norm);
    return norm;
}

/**
 * @brief Realiza a análise semântica de um nó.
 */
void SemanticAnalyzer::visit(Node *node)
{
    if (!node)
        return;
    LOG_DEBUG("SemanticAnalyzer: visit node type " << typeid(*node).name());

    if (auto mod = dynamic_cast<Module *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Module");
        context_stack.push_back(mod);
        generic_visit(mod);
        context_stack.pop_back();
        LOG_DEBUG("SemanticAnalyzer: exit Module");
    }
    else if (auto seq = dynamic_cast<Seq *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Seq");
        if (seq->isBlock())
        {
            LOG_DEBUG("SemanticAnalyzer: Seq is block, creating new scope");
            scope_stack.emplace_back();
        }
        for (const auto &stmt : seq->getBody())
            visit(stmt.get());
        if (seq->isBlock())
        {
            LOG_DEBUG("SemanticAnalyzer: Exiting Seq block, popping scope");
            scope_stack.pop_back();
        }
        LOG_DEBUG("SemanticAnalyzer: exit Seq");
    }
    else if (auto asn = dynamic_cast<Assign *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Assign");
        visit_Assign(asn);
    }
    else if (auto fn = dynamic_cast<FuncDef *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit FuncDef " << fn->getName());
        visit_FuncDef(fn);
    }
    else if (auto ret = dynamic_cast<Return *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Return");
        visit_Return(ret);
    }
    else if (auto br = dynamic_cast<Break *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Break");
        visit_Break(br);
    }
    else if (auto cont = dynamic_cast<Continue *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Continue");
        visit_Continue(cont);
    }
    else if (auto iff = dynamic_cast<If *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit If");
        visit_If(iff);
    }
    else if (auto wh = dynamic_cast<While *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit While");
        visit_While(wh);
    }
    else if (auto p = dynamic_cast<Par *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Par");
        visit_Par(p);
    }
    else if (auto cch = dynamic_cast<CChannel *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit CChannel " << cch->getName());
        visit_CChannel(cch);
    }
    else if (auto sch = dynamic_cast<SChannel *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit SChannel " << sch->getName());
        visit_SChannel(sch);
    }
    else if (auto arrDecl = dynamic_cast<ArrayDecl *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit ArrayDecl " << arrDecl->getName());
        visit_ArrayDecl(arrDecl);
    }
    else if (auto call = dynamic_cast<Call *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Call");
        evaluate(call); // T19: force semantic check on call-as-statement
    }
    else if (auto un = dynamic_cast<Unary *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Unary");
        evaluate(un); // T19: force semantic check on unary-as-statement (++/--)
    }
    else
    {
        LOG_DEBUG("SemanticAnalyzer: generic_visit on " << typeid(*node).name());
        generic_visit(node);
    }
}

/**
 * @brief Realiza uma visita genérica aos atributos de um nó.
 */
void SemanticAnalyzer::generic_visit(Node *node)
{
    LOG_DEBUG("SemanticAnalyzer: generic_visit attributes of " << typeid(*node).name());
    for (auto *child : node->getAttributes())
        visit(child);
}

/**
 * @brief Visita um bloco de instruções.
 */
void SemanticAnalyzer::visit_block(const Body &block)
{
    LOG_DEBUG("SemanticAnalyzer: visit_block start");
    for (auto &st : block)
        visit(st.get());
    LOG_DEBUG("SemanticAnalyzer: visit_block end");
}
