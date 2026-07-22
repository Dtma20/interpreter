/**
 * @file interpreter_expr.cpp
 * @brief Implementação dos métodos de avaliação de expressões do Interpreter.
 *
 * Contém: evaluate(), evaluateConstant(), evaluateID(), evaluateArray(),
 * evaluateAccess(), evaluateRelational(), evaluateArithmetic(),
 * evaluateUnary(), evaluateLogical().
 */

#include "../../include/interpreter/interpreter_core.hpp"
#include <iostream>
#include <cmath>
#include "../../include/debug.hpp"

/**
 * @brief Avalia uma expressão e retorna seu valor.
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
        return evaluateConstant(constant);
    else if (auto *id = dynamic_cast<ID *>(expr))
        return evaluateID(id);
    else if (auto *array = dynamic_cast<Array *>(expr))
        return evaluateArray(array);
    else if (auto *access = dynamic_cast<Access *>(expr))
        return evaluateAccess(access);
    else if (auto *call = dynamic_cast<Call *>(expr))
        return evaluateFunctionCall(call);
    else if (auto *relational = dynamic_cast<Relational *>(expr))
        return evaluateRelational(relational);
    else if (auto *arithmetic = dynamic_cast<Arithmetic *>(expr))
        return evaluateArithmetic(arithmetic);
    else if (auto *unary = dynamic_cast<Unary *>(expr))
        return evaluateUnary(unary);
    else if (auto *logical = dynamic_cast<Logical *>(expr))
        return evaluateLogical(logical);

    LOG_DEBUG("Interpreter: Erro, tipo de expressão não suportado");
    throw RunTimeError("Expressão não suportada");
}

/**
 * @brief Avalia uma constante e retorna o valor associado.
 */
ValueWrapper Interpreter::evaluateConstant(Constant *constant)
{
    std::string valueStr = constant->getToken().getValue();
    std::string typeStr = constant->getType();
    LOG_DEBUG("Interpreter: Constante detectada, tipo: " << typeStr << ", valor: " << valueStr);

    if (typeStr == "NUM")
    {
        long double num = std::stod(valueStr);
        LOG_DEBUG("Interpreter: Convertendo num para long double: " << num);
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

/**
 * @brief Avalia um identificador (ID) e retorna o valor associado.
 */
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
            return *(var_it->second);
        }
    }
    LOG_DEBUG("Interpreter: Erro, variável não definida: " << var_name);
    throw RunTimeError("Variável não definida: " + var_name);
}

/**
 * @brief Avalia um array e retorna um ValueWrapper contendo o vetor de valores.
 */
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

/**
 * @brief Avalia um acesso (array ou string) e retorna um ValueWrapper contendo o valor
 *        no índice especificado.
 */
ValueWrapper Interpreter::evaluateAccess(Access *access)
{
    if (!access->getBase() || !access->getIndex())
    {
        LOG_DEBUG("Interpreter: Acesso com base ou índice nulo");
        throw RunTimeError("Acesso com base ou índice nulo");
    }
    ValueWrapper base_val = evaluate(access->getBase());
    ValueWrapper index_val = evaluate(access->getIndex());

    if (std::holds_alternative<std::vector<ValueWrapper>>(base_val.data) &&
        std::holds_alternative<long double>(index_val.data))
    {
        int index = static_cast<int>(std::get<long double>(index_val.data));
        auto &arr = std::get<std::vector<ValueWrapper>>(base_val.data);
        if (index >= 0 && index < static_cast<int>(arr.size()))
        {
            LOG_DEBUG("Interpreter: Acesso a array no índice " << index);
            return arr[index];
        }
        LOG_DEBUG("Interpreter: Erro, índice fora do intervalo: " << index);
        throw RunTimeError("Índice " + std::to_string(index) + " fora do intervalo");
    }
    else if (std::holds_alternative<std::string>(base_val.data) &&
             std::holds_alternative<long double>(index_val.data))
    {
        std::string str = std::get<std::string>(base_val.data);
        int index = static_cast<int>(std::get<long double>(index_val.data));
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

/**
 * @brief Avalia uma expressão relacional.
 */
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

    if (std::holds_alternative<long double>(left_value.data) && std::holds_alternative<long double>(right_value.data))
    {
        long double left_num = std::get<long double>(left_value.data);
        long double right_num = std::get<long double>(right_value.data);
        if (op == "<") return ValueWrapper(left_num < right_num);
        if (op == ">") return ValueWrapper(left_num > right_num);
        if (op == "LTE") return ValueWrapper(left_num <= right_num);
        if (op == "GTE") return ValueWrapper(left_num >= right_num);
        if (op == "EQ") return ValueWrapper(left_num == right_num);
        if (op == "NEQ") return ValueWrapper(left_num != right_num);
    }
    else if (std::holds_alternative<std::string>(left_value.data) &&
             std::holds_alternative<std::string>(right_value.data))
    {
        std::string left_str = std::get<std::string>(left_value.data);
        std::string right_str = std::get<std::string>(right_value.data);
        if (op == "EQ") return ValueWrapper(left_str == right_str);
        if (op == "NEQ") return ValueWrapper(left_str != right_str);
    }
    LOG_DEBUG("Interpreter: Erro, tipos incompatíveis para operador relacional: " << op);
    throw RunTimeError("Operador relacional '" + op + "' requer operandos numéricos ou strings compatíveis");
}

/**
 * @brief Avalia uma expressão aritmética.
 */
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

    if (std::holds_alternative<long double>(left_value.data) && std::holds_alternative<long double>(right_value.data))
    {
        long double left_num = std::get<long double>(left_value.data);
        long double right_num = std::get<long double>(right_value.data);
        if (op == "+") return ValueWrapper(left_num + right_num);
        if (op == "-") return ValueWrapper(left_num - right_num);
        if (op == "*") return ValueWrapper(left_num * right_num);
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

/**
 * @brief Avalia uma expressão unária.
 */
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

        if (auto *id = dynamic_cast<ID *>(operand))
        {
            std::string var_name = id->getToken().getValue();
            ValueWrapper *var_ptr = nullptr;
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto var_it = it->variables.find(var_name);
                if (var_it != it->variables.end())
                {
                    var_ptr = var_it->second.get();
                    break;
                }
            }
            if (!var_ptr)
                throw RunTimeError("Variável não definida: " + var_name);
            if (!std::holds_alternative<long double>(var_ptr->data))
                throw RunTimeError("Operadores ++ e -- requerem uma variável numérica");
            long double &value = std::get<long double>(var_ptr->data);
            if (unary->isPostfix())
            {
                result = ValueWrapper(value);
                if (op == "INC") value += 1.0; else value -= 1.0;
            }
            else
            {
                if (op == "INC") value += 1.0; else value -= 1.0;
                result = ValueWrapper(value);
            }
            LOG_DEBUG("Interpreter: " << var_name << " " << (unary->isPostfix() ? "pós" : "pré") << "-fixado: " << value);
            return result;
        }
        else if (auto *access = dynamic_cast<Access *>(operand))
        {
            if (!access->getBase() || !access->getIndex())
                throw RunTimeError("Acesso com base ou índice nulo");
            std::string base_name = access->getBase()->getToken().getValue();
            ValueWrapper index_val = evaluate(access->getIndex());
            if (!std::holds_alternative<long double>(index_val.data))
                throw RunTimeError("Índice deve ser um número");
            int index = static_cast<int>(std::get<long double>(index_val.data));

            ValueWrapper *base_ptr = nullptr;
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            {
                auto var_it = it->variables.find(base_name);
                if (var_it != it->variables.end())
                {
                    base_ptr = var_it->second.get();
                    break;
                }
            }
            if (!base_ptr)
                throw RunTimeError("Array não definido: " + base_name);
            if (!std::holds_alternative<std::vector<ValueWrapper>>(base_ptr->data))
                throw RunTimeError(base_name + " não é um array");
            auto &arr = std::get<std::vector<ValueWrapper>>(base_ptr->data);
            if (index < 0 || index >= arr.size())
                throw RunTimeError("Índice " + std::to_string(index) + " fora do intervalo para " + base_name);
            if (!std::holds_alternative<long double>(arr[index].data))
                throw RunTimeError("Elemento no índice " + std::to_string(index) + " não é numérico");
            long double &value = std::get<long double>(arr[index].data);
            if (unary->isPostfix())
            {
                result = ValueWrapper(value);
                if (op == "INC") value += 1.0; else value -= 1.0;
            }
            else
            {
                if (op == "INC") value += 1.0; else value -= 1.0;
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
        if (std::holds_alternative<long double>(value.data))
            return ValueWrapper(-std::get<long double>(value.data));
        throw RunTimeError("Operador unário '-' aplicado a tipo não numérico");
    }
    else if (op == "!")
    {
        ValueWrapper value = evaluate(unary->getExpr());
        return ValueWrapper(!is_true(value));
    }
    throw RunTimeError("Operador unário não suportado: " + op);
}

/**
 * @brief Avalia uma expressão lógica (AND ou OR).
 */
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
    LOG_DEBUG("Interpreter: Operação lógica: " << convert_value_to_string(left_value) << " " << op);

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
