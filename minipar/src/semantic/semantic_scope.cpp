/**
 * @file semantic_scope.cpp
 * @brief Implementação dos visitantes de escopo do SemanticAnalyzer.
 *
 * Contém: visit_Assign(), visit_ArrayDecl(), visit_FuncDef().
 */

#include "../../include/semantic/semantic_core.hpp"
#include "../../include/debug.hpp"
#include <stdexcept>

/**
 * @brief Analisa uma declaração de array.
 */
void SemanticAnalyzer::visit_ArrayDecl(ArrayDecl *node) {
    for (auto &dim : node->getDimensions()) {
        if (evaluate(dim.get()) != "num")
            throw SemanticError("Dimensão de array deve ser num");
    }
    std::string t = "unknown";
    for (size_t k = 0; k < node->getDimensions().size(); ++k) {
        t = "array<" + t + ">";
    }
    scope_stack.back()[node->getName()] = t;
}

/**
 * @brief Verifica a consistência de tipo em atribuições.
 */
void SemanticAnalyzer::visit_Assign(Assign *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Assign start");

    if (auto id = dynamic_cast<ID *>(node->getLeft()))
    {
        const std::string name = id->getToken().getValue();
        const std::string declared = id->getType();
        LOG_DEBUG("SemanticAnalyzer: Assign to ID " << name << " declared type " << declared);

        std::string rightType = evaluate(node->getRight());
        std::string leftType;

        if (!declared.empty() && declared != "ID")
        {
            if (declared == "array")
            {
                if (rightType.rfind("array<", 0) != 0)
                    throw SemanticError("Esperado tipo array, mas recebeu " + rightType);
                leftType = rightType;
            }
            else
            {
                leftType = normalize(declared);
                if (leftType != rightType)
                    throw SemanticError("Tipo " + leftType + " esperado, mas recebeu " + rightType);
            }

            if (scope_stack.back().count(name))
                throw SemanticError("Variável " + name + " já declarada");

            scope_stack.back()[name] = leftType;
            LOG_DEBUG("SemanticAnalyzer: Declared new var " << name << " of type " << leftType);
            LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
            return;
        }

        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it)
        {
            auto vit = it->find(name);
            if (vit != it->end())
            {
                leftType = vit->second;
                LOG_DEBUG("SemanticAnalyzer: Found existing var " << name << " of type " << leftType);

                if (leftType.find("array") != std::string::npos)
                {
                    vit->second = rightType;
                    LOG_DEBUG("SemanticAnalyzer: Inferred array type for " << name << ": " << rightType);
                    return;
                }

                if (leftType != rightType)
                    throw SemanticError("Tipo " + leftType + " esperado, mas recebeu " + rightType);

                LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
                return;
            }
        }

        throw SemanticError("Variável " + name + " não declarada");
    }
    else if (auto acc = dynamic_cast<Access *>(node->getLeft()))
    {
        LOG_DEBUG("SemanticAnalyzer: Assign to Access");
    
        std::string baseType = evaluate(acc->getBase());
        LOG_DEBUG("SemanticAnalyzer: base type " << baseType);
    
        std::string elemType;
        if (baseType == "string") {
            elemType = "string";
        }
        else if (baseType == "array") {
            baseType = "array<unknown>";
            elemType = "unknown";
        }
        else if (baseType.rfind("array<", 0) == 0) {
            elemType = baseType.substr(6, baseType.size() - 7);
        }
        else {
            throw SemanticError("Tipo " + baseType + " não indexável");
        }
        
        std::string valType = evaluate(node->getRight());
        LOG_DEBUG("SemanticAnalyzer: element type " << elemType
                  << " rhs type " << valType);

        if (elemType.find("unknown") != std::string::npos)
        {
            Node *root = acc;
            while (auto a = dynamic_cast<Access *>(root)) {
                root = a->getBase();
            }
            auto baseID = dynamic_cast<ID *>(root);
            if (!baseID)
                throw SemanticError("Não foi possível inferir tipo de array não‑ID");
    
            std::string arrName = baseID->getToken().getValue();
            std::string originalType;
            for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
                if (it->count(arrName)) {
                    originalType = it->at(arrName);
                    break;
                }
            }

            std::string innerValType = valType;
            if (valType.rfind("array<", 0) == 0)
                innerValType = valType.substr(6, valType.size() - 7);

            std::string newType = originalType;
            size_t pos;
            while ((pos = newType.find("unknown")) != std::string::npos) {
                newType.replace(pos, 7, innerValType);
            }

            for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
                if (it->count(arrName)) {
                    (*it)[arrName] = newType;
                    LOG_DEBUG("SemanticAnalyzer: Inferred array type for "
                              << arrName << ": " << newType);
                    return;
                }
            }
        }
    
        if (valType != elemType)
            throw SemanticError("Tipo de índice espera " +
                                elemType + ", recebeu " + valType);
    
        LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
        return;
    }
    
    throw SemanticError("Lado esquerdo inválido em atribuição");
}

/**
 * @brief Analisa uma definição de função.
 */
void SemanticAnalyzer::visit_FuncDef(FuncDef *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_FuncDef " << node->getName());

    for (auto ctx : context_stack) {
        if (dynamic_cast<If *>(ctx) ||
            dynamic_cast<While *>(ctx) ||
            dynamic_cast<Par *>(ctx))
        {
            throw SemanticError("Não pode declarar função em escopo local");
        }
    }

    std::string fname = node->getName();
    if (function_table.count(fname)) {
        throw SemanticError("Função " + fname + " já declarada");
    }
    function_table[fname] = node;

    scope_stack.emplace_back();
    LOG_DEBUG("SemanticAnalyzer: New function scope for " << fname);

    for (auto &param : node->getParams()) {
        auto normalized_type = normalize(param.second.first);
        if (param.second.first.find("array") != std::string::npos && normalized_type == "unknown") {
            throw SemanticError("Erro semântico: Tipo " + param.second.first + " esperado, mas recebeu array<unknown>");
        }
        scope_stack.back()[param.first] = normalized_type;
        LOG_DEBUG("SemanticAnalyzer: Param " << param.first << " of type " << param.second.first);
    }

    context_stack.push_back(node);
    visit_block(node->getBody());
    context_stack.pop_back();
    scope_stack.pop_back();

    LOG_DEBUG("SemanticAnalyzer: exit FuncDef " << fname);
}
