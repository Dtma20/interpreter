/**
 * @file interpreter_stmt.cpp
 * @brief Implementação do método execute_stmt() do Interpreter.
 *
 * Contém: execute_stmt() — dispatch principal para todos os tipos de statement.
 */

#include "../../include/interpreter/interpreter_core.hpp"
#include <iostream>
#include <thread>
#include <cstring>
#include <algorithm>
#include <functional>
#include "../../include/debug.hpp"

/**
 * @brief Executa um statement (comando) do programa.
 */
void Interpreter::execute_stmt(Node *stmt)
{
    if (!stmt)
    {
        LOG_DEBUG("Interpreter: Tentativa de executar statement nulo");
        throw RunTimeError("Statement nulo");
    }
    LOG_DEBUG("Interpreter: Executando stmt, tipo: " << typeid(*stmt).name());

    if (auto *unary = dynamic_cast<Unary *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando Unary como statement");
        evaluate(unary);
        return;
    }
    else if (auto *assign = dynamic_cast<Assign *>(stmt))
    {
        Expression *left = assign->getLeft();
        ValueWrapper value = evaluate(assign->getRight());
        if (!value.isInitialized())
        {
            LOG_DEBUG("Interpreter: ValueWrapper não inicializado ao atribuir");
            throw RunTimeError("ValueWrapper não inicializado ao atribuir");
        }
        if (auto *id = dynamic_cast<ID *>(left))
        {
            const std::string var_name = id->getToken().getValue();
            auto &current = scopes.back();
            auto var_it = current.variables.find(var_name);

            if (var_it != current.variables.end())
            {
                auto &dataVar = var_it->second->data;
                if (std::holds_alternative<std::vector<ValueWrapper>>(dataVar))
                {
                    auto &arr = std::get<std::vector<ValueWrapper>>(dataVar);
                    if (std::holds_alternative<std::vector<ValueWrapper>>(value.data))
                    {
                        const auto &new_elems = std::get<std::vector<ValueWrapper>>(value.data);
                        size_t copy_size = std::min(arr.size(), new_elems.size());
                        for (size_t i = 0; i < copy_size; ++i)
                            arr[i] = new_elems[i];
                    }
                    else
                    {
                        *(var_it->second) = value;
                    }
                }
                else
                {
                    *(var_it->second) = value;
                }
            }
            else
            {
                bool updated = false;
                for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
                {
                    auto vit = it->variables.find(var_name);
                    if (vit != it->variables.end())
                    {
                        auto &dataVar = vit->second->data;
                        if (std::holds_alternative<std::vector<ValueWrapper>>(dataVar))
                        {
                            auto &arr = std::get<std::vector<ValueWrapper>>(dataVar);
                            if (std::holds_alternative<std::vector<ValueWrapper>>(value.data))
                            {
                                const auto &new_elems = std::get<std::vector<ValueWrapper>>(value.data);
                                size_t copy_size = std::min(arr.size(), new_elems.size());
                                for (size_t i = 0; i < copy_size; ++i)
                                    arr[i] = new_elems[i];
                            }
                            else
                            {
                                throw RunTimeError("Tentativa de atribuir valor não-array a um array existente");
                            }
                        }
                        else
                        {
                            *(vit->second) = value;
                        }
                        updated = true;
                        break;
                    }
                }
                if (!updated)
                {
                    scopes.back().variables[var_name] = std::make_shared<ValueWrapper>(value);
                }
            }
        }
        else if (auto *access = dynamic_cast<Access *>(left))
        {
            std::vector<int> indices;
            std::vector<Expression *> chain;
            Expression *current = access;
            while (auto *acc = dynamic_cast<Access *>(current))
            {
                ValueWrapper idx_val = evaluate(acc->getIndex());
                if (!std::holds_alternative<long double>(idx_val.data))
                    throw RunTimeError("Índice deve ser um número");
                indices.push_back(static_cast<int>(std::get<long double>(idx_val.data)));
                chain.push_back(acc);
                current = acc->getBase();
            }
            const std::string base_name = dynamic_cast<ID *>(current)->getToken().getValue();
            ValueWrapper *node = nullptr;
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto vit = it->variables.find(base_name);
                if (vit != it->variables.end())
                {
                    node = vit->second.get();
                    break;
                }
            }
            if (!node)
                throw RunTimeError("Variável " + base_name + " não definida");
            for (int i = static_cast<int>(chain.size()) - 1; i >= 0; --i)
            {
                auto &arr = std::get<std::vector<ValueWrapper>>(node->data);
                int idx = indices[i];
                if (idx < 0 || idx >= static_cast<int>(arr.size()))
                    throw RunTimeError("Índice " + std::to_string(idx) + " fora do intervalo");
                if (i == 0)
                {
                    arr[idx] = value;
                }
                else
                {
                    node = &arr[idx];
                }
            }
        }
        else
        {
            throw RunTimeError("Lado esquerdo da atribuição deve ser uma variável ou um acesso a índice");
        }
    }
    else if (auto *call = dynamic_cast<Call *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando chamada de função como statement");
        evaluate(call);
    }
    else if (auto *accessStmt = dynamic_cast<Access *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando Access como statement");
        evaluate(accessStmt);
    }
    else if (auto *func_def = dynamic_cast<FuncDef *>(stmt))
    {
        functions[func_def->getName()] = func_def;
        LOG_DEBUG("Interpreter: Definindo função: " << func_def->getName());
    }
    else if (auto *ret = dynamic_cast<Return *>(stmt))
    {
        return_value = evaluate(ret->getExpr());
        return_flag = true;
        LOG_DEBUG("Interpreter: Retorno definido: " << convert_value_to_string(return_value));
    }
    else if (auto *if_stmt = dynamic_cast<If *>(stmt))
    {
        ValueWrapper cond = evaluate(if_stmt->getCondition());
        if (is_true(cond))
        {
            push_scope();
            for (auto &st : if_stmt->getBody())
            {
                execute_stmt(st.get());
                if (return_flag || break_flag || continue_flag)
                    break;
            }
            pop_scope();
        }
        else if (if_stmt->getElseStmt())
        {
            push_scope();
            for (auto &st : *if_stmt->getElseStmt())
            {
                execute_stmt(st.get());
                if (return_flag || break_flag || continue_flag)
                    break;
            }
            pop_scope();
        }
    }
    else if (auto *while_stmt = dynamic_cast<While *>(stmt))
    {
        while (true)
        {
            ValueWrapper cond = evaluate(while_stmt->getCondition());
            if (!is_true(cond))
                break;
            push_scope();
            for (auto &st : while_stmt->getBody())
            {
                execute_stmt(st.get());
                if (return_flag)
                {
                    pop_scope();
                    return;
                }
                if (break_flag)
                {
                    break_flag = false;
                    pop_scope();
                    break;
                }
                if (continue_flag)
                {
                    continue_flag = false;
                    break;
                }
            }
            pop_scope();
            if (break_flag)
            {
                break;
            }
        }
    }
    else if (auto *brk = dynamic_cast<Break *>(stmt))
    {
        break_flag = true;
    }
    else if (auto *cont = dynamic_cast<Continue *>(stmt))
    {
        continue_flag = true;
    }
    else if (auto *par = dynamic_cast<Par *>(stmt))
    {
        const auto &body = par->getBody();
        std::vector<std::thread> threads;
        threads.reserve(body.size());

        static std::mutex cout_mutex;

        for (const auto &uptr : body)
        {
            threads.emplace_back([this, ptr = uptr.get()]()
                                 {
                execute_stmt(ptr);
                std::lock_guard<std::mutex> lk(cout_mutex); });
        }

        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }
    }
    else if (auto *seq = dynamic_cast<Seq *>(stmt))
    {
        if (seq->isBlock())
            push_scope();
        for (auto &st : seq->getBody())
        {
            execute_stmt(st.get());
            if (return_flag || break_flag || continue_flag)
                break;
        }
        if (seq->isBlock())
            pop_scope();
    }
    else if (auto *cch = dynamic_cast<CChannel *>(stmt))
    {
        std::string name = cch->getName();
        ValueWrapper host = evaluate(cch->getLocalhostNode());
        ValueWrapper port = evaluate(cch->getPortNode());
        std::string h = std::get<std::string>(host.data);
        int p = static_cast<int>(std::get<long double>(port.data));

        auto channelVal = std::make_shared<CChannelValue>(h, p);
        scopes.back().variables[name] = std::make_shared<ValueWrapper>(channelVal);

        std::cout << "CChannel '" << name << "' criado com localhost: "
                  << h << ", port: " << p << std::endl;
    }
    else if (auto *sch = dynamic_cast<SChannel *>(stmt))
    {
        run_server(sch);
    }
    else if (auto *arr_decl = dynamic_cast<ArrayDecl *>(stmt))
    {
        const std::string var_name = arr_decl->getName();
        std::vector<int> dims;
        for (auto &dim_expr : arr_decl->getDimensions())
        {
            ValueWrapper dv = evaluate(dim_expr.get());
            if (!std::holds_alternative<long double>(dv.data))
                throw RunTimeError("Tamanho do array '" + var_name + "' deve ser número");
            int sz = static_cast<int>(std::get<long double>(dv.data));
            if (sz < 0)
                throw RunTimeError("Tamanho do array '" + var_name + "' não pode ser negativo");
            dims.push_back(sz);
        }
        std::function<ValueWrapper(const std::vector<int> &, size_t)> make_arr =
            [&](const std::vector<int> &d, size_t lvl) -> ValueWrapper
        {
            if (lvl == d.size())
                return ValueWrapper(0.0L);
            std::vector<ValueWrapper> vec(d[lvl]);
            for (auto &e : vec)
                e = make_arr(d, lvl + 1);
            return ValueWrapper(vec);
        };
        scopes.back().variables[var_name] = std::make_shared<ValueWrapper>(make_arr(dims, 0));
    }
    else
    {
        throw RunTimeError("Statement não suportado: " + std::string(typeid(*stmt).name()));
    }
}
