/**
 * @file interpreter_core.cpp
 * @brief Implementação dos métodos centrais do Interpreter.
 *
 * Contém: unescape_string(), Interpreter(), execute(), push_scope(),
 * pop_scope(), is_true(), convert_value_to_string(), operator<<().
 */

#include <memory>
#include "../../include/interpreter/interpreter_core.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <limits>
#include "../../include/debug.hpp"

std::mutex cout_mutex;

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

/**
 * @brief Remove escape sequences de uma string.
 *
 * Substitui sequências de escape no formato "\x" por seu caractere
 * correspondente. As sequências de escape suportadas são:
 * - \n: nova linha
 * - \t: tabulação horizontal
 * - \r: carriage return
 * - \\: barra invertida
 * - \": aspas duplas
 *
 * @param input String contendo as sequências de escape a serem removidas.
 * @return String sem as sequências de escape.
 */
std::string unescape_string(const std::string &input)
{
    std::string result;
    for (size_t i = 0; i < input.length(); ++i)
    {
        if (input[i] == '\\' && i + 1 < input.length())
        {
            ++i;
            switch (input[i])
            {
            case 'n':
                result += '\n';
                break;
            case 't':
                result += '\t';
                break;
            case 'r':
                result += '\r';
                break;
            case '\\':
                result += '\\';
                break;
            case '"':
                result += '"';
                break;
            default:
                result += input[i];
                break;
            }
        }
        else
        {
            result += input[i];
        }
    }
    return result;
}

/**
 * @brief Construtor do interpretador.
 */
Interpreter::Interpreter() : break_flag(false), continue_flag(false), return_flag(false), return_value()
{
    LOG_DEBUG("Interpreter: Construtor chamado, inicializando flags e escopo");
    push_scope();
}

/**
 * @brief Cria um worker isolado para um braço de `par` (C01).
 *
 * Cada thread recebe a sua própria pilha de scopes e flags. Os
 * ValueWrappers dos scopes externos são partilhados via shared_ptr,
 * por isso atribuições a variáveis já existentes ficam visíveis no
 * interpretador pai sem partilhar o contentor `scopes` (data race).
 */
Interpreter Interpreter::create_par_worker() const
{
    Interpreter worker;
    worker.scopes = scopes;
    worker.functions = functions;
    worker.max_recursion_depth = max_recursion_depth;
    worker.break_flag = false;
    worker.continue_flag = false;
    worker.return_flag = false;
    worker.return_value = ValueWrapper();
    worker.recursion_depth = 0;
    return worker;
}

void Interpreter::run_isolated_par_arm(Node *stmt)
{
    Interpreter worker = create_par_worker();
    worker.execute_stmt(stmt);
}

/**
 * @brief Adiciona um novo escopo ao ambiente.
 */
void Interpreter::push_scope()
{
    LOG_DEBUG("Interpreter: Empilhando novo escopo, tamanho atual: " << scopes.size());
    scopes.push_back(Scope{});
}

/**
 * @brief Remove o escopo mais recente do ambiente.
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

// T15: lexical scoping — search [scope_base, scopes.size()) then global[0]
std::shared_ptr<ValueWrapper> Interpreter::find_in_scope(const std::string &name)
{
    // Search local frame: from innermost (back) to scope_base (inclusive)
    for (size_t i = scopes.size(); i-- > scope_base; )
    {
        auto it = scopes[i].variables.find(name);
        if (it != scopes[i].variables.end())
            return it->second;
    }
    // Search global scope (index 0) if not already covered
    if (scope_base > 0)
    {
        auto it = scopes[0].variables.find(name);
        if (it != scopes[0].variables.end())
            return it->second;
    }
    return nullptr;
}

/**
 * @brief Verifica o "valor de verdade" de um ValueWrapper.
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
    else if (std::holds_alternative<long double>(value.data))
    {
        long double num = std::get<long double>(value.data);
        bool result = num != 0.0;
        LOG_DEBUG("Interpreter: Valor é long double (" << num << "), resultado: " << (result ? "true" : "false"));
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
 * @brief Converte um ValueWrapper para sua representação em string.
 */
std::string Interpreter::convert_value_to_string(const ValueWrapper &value)
{
    if (!value.isInitialized())
    {
        throw RunTimeError("ValueWrapper não inicializado");
    }

    return std::visit([this](auto &&val) -> std::string
                      {
         using T = std::decay_t<decltype(val)>;
         if constexpr (std::is_same_v<T, long double>)
         {
             return format_number(val); // T21: sem cast a int, coerente com print()
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
              throw RunTimeError("Tipo não suportado em ValueWrapper");
         } }, value.data);
}

/**
 * @brief Sobrecarga do operador << para imprimir um ValueWrapper.
 */
std::ostream &operator<<(std::ostream &os, const ValueWrapper &v)
{
    std::visit([&os](const auto &val)
               {
         using T = std::decay_t<decltype(val)>;
         if constexpr (std::is_same_v<T, std::monostate>) {
             os << "[uninitialized]";
         }
         else if constexpr (std::is_same_v<T, long double>) {
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

/**
 * @brief Executa um módulo, percorrendo e executando cada statement.
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

    if (auto *seq = dynamic_cast<Seq *>(root_stmt))
    {
        LOG_DEBUG("Interpreter: Módulo contém um Seq, isBlock() = " << (seq->isBlock() ? "true" : "false"));
        std::unique_ptr<ScopeGuard> block_scope;
        if (seq->isBlock())
        {
            block_scope = std::make_unique<ScopeGuard>(*this);
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
    }
    else
    {
        LOG_DEBUG("Interpreter: Erro, nó raiz do módulo não é um Seq");
        throw RunTimeError("Estrutura inválida do módulo: esperado Seq");
    }

    LOG_DEBUG("Interpreter: Execução do módulo concluída");
}

namespace
{

    void validate_finite_integral(long double value, const char *context)
    {
        if (!std::isfinite(value))
        {
            throw RunTimeError(std::string(context) + " deve ser um número finito");
        }
        long double int_part = 0.0L;
        long double frac = std::modf(value, &int_part);
        if (frac != 0.0L)
        {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.6Lf", value);
            throw RunTimeError(std::string(context) + " deve ser número inteiro, recebeu " + buf);
        }
    }

}

size_t to_index(long double value, const char *context)
{
    validate_finite_integral(value, context);
    if (value < 0.0L)
        throw RunTimeError(std::string(context) + " não pode ser negativo");
    if (value > static_cast<long double>(std::numeric_limits<size_t>::max()))
        throw RunTimeError(std::string(context) + " excede o valor máximo permitido");
    return static_cast<size_t>(value);
}

uint16_t to_port(long double value, const char *context)
{
    validate_finite_integral(value, context);
    if (value < 1.0L || value > 65535.0L)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0Lf", value);
        throw RunTimeError(std::string(context) + " deve estar entre 1 e 65535, recebeu " + buf);
    }
    return static_cast<uint16_t>(value);
}

std::string format_number(long double val)
{
    constexpr long double epsilon = 1e-9L;
    std::ostringstream oss;
    if (std::fabs(val - std::trunc(val)) > epsilon)
        oss << std::fixed << std::setprecision(8) << val;
    else
        oss << std::noshowpoint << val;
    return oss.str();
}
