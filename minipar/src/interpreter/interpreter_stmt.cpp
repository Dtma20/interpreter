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
#include <exception>
#include <mutex>
#include <vector>
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
                        arr.resize(new_elems.size());
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
                                arr.resize(new_elems.size());
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
            // T04: Reusa lvalue resolver para atribuicao indexada
            ValueWrapper *lvalue = resolveAccessLvalue(access);
            if (!lvalue)
                throw RunTimeError("Lado esquerdo da atribuição deve ser uma variável indexada existente");
            *lvalue = value;
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
            ScopeGuard scope_guard(*this);
            for (auto &st : if_stmt->getBody())
            {
                execute_stmt(st.get());
                if (return_flag || break_flag || continue_flag)
                    break;
            }
        }
        else if (if_stmt->getElseStmt())
        {
            ScopeGuard scope_guard(*this);
            for (auto &st : *if_stmt->getElseStmt())
            {
                execute_stmt(st.get());
                if (return_flag || break_flag || continue_flag)
                    break;
            }
        }
    }
    else if (auto *while_stmt = dynamic_cast<While *>(stmt))
    {
        while (true)
        {
            ValueWrapper cond = evaluate(while_stmt->getCondition());
            if (!is_true(cond))
                break;
            ScopeGuard scope_guard(*this);
            for (auto &st : while_stmt->getBody())
            {
                execute_stmt(st.get());
                if (return_flag)
                    return;
                if (break_flag)
                {
                    break_flag = false;
                    break;
                }
                if (continue_flag)
                {
                    continue_flag = false;
                    break;
                }
            }
            if (break_flag)
                break;
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
        // C01: cada braço corre num Interpreter isolado (snapshot de scopes).
        // C02: exceções na thread são capturadas e re-lançadas após join.
        const auto &body = par->getBody();
        std::vector<std::thread> threads;
        threads.reserve(body.size());

        std::mutex exception_mutex;
        std::vector<std::exception_ptr> exceptions;

        for (const auto &uptr : body)
        {
            threads.emplace_back([this, ptr = uptr.get(), &exception_mutex, &exceptions]() {
                try
                {
                    run_isolated_par_arm(ptr);
                }
                catch (const RunTimeError &)
                {
                    std::lock_guard<std::mutex> lk(exception_mutex);
                    exceptions.push_back(std::current_exception());
                }
                catch (const std::exception &e)
                {
                    std::lock_guard<std::mutex> lk(exception_mutex);
                    exceptions.push_back(std::make_exception_ptr(
                        RunTimeError(std::string("Erro em thread par: ") + e.what())));
                }
                catch (...)
                {
                    std::lock_guard<std::mutex> lk(exception_mutex);
                    exceptions.push_back(std::make_exception_ptr(
                        RunTimeError("Erro desconhecido em thread par")));
                }
            });
        }

        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }

        if (!exceptions.empty())
            std::rethrow_exception(exceptions.front());
    }
    else if (auto *seq = dynamic_cast<Seq *>(stmt))
    {
        std::unique_ptr<ScopeGuard> block_scope;
        if (seq->isBlock())
            block_scope = std::make_unique<ScopeGuard>(*this);
        for (auto &st : seq->getBody())
        {
            execute_stmt(st.get());
            if (return_flag || break_flag || continue_flag)
                break;
        }
    }
    else if (auto *cch = dynamic_cast<CChannel *>(stmt))
    {
        std::string name = cch->getName();
        ValueWrapper host = evaluate(cch->getLocalhostNode());
        ValueWrapper port = evaluate(cch->getPortNode());

        auto *h_str = std::get_if<std::string>(&host.data);
        if (!h_str)
            throw RunTimeError("CChannel '" + name + "': localhost deve ser string");
        auto *p_num = std::get_if<long double>(&port.data);
        if (!p_num)
            throw RunTimeError("CChannel '" + name + "': porta deve ser número");
        uint16_t p = to_port(*p_num, ("Porta de CChannel '" + name + "'").c_str());

        auto channelVal = std::make_shared<CChannelValue>(*h_str, p);
        scopes.back().variables[name] = std::make_shared<ValueWrapper>(channelVal);

        std::cout << "CChannel '" << name << "' criado com localhost: "
                  << *h_str << ", port: " << p << std::endl;
    }
    else if (auto *sch = dynamic_cast<SChannel *>(stmt))
    {
        run_server(sch);
    }
    else if (auto *arr_decl = dynamic_cast<ArrayDecl *>(stmt))
    {
        const std::string var_name = arr_decl->getName();
        std::vector<size_t> dims;
        for (auto &dim_expr : arr_decl->getDimensions())
        {
            ValueWrapper dv = evaluate(dim_expr.get());
            if (!std::holds_alternative<long double>(dv.data))
                throw RunTimeError("Tamanho do array '" + var_name + "' deve ser número");
            dims.push_back(to_index(std::get<long double>(dv.data),
                                    ("Dimensão de '" + var_name + "'").c_str()));
        }
        std::function<ValueWrapper(const std::vector<size_t> &, size_t)> make_arr =
            [&](const std::vector<size_t> &d, size_t lvl) -> ValueWrapper
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
