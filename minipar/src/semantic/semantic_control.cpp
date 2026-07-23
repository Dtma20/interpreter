/**
 * @file semantic_control.cpp
 * @brief Implementação dos visitantes de controle de fluxo do SemanticAnalyzer.
 *
 * Contém: visit_Return(), visit_Break(), visit_Continue(),
 * visit_If(), visit_While(), visit_Par(), visit_CChannel(), visit_SChannel().
 */

#include "../../include/semantic/semantic_core.hpp"
#include "../../include/debug.hpp"
#include <stdexcept>

/**
 * @brief Analisa uma instrução de retorno.
 */
void SemanticAnalyzer::visit_Return(Return *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Return");

    FuncDef *f = nullptr;
    for (auto ctx : context_stack)
        if ((f = dynamic_cast<FuncDef *>(ctx)))
            break;
    if (!f)
        throw SemanticError(node->getLine(), "return fora de função");

    std::string retType = evaluate(node->getExpr());
    LOG_DEBUG("SemanticAnalyzer: return expr type " << retType);

    std::string declaredRaw = f->getReturnType();
    LOG_DEBUG("SemanticAnalyzer: declared return type " << declaredRaw);

    if (declaredRaw == "array")
    {
        if (retType.rfind("array<", 0) != 0)
            throw SemanticError(node->getLine(),
                                "Retorno em " + f->getName() +
                                    " deve ser um array, mas retornou " + retType);
    }
    else
    {
        std::string expectedType = normalize(declaredRaw);
        LOG_DEBUG("SemanticAnalyzer: expected normalized " << expectedType);
        if (retType != expectedType)
            throw SemanticError(node->getLine(),
                                "Retorno em " + f->getName() +
                                    " deve ser " + expectedType +
                                    ", mas retornou " + retType);
    }

    inferredReturnTypes[f->getName()] = retType;

    LOG_DEBUG("SemanticAnalyzer: visit_Return end");
}

/**
 * @brief Analisa uma instrução if.
 */
void SemanticAnalyzer::visit_If(If *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_If");
    std::string cond = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: if condition type " << cond);
    if (cond != "bool")
        throw SemanticError(node->getLine(), "Esperado bool em if, mas recebeu " + cond);
    scope_stack.emplace_back();
    visit_block(node->getBody());
    scope_stack.pop_back();
    if (node->getElseStmt())
    {
        scope_stack.emplace_back();
        visit_block(*node->getElseStmt());
        scope_stack.pop_back();
    }
    LOG_DEBUG("SemanticAnalyzer: visit_If end");
}

/**
 * @brief Analisa uma instrução while.
 */
void SemanticAnalyzer::visit_While(While *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_While");
    context_stack.push_back(node);
    std::string cond = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: while condition type " << cond);
    if (cond != "bool")
        throw SemanticError(node->getLine(), "Esperado bool em while, mas recebeu " + cond);
    scope_stack.emplace_back();
    visit_block(node->getBody());
    scope_stack.pop_back();
    context_stack.pop_back();
    LOG_DEBUG("SemanticAnalyzer: visit_While end");
}

/**
 * @brief Analisa uma instrução break.
 */
void SemanticAnalyzer::visit_Break(Break *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Break");
    bool inLoop = false;
    for (auto ctx : context_stack)
        if (dynamic_cast<While *>(ctx))
        {
            inLoop = true;
            break;
        }
    if (!inLoop)
        throw SemanticError(node->getLine(), "break fora de loop");
    LOG_DEBUG("SemanticAnalyzer: visit_Break end");
}

/**
 * @brief Analisa uma instrução continue.
 */
void SemanticAnalyzer::visit_Continue(Continue *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Continue");
    bool inLoop = false;
    for (auto ctx : context_stack)
        if (dynamic_cast<While *>(ctx))
        {
            inLoop = true;
            break;
        }
    if (!inLoop)
        throw SemanticError(node->getLine(), "continue fora de loop");
    LOG_DEBUG("SemanticAnalyzer: visit_Continue end");
}

/**
 * @brief Analisa uma execução paralela.
 */
void SemanticAnalyzer::visit_Par(Par *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Par");
    for (auto &st : node->getBody())
    {
        if (!dynamic_cast<Call *>(st.get()))
            throw SemanticError(node->getLine(), "Apenas chamadas válidas em par");
    }
    LOG_DEBUG("SemanticAnalyzer: visit_Par end");
}

/**
 * @brief Analisa uma declaração de canal de comunicação (CChannel).
 */
void SemanticAnalyzer::visit_CChannel(CChannel *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_CChannel " << node->getName());

    auto &currentScope = scope_stack.back();
    std::string name = node->getName();
    if (currentScope.find(name) != currentScope.end())
    {
        throw SemanticError(node->getLine(), "Identificador duplicado: " + name);
    }
    currentScope[name] = "CChannel";

    if (evaluate(node->getLocalhostNode()) != "string")
        throw SemanticError(node->getLine(), "localhost deve ser string em CChannel");
    if (evaluate(node->getPortNode()) != "num")
        throw SemanticError(node->getLine(), "port deve ser num em CChannel");

    if (auto lit = dynamic_cast<Constant *>(node->getLocalhostNode()))
    {
        std::string hostVal = lit->getToken().getValue();
        if (hostVal.empty())
        {
            throw SemanticError(node->getLine(), "localhost não pode ser string vazia em CChannel");
        }
    }

    if (auto lit = dynamic_cast<Constant *>(node->getPortNode()))
    {
        double portVal = std::stod(lit->getToken().getValue());
        if (portVal < 0 || portVal > 65535)
        {
            throw SemanticError(node->getLine(), "port fora do intervalo válido [0,65535] em CChannel");
        }
    }

    LOG_DEBUG("SemanticAnalyzer: visit_CChannel end");
}

/**
 * @brief Analisa uma declaração de canal de serviço (SChannel).
 */
void SemanticAnalyzer::visit_SChannel(SChannel *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_SChannel " << node->getName());
    auto it = function_table.find(node->getFuncName());
    if (it == function_table.end())
        throw SemanticError(node->getLine(), "Função não declarada em SChannel");
    if (evaluate(node->getDescription()) != "string")
        throw SemanticError(node->getLine(), "description deve ser string");
    if (evaluate(node->getLocalhostNode()) != "string")
        throw SemanticError(node->getLine(), "localhost deve ser string");
    if (evaluate(node->getPortNode()) != "num")
        throw SemanticError(node->getLine(), "port deve ser num");
    LOG_DEBUG("SemanticAnalyzer: visit_SChannel end");
}
