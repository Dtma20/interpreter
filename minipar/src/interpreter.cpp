/**
 * @file interpreter.cpp
 * @brief Implementação do interpretador para a linguagem Minipar.
 *
 * Este arquivo contém a implementação das funções responsáveis pela avaliação de expressões,
 * execução de statements e gerenciamento do ambiente de execução (escopos, funções, canais, etc.).
 */

#include "../include/interpreter.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "../include/debug.hpp"


std::string unescape_string(const std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '\\' && i + 1 < input.length()) {
            ++i;
            switch (input[i]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += input[i]; break;
            }
        } else {
            result += input[i];
        }
    }
    return result;
}

/**
 * @brief Construtor do interpretador.
 *
 * Inicializa as flags de controle (break, continue, return) e empilha o escopo global.
 */
Interpreter::Interpreter() : break_flag(false), continue_flag(false), return_flag(false), return_value()
{
    LOG_DEBUG("Interpreter: Construtor chamado, inicializando flags e escopo");
    push_scope();
}

/**
 * @brief Adiciona um novo escopo ao ambiente.
 *
 * Utilizado para gerenciar variáveis locais e contextos de execução.
 */
void Interpreter::push_scope()
{
    LOG_DEBUG("Interpreter: Empilhando novo escopo, tamanho atual: " << scopes.size());
    scopes.push_back(Scope{});
}

/**
 * @brief Remove o escopo mais recente do ambiente.
 *
 * Garante que a remoção só seja realizada se houver escopos na pilha.
 */
void Interpreter::pop_scope()
{
    if (!scopes.empty())
    {
        LOG_DEBUG("Interpreter: Desempilhando escopo, tamanho antes: " << scopes.size());
        scopes.pop_back();
        LOG_DEBUG("Interpreter: Escopo desempilhado, tamanho agora: " << scopes.size());
    }
}

/**
 * @brief Verifica o "valor de verdade" de um ValueWrapper.
 *
 * @param value Valor a ser testado.
 * @return true se o valor for considerado verdadeiro; false caso contrário.
 *
 * O comportamento varia conforme o tipo:
 * - bool: retorna o valor.
 * - double: zero é considerado falso.
 * - string: string vazia é falsa.
 */
bool Interpreter::is_true(const ValueWrapper &value)
{
    LOG_DEBUG("Interpreter: Verificando verdade em valor");
    if (std::holds_alternative<bool>(value.data))
    {
        bool result = std::get<bool>(value.data);
        LOG_DEBUG("Interpreter: Valor é bool, resultado: " << (result ? "true" : "false"));
        return result;
    }
    else if (std::holds_alternative<double>(value.data))
    {
        double num = std::get<double>(value.data);
        bool result = num != 0.0;
        LOG_DEBUG("Interpreter: Valor é double (" << num << "), resultado: " << (result ? "true" : "false"));
        return result;
    }
    else if (std::holds_alternative<std::string>(value.data))
    {
        bool result = !std::get<std::string>(value.data).empty();
        LOG_DEBUG("Interpreter: Valor é string, vazia: " << (result ? "não" : "sim")
                                                         << ", resultado: " << (result ? "true" : "false"));
        return result;
    }
    LOG_DEBUG("Interpreter: Valor desconhecido, retornando false por padrão");
    return false;
}

/**
 * @brief Avalia uma expressão e retorna seu valor.
 *
 * @param expr Ponteiro para a expressão a ser avaliada.
 * @return ValueWrapper com o resultado da avaliação.
 *
 * São tratados diversos tipos de expressões, como constantes, identificadores,
 * arrays, acessos a elementos, chamadas de função, operações aritméticas, lógicas, etc.
 *
 * Em caso de erro (ex.: expressão nula, variável não definida), lança uma exceção RunTimeError.
 */

ValueWrapper Interpreter::evaluate(Expression *expr)
{
    if (!expr)
    {
        LOG_DEBUG("Interpreter: Tentativa de avaliar expressão nula");
        throw RunTimeError("Tentativa de avaliar expressão nula");
    }

    LOG_DEBUG("Interpreter: Avaliando expressão, tipo: " << typeid(*expr).name());

    if (auto *constant = dynamic_cast<Constant *>(expr))
    {
        return evaluateConstant(constant);
    }
    else if (auto *id = dynamic_cast<ID *>(expr))
    {
        return evaluateID(id);
    }
    else if (auto *array = dynamic_cast<Array *>(expr))
    {
        return evaluateArray(array);
    }
    else if (auto *access = dynamic_cast<Access *>(expr))
    {
        return evaluateAccess(access);
    }
    else if (auto *call = dynamic_cast<Call *>(expr))
    {
        return evaluateFunctionCall(call);
    }
    else if (auto *relational = dynamic_cast<Relational *>(expr))
    {
        return evaluateRelational(relational);
    }
    else if (auto *arithmetic = dynamic_cast<Arithmetic *>(expr))
    {
        return evaluateArithmetic(arithmetic);
    }
    else if (auto *unary = dynamic_cast<Unary *>(expr))
    {
        return evaluateUnary(unary);
    }
    else if (auto *logical = dynamic_cast<Logical *>(expr))
    {
        return evaluateLogical(logical);
    }

    LOG_DEBUG("Interpreter: Erro, tipo de expressão não suportado");
    throw RunTimeError("Expressão não suportada");
}

ValueWrapper Interpreter::evaluateID(ID *id)
{
    std::string var_name = id->getToken().getValue();
    LOG_DEBUG("Interpreter: Avaliando ID: " << var_name);
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        auto var_it = it->variables.find(var_name);
        if (var_it != it->variables.end())
        {
            LOG_DEBUG("Interpreter: Variável " << var_name << " encontrada no escopo atual");
            return var_it->second;
        }
    }
    LOG_DEBUG("Interpreter: Erro, variável não definida: " << var_name);
    throw RunTimeError("Variável não definida: " + var_name);
}

ValueWrapper Interpreter::evaluateConstant(Constant *constant)
{
    std::string valueStr = constant->getToken().getValue();
    std::string typeStr = constant->getType();
    LOG_DEBUG("Interpreter: Constante detectada, tipo: " << typeStr << ", valor: " << valueStr);

    if (typeStr == "NUM")
    {
        double num = std::stod(valueStr);
        LOG_DEBUG("Interpreter: Convertendo num para double: " << num);
        return ValueWrapper(num);
    }
    else if (typeStr == "STRING")
    {
        std::string unescapedValue = unescape_string(valueStr);
        LOG_DEBUG("Interpreter: Retornando STRING: " << unescapedValue);
        return ValueWrapper(unescapedValue);
    }
    else if (typeStr == "BOOL")
    {
        bool result = valueStr == "true";
        LOG_DEBUG("Interpreter: Convertendo BOOL: " << result);
        return ValueWrapper(result);
    }

    LOG_DEBUG("Interpreter: Erro, tipo de constante não suportado: " << typeStr);
    throw RunTimeError("Tipo de constante não suportado: " + typeStr);
}
ValueWrapper Interpreter::evaluateArray(Array *array)
{
    std::vector<ValueWrapper> elements;
    for (const auto &elem : array->getElements())
    {
        if (!elem)
        {
            LOG_DEBUG("Interpreter: Elemento nulo no array");
            throw RunTimeError("Elemento nulo no array");
        }
        ValueWrapper elem_value = evaluate(elem.get());
        if (!elem_value.isInitialized())
        {
            LOG_DEBUG("Interpreter: Elemento do array não inicializado");
            throw RunTimeError("Elemento do array não inicializado");
        }
        LOG_DEBUG("Interpreter: Elemento avaliado: " << convert_value_to_string(elem_value));
        elements.push_back(elem_value);
    }
    LOG_DEBUG("Interpreter: Array avaliado com " << elements.size() << " elementos");
    return ValueWrapper(elements);
}

ValueWrapper Interpreter::evaluateAccess(Access *access)
{
    if (!access->getId() || !access->getExpr())
    {
        LOG_DEBUG("Interpreter: Acesso com base ou índice nulo");
        throw RunTimeError("Acesso com base ou índice nulo");
    }
    ValueWrapper base_val = evaluate(access->getId());
    ValueWrapper index_val = evaluate(access->getExpr());

    // Acesso a array por índice numérico
    if (std::holds_alternative<std::vector<ValueWrapper>>(base_val.data) &&
        std::holds_alternative<double>(index_val.data))
    {
        int index = static_cast<int>(std::get<double>(index_val.data));
        auto &arr = std::get<std::vector<ValueWrapper>>(base_val.data);
        if (index >= 0 && index < static_cast<int>(arr.size()))
        {
            LOG_DEBUG("Interpreter: Acesso a array no índice " << index);
            return arr[index];
        }
        LOG_DEBUG("Interpreter: Erro, índice fora do intervalo: " << index);
        throw RunTimeError("Índice " + std::to_string(index) + " fora do intervalo");
    }
    // Acesso a string por índice numérico
    else if (std::holds_alternative<std::string>(base_val.data) &&
             std::holds_alternative<double>(index_val.data))
    {
        std::string str = std::get<std::string>(base_val.data);
        int index = static_cast<int>(std::get<double>(index_val.data));
        LOG_DEBUG("Interpreter: Acesso a string '" << str << "' no índice: " << index);
        if (index >= 0 && index < static_cast<int>(str.length()))
        {
            std::string result(1, str[index]);
            LOG_DEBUG("Interpreter: Resultado do acesso: " << result);
            return ValueWrapper(result);
        }
        LOG_DEBUG("Interpreter: Erro, índice fora do intervalo: " << index);
        throw RunTimeError("Índice fora do intervalo: " + std::to_string(index));
    }
    LOG_DEBUG("Interpreter: Erro, tipo inválido para acesso");
    throw RunTimeError("Tipo inválido para acesso");
}

ValueWrapper Interpreter::evaluateFunctionCall(Call *call)
{
    if (!call->getId())
    {
        LOG_DEBUG("Interpreter: Chamada de função com identificador nulo");
        throw RunTimeError("Chamada de função com identificador nulo");
    }

    std::string func_name = call->getId()->getToken().getValue();
    LOG_DEBUG("Interpreter: Avaliando chamada de função: " << func_name);
    // funções internas
    if (func_name == "print")
    {
        for (const auto &arg : call->getArgs())
        {
            if (!arg)
            {
                LOG_DEBUG("Interpreter: Argumento nulo em print");
                throw RunTimeError("Argumento nulo em print");
            }
            ValueWrapper value = evaluate(arg.get());
            LOG_DEBUG("Interpreter: Argumento para print: " << convert_value_to_string(value));
            std::visit([](auto &&val)
                       {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    std::cout << "[uninitialized]" << std::endl;
                }
                else if constexpr (std::is_same_v<T, double>) {
                    std::cout << val << std::endl;
                }
                else if constexpr (std::is_same_v<T, bool>) {
                    std::cout << (val ? "true" : "false") << std::endl;
                }
                else if constexpr (std::is_same_v<T, std::string>) {
                    std::cout << val << std::endl;
                }
                else if constexpr (std::is_same_v<T, std::vector<ValueWrapper>>) {
                    std::cout << "[";
                    bool first = true;
                    for (const auto &elem : val) {
                        if (!first) std::cout << ", ";
                        std::cout << elem;
                        first = false;
                    }
                    std::cout << "]" << std::endl;
                } }, value.data);
        }
        LOG_DEBUG("Interpreter: print concluído, retornando string vazia");
        return ValueWrapper(std::string(""));
    }
    else if (func_name == "len")
    {
        if (call->getArgs().empty() || !call->getArgs()[0])
        {
            LOG_DEBUG("Interpreter: len chamado sem argumento válido");
            throw RunTimeError("len requer um argumento válido");
        }
        ValueWrapper arg = evaluate(call->getArgs()[0].get());
        if (std::holds_alternative<std::string>(arg.data))
        {
            double length = static_cast<double>(std::get<std::string>(arg.data).length());
            LOG_DEBUG("Interpreter: len retornando: " << length);
            return ValueWrapper(length);
        }
        else if (std::holds_alternative<std::vector<ValueWrapper>>(arg.data))
        {
            double length = static_cast<double>(std::get<std::vector<ValueWrapper>>(arg.data).size());
            LOG_DEBUG("Interpreter: len retornando: " << length);
            return ValueWrapper(length);
        }
        LOG_DEBUG("Interpreter: Erro, len requer string ou array");
        throw RunTimeError("len requer uma string ou array como argumento");
    }
    else if (func_name == "to_num")
    {
        if (call->getArgs().empty() || !call->getArgs()[0])
        {
            LOG_DEBUG("Interpreter: to_num chamado sem argumento válido");
            throw RunTimeError("to_num requer um argumento válido");
        }
        ValueWrapper arg = evaluate(call->getArgs()[0].get());
        if (std::holds_alternative<std::string>(arg.data))
        {
            double num = std::stod(std::get<std::string>(arg.data));
            LOG_DEBUG("Interpreter: to_num retornando: " << num);
            return ValueWrapper(num);
        }
        LOG_DEBUG("Interpreter: Erro, to_num requer string");
        throw RunTimeError("to_num requer uma string como argumento");
    }
    else if (func_name == "to_string")
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
    else if (func_name == "isnum")
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
            bool result = std::all_of(str.begin(), str.end(), ::isdigit);
            LOG_DEBUG("Interpreter: isnum retornando: " << (result ? "true" : "false"));
            return ValueWrapper(result);
        }
        LOG_DEBUG("Interpreter: isnum retornando false (não é string)");
        return ValueWrapper(false);
    }
    else if (func_name == "isalpha")
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
            bool result = std::all_of(str.begin(), str.end(), ::isalpha);
            LOG_DEBUG("Interpreter: isalpha retornando: " << (result ? "true" : "false"));
            return ValueWrapper(result);
        }
        LOG_DEBUG("Interpreter: isalpha retornando false (não é string)");
        return ValueWrapper(false);
    }
    else if (functions.find(func_name) != functions.end())
    {
        ValueWrapper result = execute_function(functions[func_name], call->getArgs());
        LOG_DEBUG("Interpreter: Função definida pelo usuário " << func_name << " retornou valor");
        return result;
    }
    LOG_DEBUG("Interpreter: Erro, função não suportada: " << func_name);
    throw RunTimeError("Função não suportada: " + func_name);
}

ValueWrapper Interpreter::evaluateRelational(Relational *relational)
{
    LOG_DEBUG("Interpreter: Avaliando operação relacional");
    if (!relational->getLeft() || !relational->getRight())
    {
        LOG_DEBUG("Interpreter: Operando nulo em expressão relacional");
        throw RunTimeError("Operando nulo em expressão relacional");
    }
    ValueWrapper left_value = evaluate(relational->getLeft());
    ValueWrapper right_value = evaluate(relational->getRight());
    std::string op = relational->getToken().getTag();
    LOG_DEBUG("Interpreter: Operação relacional: " << convert_value_to_string(left_value)
                                                   << " " << op << " " << convert_value_to_string(right_value));

    if (std::holds_alternative<double>(left_value.data) && std::holds_alternative<double>(right_value.data))
    {
        double left_num = std::get<double>(left_value.data);
        double right_num = std::get<double>(right_value.data);
        if (op == "<")
            return ValueWrapper(left_num < right_num);
        if (op == ">")
            return ValueWrapper(left_num > right_num);
        if (op == "LTE")
            return ValueWrapper(left_num <= right_num);
        if (op == "GTE")
            return ValueWrapper(left_num >= right_num);
        if (op == "EQ")
            return ValueWrapper(left_num == right_num);
        if (op == "NEQ")
            return ValueWrapper(left_num != right_num);
    }
    else if (std::holds_alternative<std::string>(left_value.data) &&
             std::holds_alternative<std::string>(right_value.data))
    {
        std::string left_str = std::get<std::string>(left_value.data);
        std::string right_str = std::get<std::string>(right_value.data);
        if (op == "EQ")
            return ValueWrapper(left_str == right_str);
        if (op == "NEQ")
            return ValueWrapper(left_str != right_str);
    }
    LOG_DEBUG("Interpreter: Erro, tipos incompatíveis para operador relacional: " << op);
    throw RunTimeError("Operador relacional '" + op + "' requer operandos numéricos ou strings compatíveis");
}

ValueWrapper Interpreter::evaluateArithmetic(Arithmetic *arithmetic)
{
    LOG_DEBUG("Interpreter: Avaliando operação aritmética");
    if (!arithmetic->getLeft() || !arithmetic->getRight())
    {
        LOG_DEBUG("Interpreter: Operando nulo em expressão aritmética");
        throw RunTimeError("Operando nulo em expressão aritmética");
    }
    ValueWrapper left_value = evaluate(arithmetic->getLeft());
    ValueWrapper right_value = evaluate(arithmetic->getRight());
    std::string op = arithmetic->getToken().getTag();
    LOG_DEBUG("Interpreter: Operação aritmética: " << convert_value_to_string(left_value)
                                                   << " " << op << " " << convert_value_to_string(right_value));

    if (std::holds_alternative<double>(left_value.data) && std::holds_alternative<double>(right_value.data))
    {
        double left_num = std::get<double>(left_value.data);
        double right_num = std::get<double>(right_value.data);
        if (op == "+")
            return ValueWrapper(left_num + right_num);
        if (op == "-")
            return ValueWrapper(left_num - right_num);
        if (op == "*")
            return ValueWrapper(left_num * right_num);
        if (op == "/")
        {
            if (right_num == 0)
            {
                LOG_DEBUG("Interpreter: Erro, divisão por zero");
                throw RunTimeError("Divisão por zero");
            }
            return ValueWrapper(left_num / right_num);
        }
    }
    LOG_DEBUG("Interpreter: Erro, tipos inválidos para operador aritmético: " << op);
    throw RunTimeError("Operador aritmético '" + op + "' requer operandos numéricos");
}

ValueWrapper Interpreter::evaluateUnary(Unary *unary)
{
    LOG_DEBUG("Interpreter: Avaliando operação unária");
    if (!unary->getExpr())
    {
        LOG_DEBUG("Interpreter: Expressão nula em operação unária");
        throw RunTimeError("Expressão nula em operação unária");
    }
    std::string op = unary->getToken().getTag();
    LOG_DEBUG("Interpreter: Operação unária: " << op);

    if (op == "INC" || op == "DEC")
    {
        Expression *operand = unary->getExpr();
        ValueWrapper result;

        // Caso 1: Operando é um ID (variável simples)
        if (auto *id = dynamic_cast<ID *>(operand))
        {
            std::string var_name = id->getToken().getValue();
            ValueWrapper *var_ptr = nullptr;
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto var_it = it->variables.find(var_name);
                if (var_it != it->variables.end())
                {
                    var_ptr = &var_it->second;
                    break;
                }
            }
            if (!var_ptr)
            {
                throw RunTimeError("Variável não definida: " + var_name);
            }
            if (!std::holds_alternative<double>(var_ptr->data))
            {
                throw RunTimeError("Operadores ++ e -- requerem uma variável numérica");
            }
            double &value = std::get<double>(var_ptr->data);
            if (unary->isPostfix()) // x++ ou x--
            {
                result = ValueWrapper(value);
                if (op == "INC")
                    value += 1.0;
                else
                    value -= 1.0;
            }
            else // ++x ou --x
            {
                if (op == "INC")
                    value += 1.0;
                else
                    value -= 1.0;
                result = ValueWrapper(value);
            }
            LOG_DEBUG("Interpreter: " << var_name << " " << (unary->isPostfix() ? "pós" : "pré") << "-fixado: " << value);
            return result;
        }
        // Caso 2: Operando é um Access (posição de array)
        else if (auto *access = dynamic_cast<Access *>(operand))
        {
            if (!access->getId() || !access->getExpr())
            {
                throw RunTimeError("Acesso com base ou índice nulo");
            }
            std::string base_name = access->getId()->getToken().getValue();
            ValueWrapper index_val = evaluate(access->getExpr());
            if (!std::holds_alternative<double>(index_val.data))
            {
                throw RunTimeError("Índice deve ser um número");
            }
            int index = static_cast<int>(std::get<double>(index_val.data));

            ValueWrapper *base_ptr = nullptr;
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto var_it = it->variables.find(base_name);
                if (var_it != it->variables.end())
                {
                    base_ptr = &var_it->second;
                    break;
                }
            }
            if (!base_ptr)
            {
                throw RunTimeError("Array não definido: " + base_name);
            }
            if (!std::holds_alternative<std::vector<ValueWrapper>>(base_ptr->data))
            {
                throw RunTimeError(base_name + " não é um array");
            }
            auto &arr = std::get<std::vector<ValueWrapper>>(base_ptr->data);
            if (index < 0 || index >= arr.size())
            {
                throw RunTimeError("Índice " + std::to_string(index) + " fora do intervalo para " + base_name);
            }
            if (!std::holds_alternative<double>(arr[index].data))
            {
                throw RunTimeError("Elemento no índice " + std::to_string(index) + " não é numérico");
            }
            double &value = std::get<double>(arr[index].data);
            if (unary->isPostfix())
            {
                result = ValueWrapper(value);
                if (op == "INC")
                    value += 1.0;
                else
                    value -= 1.0;
            }
            else // ++arr[6] ou --arr[6]
            {
                if (op == "INC")
                    value += 1.0;
                else
                    value -= 1.0;
                result = ValueWrapper(value);
            }
            LOG_DEBUG("Interpreter: " << base_name << "[" << index << "] " << (unary->isPostfix() ? "pós" : "pré") << "-fixado: " << value);
            return result;
        }
        throw RunTimeError("Operadores ++ e -- só podem ser aplicados a variáveis ou posições de array");
    }
    else if (op == "-")
    {
        ValueWrapper value = evaluate(unary->getExpr());
        if (std::holds_alternative<double>(value.data))
        {
            return ValueWrapper(-std::get<double>(value.data));
        }
        throw RunTimeError("Operador unário '-' aplicado a tipo não numérico");
    }
    else if (op == "!")
    {
        ValueWrapper value = evaluate(unary->getExpr());
        return ValueWrapper(!is_true(value));
    }
    throw RunTimeError("Operador unário não suportado: " + op);
}

ValueWrapper Interpreter::evaluateLogical(Logical *logical)
{
    LOG_DEBUG("Interpreter: Avaliando operação lógica");
    if (!logical->getLeft() || !logical->getRight())
    {
        LOG_DEBUG("Interpreter: Operando nulo em expressão lógica");
        throw RunTimeError("Operando nulo em expressão lógica");
    }
    ValueWrapper left_value = evaluate(logical->getLeft());
    std::string op = logical->getToken().getTag();
    LOG_DEBUG("Interpreter: Operação lógica: " << convert_value_to_string(left_value)
                                               << " " << op);

    if (op == "AND")
    {
        if (!is_true(left_value))
            return ValueWrapper(false);
        ValueWrapper right_value = evaluate(logical->getRight());
        return ValueWrapper(is_true(right_value));
    }
    else if (op == "OR")
    {
        if (is_true(left_value))
            return ValueWrapper(true);
        ValueWrapper right_value = evaluate(logical->getRight());
        return ValueWrapper(is_true(right_value));
    }
    LOG_DEBUG("Interpreter: Erro, operador lógico não suportado: " << op);
    throw RunTimeError("Operador lógico não suportado: " + op);
}

/**
 * @brief Executa uma função definida pelo usuário.
 *
 * @param func Ponteiro para a definição da função.
 * @param args Lista de argumentos passados na chamada.
 * @return ValueWrapper com o valor de retorno da função.
 *
 * O método empilha um novo escopo, vincula os parâmetros aos argumentos, executa o corpo
 * da função e retorna o valor do retorno.
 */
ValueWrapper Interpreter::execute_function(FuncDef *func, const Arguments &args)
{
    push_scope();
    const Parameters &params = func->getParams();
    if (args.size() != params.size())
    {
        throw RunTimeError("Número incorreto de argumentos para função " + func->getName());
    }

    size_t i = 0;
    for (const auto &[name, param_data] : params)
    {
        ValueWrapper value = evaluate(args[i].get());
        scopes.back().variables[name] = value;
        i++;
    }

    for (const auto &stmt : func->getBody())
    {
        execute_stmt(stmt.get());
        if (return_flag)
        {
            ValueWrapper result = return_value;
            return_flag = false;
            pop_scope();
            return result;
        }
    }
    pop_scope();
    return ValueWrapper(std::string(""));
}

/**
 * @brief Executa um servidor para um canal S_CHANNEL.
 *
 * @param schannel Ponteiro para o nó SChannel.
 *
 * O método configura um socket, vincula-o à porta especificada e fica aguardando
 * conexões. Ao receber uma mensagem, executa a função associada e envia a resposta.
 */
void Interpreter::run_server(SChannel *schannel)
{
    std::string name = schannel->getName();
    ValueWrapper localhost_val = evaluate(schannel->getLocalhostNode());
    ValueWrapper port_val = evaluate(schannel->getPortNode());
    std::string func_name = schannel->getFuncName();
    ValueWrapper desc_val = evaluate(schannel->getDescription());

    std::string localhost = std::get<std::string>(localhost_val.data);
    int port = static_cast<int>(std::get<double>(port_val.data));
    std::string description = std::get<std::string>(desc_val.data);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        throw RunTimeError("Erro ao criar socket para SChannel '" + name + "'");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        throw RunTimeError("Erro ao configurar opções do socket para SChannel '" + name + "'");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        throw RunTimeError("Erro ao vincular socket para SChannel '" + name + "' na porta " + std::to_string(port));
    }

    if (listen(server_fd, 3) < 0)
    {
        throw RunTimeError("Erro ao escutar no socket para SChannel '" + name + "'");
    }

    std::cout << "SChannel '" << name << "' escutando em " << localhost << ":" << port << " (" << description << ")\n";

    while (true)
    {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            std::cerr << "Erro ao aceitar conexão em SChannel '" << name << "'\n";
            continue;
        }

        read(client_fd, buffer, 1024);
        std::string message(buffer);
        std::cout << "Mensagem recebida: " << message << "\n";

        Arguments args;
        args.push_back(std::make_unique<Constant>("STRING", Token("STRING", message)));
        ValueWrapper result = execute_function(functions[func_name], args);

        std::string response = convert_value_to_string(result);
        send(client_fd, response.c_str(), response.length(), 0);
        std::cout << "Resposta enviada: " << response << "\n";

        close(client_fd);
    }
    close(server_fd);
}

/**
 * @brief Converte um ValueWrapper para sua representação em string.
 *
 * @param value Valor a ser convertido.
 * @return std::string Representação textual do valor.
 *
 * Utiliza std::visit para tratar os diferentes tipos armazenados no ValueWrapper.
 */
std::string Interpreter::convert_value_to_string(const ValueWrapper &value)
{
    if (!value.isInitialized()) // Verifica se o valor foi inicializado
    {
        throw std::runtime_error("ValueWrapper não inicializado");
    }

    return std::visit([this](auto &&val) -> std::string
                      {
         using T = std::decay_t<decltype(val)>;
         if constexpr (std::is_same_v<T, double>)
         {
             // Se o número for inteiro, remove a parte decimal
             if (std::fabs(val - std::round(val)) < 1e-9)
                 return std::to_string(static_cast<int>(std::round(val)));
             else
             {
                 std::ostringstream oss;
                 oss << val;
                 return oss.str();
             }
         }
         else if constexpr (std::is_same_v<T, std::string>)
         {
             return val;
         }
         else if constexpr (std::is_same_v<T, bool>)
         {
             return val ? "true" : "false";
         }
         else if constexpr (std::is_same_v<T, std::vector<ValueWrapper>>)
         {
             std::ostringstream oss;
             oss << "[";
             bool first = true;
             for (const auto &elem : val)
             {
                 if (!first)
                     oss << ", ";
                 oss << this->convert_value_to_string(elem);
                 first = false;
             }
             oss << "]";
             return oss.str();
         }
         else
         {
             throw std::runtime_error("Tipo não suportado em ValueWrapper");
         } }, value.data);
}

/**
 * @brief Executa um statement (comando) do programa.
 *
 * @param stmt Ponteiro para o statement a ser executado.
 *
 * Suporta vários tipos de statements: atribuição, chamada de função, definição de função,
 * controle de fluxo (if, while, break, continue), operações paralelas (par, seq), canais, etc.
 */
void Interpreter::execute_stmt(Node *stmt)
{
    if (!stmt)
    {
        LOG_DEBUG("Interpreter: Tentativa de executar statement nulo");
        throw RunTimeError("Tentativa de executar statement nulo");
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
            LOG_DEBUG("Interpreter: Tentativa de atribuir ValueWrapper não inicializado");
            throw RunTimeError("ValueWrapper não inicializado ao atribuir");
        }

        // Caso o lado esquerdo seja um identificador (variável)
        if (auto *id = dynamic_cast<ID *>(left))
        {
            std::string var_name = id->getToken().getValue();

            // Verificar primeiro o escopo atual
            auto &current_scope = scopes.back();
            auto var_it = current_scope.variables.find(var_name);

            // Se a variável já existe no escopo atual
            if (var_it != current_scope.variables.end())
            {
                // Se for um array, atualizar os elementos
                if (std::holds_alternative<std::vector<ValueWrapper>>(var_it->second.data))
                {
                    auto &arr = std::get<std::vector<ValueWrapper>>(var_it->second.data);
                    if (std::holds_alternative<std::vector<ValueWrapper>>(value.data))
                    {
                        const auto &new_elements = std::get<std::vector<ValueWrapper>>(value.data);
                        size_t copy_size = std::min(new_elements.size(), arr.size());
                        for (size_t i = 0; i < copy_size; ++i)
                        {
                            arr[i] = new_elements[i];
                        }
                        LOG_DEBUG("Interpreter: Atualizando array " << var_name << " com " << copy_size << " elementos no escopo local");
                    }
                    else
                    {
                        throw RunTimeError("Tentativa de atribuir valor não-array a um array existente");
                    }
                }
                else
                {
                    // Atribuição normal no escopo atual
                    var_it->second = value;
                    LOG_DEBUG("Interpreter: Atribuindo " << var_name << " = " << convert_value_to_string(value) << " no escopo local (existente)");
                }
            }
            else
            {
                // Buscar nos escopos superiores para atualização
                for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
                {
                    auto var_it = it->variables.find(var_name);
                    if (var_it != it->variables.end())
                    {
                        // Se for um array, atualizar os elementos
                        if (std::holds_alternative<std::vector<ValueWrapper>>(var_it->second.data))
                        {
                            auto &arr = std::get<std::vector<ValueWrapper>>(var_it->second.data);
                            if (std::holds_alternative<std::vector<ValueWrapper>>(value.data))
                            {
                                const auto &new_elements = std::get<std::vector<ValueWrapper>>(value.data);
                                size_t copy_size = std::min(new_elements.size(), arr.size());
                                for (size_t i = 0; i < copy_size; ++i)
                                {
                                    arr[i] = new_elements[i];
                                }
                                LOG_DEBUG("Interpreter: Atualizando array " << var_name << " com " << copy_size << " elementos no escopo existente");
                            }
                            else
                            {
                                throw RunTimeError("Tentativa de atribuir valor não-array a um array existente");
                            }
                        }
                        else
                        {
                            // Atribuição normal no escopo onde a variável existe
                            var_it->second = value;
                            LOG_DEBUG("Interpreter: Atribuindo " << var_name << " = " << convert_value_to_string(value) << " no escopo existente");
                        }
                        return; // Sai após atualizar
                    }
                }
                // Se não encontrada, criar no escopo atual
                current_scope.variables[var_name] = value;
                LOG_DEBUG("Interpreter: Atribuindo " << var_name << " = " << convert_value_to_string(value) << " no escopo local (nova variável)");
            }
        }
        // Caso o lado esquerdo seja um acesso a elemento (array ou string)
        else if (auto *access = dynamic_cast<Access *>(left))
        {
            std::string base_name = access->getId()->getToken().getValue();
            ValueWrapper index_val = evaluate(access->getExpr());

            if (std::holds_alternative<double>(index_val.data))
            {
                int index = static_cast<int>(std::get<double>(index_val.data));
                for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
                {
                    auto var_it = it->variables.find(base_name);
                    if (var_it != it->variables.end())
                    {
                        ValueWrapper &base_val = var_it->second;
                        if (std::holds_alternative<std::vector<ValueWrapper>>(base_val.data))
                        {
                            auto &arr = std::get<std::vector<ValueWrapper>>(base_val.data);
                            if (index >= 0 && index < arr.size())
                            {
                                arr[index] = value;
                                LOG_DEBUG("Interpreter: Atribuindo " << base_name << "[" << index << "] = " << convert_value_to_string(value));
                            }
                            else
                            {
                                throw RunTimeError("Índice " + std::to_string(index) + " fora do intervalo para o array " + base_name);
                            }
                        }
                        else if (std::holds_alternative<std::string>(base_val.data))
                        {
                            std::string &str = std::get<std::string>(base_val.data);
                            if (index >= 0 && index < str.length())
                            {
                                if (std::holds_alternative<std::string>(value.data) && std::get<std::string>(value.data).length() == 1)
                                {
                                    str[index] = std::get<std::string>(value.data)[0];
                                    LOG_DEBUG("Interpreter: Atribuindo " << base_name << "[" << index << "] = " << convert_value_to_string(value));
                                }
                                else
                                {
                                    throw RunTimeError("Atribuição a string requer um caractere");
                                }
                            }
                            else
                            {
                                throw RunTimeError("Índice " + std::to_string(index) + " fora do intervalo para a string " + base_name);
                            }
                        }
                        else
                        {
                            throw RunTimeError(base_name + " não é um array nem uma string");
                        }
                        return;
                    }
                }
                throw RunTimeError("Variável " + base_name + " não definida");
            }
            else
            {
                throw RunTimeError("Índice deve ser um número");
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
        ValueWrapper cond_value = evaluate(if_stmt->getCondition());
        LOG_DEBUG("Interpreter: Avaliando IF, condição: " << convert_value_to_string(cond_value));
        if (is_true(cond_value))
        {
            push_scope();
            LOG_DEBUG("Interpreter: Executando corpo do IF");
            for (const auto &stmt_ptr : if_stmt->getBody())
            {
                execute_stmt(stmt_ptr.get());
                if (return_flag || break_flag || continue_flag)
                    break;
            }
            pop_scope();
        }
        else if (if_stmt->getElseStmt())
        {
            push_scope();
            LOG_DEBUG("Interpreter: Executando corpo do ELSE");
            for (const auto &stmt_ptr : *if_stmt->getElseStmt())
            {
                execute_stmt(stmt_ptr.get());
                if (return_flag || break_flag || continue_flag)
                    break;
            }
            pop_scope();
        }
    }
    else if (auto *while_stmt = dynamic_cast<While *>(stmt))
    {
        LOG_DEBUG("Interpreter: Iniciando WHILE");
        while (true)
        {
            ValueWrapper cond_value = evaluate(while_stmt->getCondition());
            LOG_DEBUG("Interpreter: Condição do WHILE: " << convert_value_to_string(cond_value));
            if (!is_true(cond_value))
            {
                LOG_DEBUG("Interpreter: Saindo do WHILE");
                break;
            }
            push_scope();
            for (const auto &stmt_ptr : while_stmt->getBody())
            {
                execute_stmt(stmt_ptr.get());
                if (return_flag)
                {
                    LOG_DEBUG("Interpreter: Retorno detectado no WHILE");
                    pop_scope();
                    return;
                }
                if (break_flag)
                {
                    LOG_DEBUG("Interpreter: Break detectado no WHILE");
                    break_flag = false;
                    pop_scope();
                    break;
                }
                if (continue_flag)
                {
                    LOG_DEBUG("Interpreter: Continue detectado no WHILE");
                    continue_flag = false;
                    break;
                }
            }
            pop_scope();
            if (break_flag)
                break;
        }
    }
    else if (auto *brk = dynamic_cast<Break *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando BREAK");
        break_flag = true;
    }
    else if (auto *cont = dynamic_cast<Continue *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando CONTINUE");
        continue_flag = true;
    }
    else if (auto *par = dynamic_cast<Par *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando PAR (execução paralela)");
        std::vector<std::thread> threads;
        for (const auto &stmt_ptr : par->getBody())
        {
            threads.emplace_back([this, &stmt_ptr]()
                                 { execute_stmt(stmt_ptr.get()); });
            LOG_DEBUG("Interpreter: Thread criada para statement em PAR");
        }
        for (auto &thread : threads)
        {
            thread.join();
            LOG_DEBUG("Interpreter: Thread concluída em PAR");
        }
    }
    else if (auto *seq = dynamic_cast<Seq *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando SEQ (sequência), is_block = " << (seq->isBlock() ? "true" : "false"));
        if (seq->isBlock())
        {
            push_scope();
            LOG_DEBUG("Interpreter: Criando novo escopo para SEQ");
        }
        for (const auto &stmt_ptr : seq->getBody())
        {
            execute_stmt(stmt_ptr.get());
            if (return_flag || break_flag || continue_flag)
                break;
        }
        if (seq->isBlock())
        {
            pop_scope();
            LOG_DEBUG("Interpreter: Desempilhando escopo do SEQ");
        }
    }
    else if (auto *cchannel = dynamic_cast<CChannel *>(stmt))
    {
        LOG_DEBUG("Interpreter: Executando C_CHANNEL");
        std::string name = cchannel->getName();
        ValueWrapper localhost = evaluate(cchannel->getLocalhostNode());
        ValueWrapper port = evaluate(cchannel->getPortNode());
        std::cout << "CChannel '" << name << "' criado com localhost: "
                  << std::get<std::string>(localhost.data) << ", port: "
                  << std::get<double>(port.data) << std::endl;
        LOG_DEBUG("Interpreter: CChannel criado: " << name);
    }
    else if (auto *schannel = dynamic_cast<SChannel *>(stmt))
    {
        LOG_DEBUG("Interpreter: Iniciando servidor S_CHANNEL");
        run_server(schannel);
    }
    else if (auto *array_decl = dynamic_cast<ArrayDecl *>(stmt))
    {
        std::string var_name = array_decl->getName();
        ValueWrapper size_val = evaluate(array_decl->getSizeExpr());

        if (!std::holds_alternative<double>(size_val.data))
        {
            throw RunTimeError("Tamanho do array '" + var_name + "' deve ser um número");
        }

        int size = static_cast<int>(std::get<double>(size_val.data));
        if (size < 0)
        {
            throw RunTimeError("Tamanho do array '" + var_name + "' não pode ser negativo");
        }

        std::vector<ValueWrapper> arr(size, ValueWrapper(0.0));
        scopes.back().variables[var_name] = ValueWrapper(arr);
        LOG_DEBUG("Interpreter: Array '" << var_name << "' criado com tamanho " << size);
    }
    else
    {
        LOG_DEBUG("Interpreter: Erro, statement não suportado: " << typeid(*stmt).name());
        throw RunTimeError("Statement não suportado");
    }
}

/**
 * @brief Executa um módulo, percorrendo e executando cada statement.
 *
 * @param module Ponteiro para o módulo a ser executado.
 *
 * Lança exceção se o módulo for nulo.
 */
void Interpreter::execute(Module *module)
{
    if (!module)
    {
        LOG_DEBUG("Interpreter: Tentativa de executar módulo nulo");
        throw RunTimeError("Módulo nulo fornecido para execução");
    }
    LOG_DEBUG("Interpreter: Iniciando execução do módulo");

    Node *root_stmt = module->getStmt();
    if (!root_stmt)
    {
        LOG_DEBUG("Interpreter: Módulo não contém statements");
        throw RunTimeError("Módulo vazio");
    }

    // Verifica se o nó raiz é um Seq
    if (auto *seq = dynamic_cast<Seq *>(root_stmt))
    {
        LOG_DEBUG("Interpreter: Módulo contém um Seq, isBlock() = " << (seq->isBlock() ? "true" : "false"));
        // Se isBlock() é false, usa o escopo global existente; se true, cria um novo escopo
        if (seq->isBlock())
        {
            push_scope();
            LOG_DEBUG("Interpreter: Criando novo escopo para o módulo (is_block = true)");
        }

        for (const auto &stmt : seq->getBody())
        {
            execute_stmt(stmt.get());
            if (return_flag)
            {
                LOG_DEBUG("Interpreter: Retorno detectado no módulo");
                break;
            }
        }

        if (seq->isBlock())
        {
            pop_scope();
            LOG_DEBUG("Interpreter: Desempilhando escopo do módulo (is_block = true)");
        }
    }
    else
    {
        LOG_DEBUG("Interpreter: Erro, nó raiz do módulo não é um Seq");
        throw RunTimeError("Estrutura inválida do módulo: esperado Seq");
    }

    LOG_DEBUG("Interpreter: Execução do módulo concluída");
}

/**
 * @brief Sobrecarga do operador << para imprimir um ValueWrapper.
 *
 * @param os Stream de saída.
 * @param v ValueWrapper a ser impresso.
 * @return std::ostream& Referência ao stream de saída.
 *
 * Utiliza std::visit para tratar os diferentes tipos armazenados em ValueWrapper.
 */
std::ostream &operator<<(std::ostream &os, const ValueWrapper &v)
{
    std::visit([&os](const auto &val)
               {
         using T = std::decay_t<decltype(val)>;
         if constexpr (std::is_same_v<T, std::monostate>) {
             os << "[uninitialized]";
         }
         else if constexpr (std::is_same_v<T, double>) {
             os << val;
         }
         else if constexpr (std::is_same_v<T, bool>) {
             os << (val ? "true" : "false");
         }
         else if constexpr (std::is_same_v<T, std::string>) {
             os << val;
         }
         else if constexpr (std::is_same_v<T, std::vector<ValueWrapper>>) {
             os << "[";
             bool first = true;
             for (const auto &elem : val) {
                 if (!first) os << ", ";
                 os << elem;
                 first = false;
             }
             os << "]";
         } }, v.data);
    return os;
}
