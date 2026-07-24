/**
 * @file interpreter_func.cpp
 * @brief Implementação dos métodos de execução de funções do Interpreter.
 *
 * Contém: evaluateFunctionCall() (dispatch), execute_function().
 */

#include "../../include/interpreter/interpreter_core.hpp"
#include "../../include/builtin_table.hpp"
#include <iostream>
#include <cassert>
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

    // Builtin dispatch via static hash map — O(1) lookup, single init
    using BuiltinPtr = ValueWrapper (Interpreter::*)(Call*);
    static const std::unordered_map<std::string, BuiltinPtr> builtins = {
        {"print",     &Interpreter::builtin_print},
        {"len",       &Interpreter::builtin_len},
        {"to_num",    &Interpreter::builtin_to_num},
        {"to_string", &Interpreter::builtin_to_string},
        {"isnum",     &Interpreter::builtin_isnum},
        {"isalpha",   &Interpreter::builtin_isalpha},
        {"exp",       &Interpreter::builtin_exp},
        {"randf",     &Interpreter::builtin_randf},
        {"randi",     &Interpreter::builtin_randi},
        {"input",     &Interpreter::builtin_input},
        {"send",      &Interpreter::builtin_send},
        {"close",     &Interpreter::builtin_close},
    };

    // T22: dispatch deve permanecer em sincronia com a fonte única BUILTIN_TABLE.
    [[maybe_unused]] static const bool consistent = []
    {
        assert(builtins.size() == BUILTIN_TABLE.size() &&
               "dispatch de builtins divergente de BUILTIN_TABLE (adicione/remova em ambos)");
        for (const auto &b : BUILTIN_TABLE)
            assert(builtins.count(std::string(b.name)) &&
                   "builtin em BUILTIN_TABLE sem dispatch correspondente");
        return true;
    }();

    auto bit = builtins.find(func_name);
    if (bit != builtins.end())
    {
        return (this->*(bit->second))(call);
    }

    // User-defined function
    auto it = functions.find(func_name);
    if (it != functions.end())
    {
        ValueWrapper result = execute_function(it->second, call->getArgs());
        LOG_DEBUG("Interpreter: Função definida pelo usuário " << func_name << " retornou valor");
        return result;
    }

    LOG_DEBUG("Interpreter: Erro, função não suportada: " << func_name);
    throw RunTimeError("Função não suportada: " + func_name);
}

/**
 * @brief Executa uma função definida pelo usuário.
 *
 * Parâmetros com valor default (T11): argumentos omitidos são preenchidos
 * avaliando-se a expressão default no escopo local da função, após a
 * vinculação dos argumentos fornecidos. Defaults podem referenciar
 * parâmetros anteriores já vinculados. Argumento explícito sempre prevalece.
 *
 * @param func Ponteiro para a definição da função.
 * @param args Lista de argumentos passados na chamada.
 * @return ValueWrapper com o valor de retorno da função.
 */
ValueWrapper Interpreter::execute_function(FuncDef *func, const Arguments &args)
{
    DepthGuard depth_guard(*this); // N5: RAII recursion limit check

    const auto &params = func->getParams();

    // T11: contar parâmetros obrigatórios (sem default)
    size_t minArgs = 0;
    for (const auto &p : params)
        if (!p.second.second) ++minArgs;

    if (args.size() < minArgs || args.size() > params.size())
        throw RunTimeError(
            "Número incorreto de argumentos para '" + func->getName() +
            "': esperado entre " + std::to_string(minArgs) +
            " e " + std::to_string(params.size()) +
            ", recebido " + std::to_string(args.size()));

    // T15: avaliar args explícitos no escopo do caller (antes de ScopeGuard)
    struct ArgBinding { std::shared_ptr<ValueWrapper> ptr; bool isRef; };
    std::vector<ArgBinding> evaluated_args;
    evaluated_args.reserve(args.size());

    for (size_t i = 0; i < args.size(); ++i)
    {
        Expression *argExpr = args[i].get();
        // Pass-by-ref para arrays: se o arg é ID cujo valor é array, partilha shared_ptr
        if (auto *id = dynamic_cast<ID *>(argExpr))
        {
            const std::string key = id->getToken().getValue();
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto vit = it->variables.find(key);
                if (vit != it->variables.end())
                {
                    if (std::holds_alternative<std::vector<ValueWrapper>>(vit->second->data))
                    {
                        evaluated_args.push_back({vit->second, true});
                        goto next_arg;
                    }
                    break;
                }
            }
        }
        // Fallback: avalia por valor
        evaluated_args.push_back({std::make_shared<ValueWrapper>(evaluate(argExpr)), false});
        next_arg:;
    }

    // T15: lexical scoping — salva base atual, define função como novo frame
    size_t saved_base = scope_base;
    scope_base = scopes.size();

    ScopeGuard scope_guard(*this); // T14: RAII scope cleanup (push function scope)

    for (size_t i = 0; i < params.size(); ++i)
    {
        const std::string &name = params[i].first;

        if (i < evaluated_args.size()) {
            // Argumento explícito — vincula (por ref ou cópia)
            scopes.back().variables[name] = evaluated_args[i].ptr;
        } else {
            // T11: argumento omitido — avaliar default no escopo da função
            Expression *defaultExpr = params[i].second.second.get();
            if (!defaultExpr)
                throw RunTimeError(
                    "Parâmetro '" + name + "' de '" + func->getName() +
                    "' não possui valor default");
            auto defaultVal = evaluate(defaultExpr);
            scopes.back().variables[name] = std::make_shared<ValueWrapper>(std::move(defaultVal));
        }
    }

    for (auto &stmt : func->getBody())
    {
        execute_stmt(stmt.get());
        if (return_flag)
        {
            ValueWrapper result = std::move(return_value);
            return_flag = false;
            scope_base = saved_base;
            return result;
        }
    }

    scope_base = saved_base;
    return ValueWrapper(std::string(""));
}
