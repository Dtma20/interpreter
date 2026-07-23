/**
 * @file interpreter_builtin.cpp
 * @brief Implementação das funções builtin do Interpreter.
 *
 * Contém: builtin_print(), builtin_len(), builtin_to_num(),
 * builtin_to_string(), builtin_isnum(), builtin_isalpha(),
 * builtin_exp(), builtin_randf(), builtin_randi(), builtin_input().
 */

#include "../../include/interpreter/interpreter_core.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <random>
#include "../../include/debug.hpp"

// BLD3: thread-local seeded generator (thread-safe, não-determinístico)
thread_local std::mt19937 rng([]() {
    std::random_device rd;
    return rd();
}());

ValueWrapper Interpreter::builtin_print(Call *call)
{
    std::ostringstream oss;
    for (const auto &arg : call->getArgs())
    {
        if (!arg)
        {
            LOG_DEBUG("Interpreter: Argumento nulo em print");
            throw RunTimeError("Argumento nulo em print");
        }
        ValueWrapper value = evaluate(arg.get());
        LOG_DEBUG("Interpreter: Argumento para print: " << convert_value_to_string(value));

        std::visit([&oss](auto &&val)
                   {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                oss << "[uninitialized]";
            }
            else if constexpr (std::is_same_v<T, long double>) {
                constexpr long double epsilon = 1e-9;
                std::ios_base::fmtflags originalFlags = oss.flags();
                std::streamsize originalPrecision = oss.precision();
                if (std::abs(val - std::trunc(val)) > epsilon) {
                    oss << std::fixed << std::setprecision(8) << val;
                } else {
                    oss << std::noshowpoint << val;
                }
                oss.flags(originalFlags);
                oss.precision(originalPrecision);
            }
            else if constexpr (std::is_same_v<T, bool>) {
                oss << (val ? "true" : "false");
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                oss << val;
            }
            else if constexpr (std::is_same_v<T, std::vector<ValueWrapper>>) {
                oss << "[";
                bool first = true;
                for (const auto &elem : val) {
                    if (!first) oss << ", ";
                    oss << std::fixed << std::setprecision(0) << elem;
                    first = false;
                }
                oss << "]";
            } }, value.data);
    }

    {
        std::lock_guard<std::mutex> lk(cout_mutex);
        std::cout << oss.str() << std::flush;
    }

    LOG_DEBUG("Interpreter: print concluído, retornando string vazia");
    return ValueWrapper(std::string(""));
}

ValueWrapper Interpreter::builtin_len(Call *call)
{
    if (call->getArgs().empty() || !call->getArgs()[0])
    {
        LOG_DEBUG("Interpreter: len chamado sem argumento válido");
        throw RunTimeError("len requer um argumento válido");
    }
    ValueWrapper arg = evaluate(call->getArgs()[0].get());
    if (std::holds_alternative<std::string>(arg.data))
    {
        long double length = static_cast<long double>(std::get<std::string>(arg.data).length());
        LOG_DEBUG("Interpreter: len retornando: " << length);
        return ValueWrapper(length);
    }
    else if (std::holds_alternative<std::vector<ValueWrapper>>(arg.data))
    {
        long double length = static_cast<long double>(std::get<std::vector<ValueWrapper>>(arg.data).size());
        LOG_DEBUG("Interpreter: len retornando: " << length);
        return ValueWrapper(length);
    }
    LOG_DEBUG("Interpreter: Erro, len requer string ou array");
    throw RunTimeError("len requer uma string ou array como argumento");
}

ValueWrapper Interpreter::builtin_to_num(Call *call)
{
    if (call->getArgs().empty() || !call->getArgs()[0])
    {
        LOG_DEBUG("Interpreter: to_num chamado sem argumento válido");
        throw RunTimeError("to_num requer um argumento válido");
    }
    ValueWrapper arg = evaluate(call->getArgs()[0].get());
    if (std::holds_alternative<std::string>(arg.data))
    {
        long double num = std::stod(std::get<std::string>(arg.data));
        LOG_DEBUG("Interpreter: to_num retornando: " << num);
        return ValueWrapper(num);
    }
    LOG_DEBUG("Interpreter: Erro, to_num requer string");
    throw RunTimeError("to_num requer uma string como argumento");
}

ValueWrapper Interpreter::builtin_to_string(Call *call)
{
    if (call->getArgs().empty() || !call->getArgs()[0])
    {
        LOG_DEBUG("Interpreter: to_string chamado sem argumento válido");
        throw RunTimeError("to_string requer um argumento válido");
    }
    ValueWrapper arg = evaluate(call->getArgs()[0].get());
    std::string result = convert_value_to_string(arg);
    LOG_DEBUG("Interpreter: to_string retornando: " << result);
    return ValueWrapper(result);
}

ValueWrapper Interpreter::builtin_isnum(Call *call)
{
    if (call->getArgs().empty() || !call->getArgs()[0])
    {
        LOG_DEBUG("Interpreter: isnum chamado sem argumento válido");
        throw RunTimeError("isnum requer um argumento válido");
    }
    ValueWrapper arg = evaluate(call->getArgs()[0].get());
    if (std::holds_alternative<std::string>(arg.data))
    {
        std::string str = std::get<std::string>(arg.data);
        bool result = std::all_of(str.begin(), str.end(),
            [](unsigned char c) { return std::isdigit(c); });
        LOG_DEBUG("Interpreter: isnum retornando: " << (result ? "true" : "false"));
        return ValueWrapper(result);
    }
    LOG_DEBUG("Interpreter: isnum retornando false (não é string)");
    return ValueWrapper(false);
}

ValueWrapper Interpreter::builtin_isalpha(Call *call)
{
    if (call->getArgs().empty() || !call->getArgs()[0])
    {
        LOG_DEBUG("Interpreter: isalpha chamado sem argumento válido");
        throw RunTimeError("isalpha requer um argumento válido");
    }
    ValueWrapper arg = evaluate(call->getArgs()[0].get());
    if (std::holds_alternative<std::string>(arg.data))
    {
        std::string str = std::get<std::string>(arg.data);
        bool result = std::all_of(str.begin(), str.end(),
            [](unsigned char c) { return std::isalpha(c); });
        LOG_DEBUG("Interpreter: isalpha retornando: " << (result ? "true" : "false"));
        return ValueWrapper(result);
    }
    LOG_DEBUG("Interpreter: isalpha retornando false (não é string)");
    return ValueWrapper(false);
}

ValueWrapper Interpreter::builtin_exp(Call *call)
{
    if (call->getArgs().empty() || !call->getArgs()[0])
    {
        LOG_DEBUG("Interpreter: exp chamado sem argumento válido");
        throw RunTimeError("exp requer um argumento válido");
    }
    ValueWrapper arg = evaluate(call->getArgs()[0].get());
    if (std::holds_alternative<long double>(arg.data))
    {
        long double input = std::get<long double>(arg.data);
        long double result = std::exp(input);
        LOG_DEBUG("Interpreter: exp retornando: " << result);
        return ValueWrapper(result);
    }
    else
    {
        LOG_DEBUG("Interpreter: Erro, exp requer um número");
        throw RunTimeError("exp requer um número como argumento");
    }
}

ValueWrapper Interpreter::builtin_randf(Call *call)
{
    size_t numArgs = call->getArgs().size();
    if (numArgs == 0)
    {
        std::uniform_real_distribution<long double> dist(0.0L, 1.0L);
        long double random_val = dist(rng);
        LOG_DEBUG("Interpreter: random sem argumentos retornando: " << random_val);
        return ValueWrapper(random_val);
    }
    else if (numArgs == 1)
    {
        ValueWrapper arg = evaluate(call->getArgs()[0].get());
        if (std::holds_alternative<long double>(arg.data))
        {
            long double max_val = std::get<long double>(arg.data);
            std::uniform_real_distribution<long double> dist(0.0L, max_val);
            long double random_val = dist(rng);
            LOG_DEBUG("Interpreter: random com 1 argumento retornando: " << random_val);
            return ValueWrapper(random_val);
        }
        else
        {
            LOG_DEBUG("Interpreter: Erro, random requer um número como argumento");
            throw RunTimeError("random requer um número como argumento");
        }
    }
    else if (numArgs == 2)
    {
        ValueWrapper arg1 = evaluate(call->getArgs()[0].get());
        ValueWrapper arg2 = evaluate(call->getArgs()[1].get());
        if (std::holds_alternative<long double>(arg1.data) && std::holds_alternative<long double>(arg2.data))
        {
            long double min_val = std::get<long double>(arg1.data);
            long double max_val = std::get<long double>(arg2.data);
            std::uniform_real_distribution<long double> dist(min_val, max_val);
            long double random_val = dist(rng);
            LOG_DEBUG("Interpreter: random com 2 argumentos retornando: " << random_val);
            return ValueWrapper(random_val);
        }
        else
        {
            LOG_DEBUG("Interpreter: Erro, random requer números como argumentos");
            throw RunTimeError("random requer números como argumentos");
        }
    }
    else
    {
        LOG_DEBUG("Interpreter: Erro, random aceita no máximo 2 argumentos");
        throw RunTimeError("random aceita no máximo 2 argumentos");
    }
}

ValueWrapper Interpreter::builtin_randi(Call *call)
{
    size_t numArgs = call->getArgs().size();

    if (numArgs == 0)
    {
        std::uniform_int_distribution<int> dist(0, 1);
        int random_int = dist(rng);
        LOG_DEBUG("Interpreter: randi() retornando: " << random_int);
        return ValueWrapper(static_cast<long double>(random_int));
    }
    else if (numArgs == 1)
    {
        ValueWrapper arg = evaluate(call->getArgs()[0].get());
        if (!std::holds_alternative<long double>(arg.data))
            throw RunTimeError("randi requer um número como argumento");
        int max_val = static_cast<int>(std::get<long double>(arg.data));
        if (max_val < 0)
            throw RunTimeError("randi: valor máximo deve ser ≥ 0");
        std::uniform_int_distribution<int> dist(0, max_val);
        int random_int = dist(rng);
        LOG_DEBUG("Interpreter: randi(max) retornando: " << random_int);
        return ValueWrapper(static_cast<long double>(random_int));
    }
    else if (numArgs == 2)
    {
        auto arg1 = evaluate(call->getArgs()[0].get());
        auto arg2 = evaluate(call->getArgs()[1].get());
        if (!std::holds_alternative<long double>(arg1.data) ||
            !std::holds_alternative<long double>(arg2.data))
            throw RunTimeError("randi requer números como argumentos");
        int min_val = static_cast<int>(std::get<long double>(arg1.data));
        int max_val = static_cast<int>(std::get<long double>(arg2.data));
        if (max_val < min_val)
            throw RunTimeError("randi: max < min");
        std::uniform_int_distribution<int> dist(min_val, max_val);
        int random_int = dist(rng);
        LOG_DEBUG("Interpreter: randi(min,max) retornando: " << random_int);
        return ValueWrapper(static_cast<long double>(random_int));
    }
    else
    {
        throw RunTimeError("randi aceita no máximo 2 argumentos");
    }
}

ValueWrapper Interpreter::builtin_input(Call *call)
{
    std::string prompt = "";

    if (!call->getArgs().empty() && call->getArgs()[0])
    {
        ValueWrapper arg = evaluate(call->getArgs()[0].get());
        prompt = convert_value_to_string(arg);
    }

    std::string user_input;
    {
        std::lock_guard<std::mutex> lk(cout_mutex);
        std::cout << prompt << std::flush;
        std::getline(std::cin, user_input);
    }

    LOG_DEBUG("Interpreter: input retornando: " << user_input);
    return ValueWrapper(user_input);
}
