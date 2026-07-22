/**
 * @file interpreter_func.cpp
 * @brief Implementação dos métodos de execução de funções do Interpreter.
 *
 * Contém: evaluateFunctionCall() (dispatch), execute_function().
 */

#include "../../include/interpreter/interpreter_core.hpp"
#include <iostream>
#include "../../include/debug.hpp"

/**
 * @brief Avalia uma chamada de função e retorna um ValueWrapper.
 *
 * Faz o dispatch entre builtins e funções definidas pelo usuário.
 */
ValueWrapper Interpreter::evaluateFunctionCall(Call *call)
{
    if (!call->getBase())
    {
        LOG_DEBUG("Interpreter: Chamada de função com identificador nulo");
        throw RunTimeError("Chamada de função com identificador nulo");
    }

    std::string func_name = call->getBase()->getToken().getValue();
    LOG_DEBUG("Interpreter: Avaliando chamada de função: " << func_name);

    // Builtin functions
    if (func_name == "print")       return builtin_print(call);
    if (func_name == "len")         return builtin_len(call);
    if (func_name == "to_num")      return builtin_to_num(call);
    if (func_name == "to_string")   return builtin_to_string(call);
    if (func_name == "isnum")       return builtin_isnum(call);
    if (func_name == "isalpha")     return builtin_isalpha(call);
    if (func_name == "exp")         return builtin_exp(call);
    if (func_name == "randf")       return builtin_randf(call);
    if (func_name == "randi")       return builtin_randi(call);
    if (func_name == "input")       return builtin_input(call);
    if (func_name == "send")        return builtin_send(call);
    if (func_name == "close")       return builtin_close(call);

    // User-defined function
    if (functions.find(func_name) != functions.end())
    {
        ValueWrapper result = execute_function(functions[func_name], call->getArgs());
        LOG_DEBUG("Interpreter: Função definida pelo usuário " << func_name << " retornou valor");
        return result;
    }

    LOG_DEBUG("Interpreter: Erro, função não suportada: " << func_name);
    throw RunTimeError("Função não suportada: " + func_name);
}

/**
 * @brief Executa uma função definida pelo usuário.
 *
 * @param func Ponteiro para a definição da função.
 * @param args Lista de argumentos passados na chamada.
 * @return ValueWrapper com o valor de retorno da função.
 */
ValueWrapper Interpreter::execute_function(FuncDef *func, const Arguments &args)
{
    push_scope();

    const auto &params = func->getParams();
    if (args.size() != params.size())
        throw RunTimeError(
            "Número incorreto de argumentos para '" + func->getName() +
            "': esperado " + std::to_string(params.size()) +
            ", recebido " + std::to_string(args.size()));

    for (size_t i = 0; i < params.size(); ++i)
    {
        const std::string &name = params[i].first;
        Expression *argExpr = args[i].get();
        bool boundByRef = false;

        if (auto *id = dynamic_cast<ID *>(argExpr))
        {
            const std::string key = id->getToken().getValue();
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto vit = it->variables.find(key);
                if (vit != it->variables.end())
                {
                    auto &dataVar = vit->second->data;
                    if (std::holds_alternative<std::vector<ValueWrapper>>(dataVar))
                    {
                        scopes.back().variables[name] = vit->second;
                        boundByRef = true;
                        break;
                    }
                }
            }
        }

        if (!boundByRef)
        {
            auto tmp = evaluate(argExpr);
            scopes.back().variables[name] = std::make_shared<ValueWrapper>(std::move(tmp));
        }
    }

    for (auto &stmt : func->getBody())
    {
        execute_stmt(stmt.get());
        if (return_flag)
        {
            ValueWrapper result = std::move(return_value);
            return_flag = false;
            pop_scope();
            return result;
        }
    }

    pop_scope();
    return ValueWrapper(std::string(""));
}
