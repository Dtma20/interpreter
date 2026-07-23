#ifndef INTERPRETER_CORE_HPP
#define INTERPRETER_CORE_HPP

#pragma once
#include <map>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <stack>
#include <variant>
#include "../ast.hpp"
#include "../error.hpp"
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  using ssize_t = ptrdiff_t;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <netdb.h>
#endif

extern std::mutex cout_mutex;

/**
 * @brief Representa o valor de um canal de cliente.
 *
 * Conexão por request: cada chamada a request() conecta, envia frame
 * com prefixo de tamanho, recebe resposta emoldurada, e fecha o socket.
 * Guarda apenas host e porta; sem estado persistente de socket.
 */
class CChannelValue {
    public:
        CChannelValue(const std::string &host, uint16_t port);
        ~CChannelValue() = default;

        CChannelValue(const CChannelValue&) = delete;
        CChannelValue& operator=(const CChannelValue&) = delete;
        CChannelValue(CChannelValue&&) noexcept = default;
        CChannelValue& operator=(CChannelValue&&) noexcept = default;

        /** Marca o canal como fechado; request() subsequente lança RunTimeError. */
        void close() noexcept { closed_ = true; }

        /**
         * @brief Ciclo completo request/response.
         *
         * Conecta ao host:port, envia @p msg emoldurada com prefixo de
         * 4 bytes big-endian, lê resposta emoldurada, fecha socket, retorna.
         *
         * @throws RunTimeError se canal fechado, host/port inválidos,
         *         timeout, ou peer fechar antes de responder.
         */
        std::string request(const std::string &msg);

        const std::string& host() const noexcept { return host_; }
        uint16_t           port() const noexcept { return port_; }
    
    private:
        std::string host_;
        uint16_t    port_;
        bool        closed_ = false;
    };

/**
 * @brief Wrapper para std::variant que representa valores em tempo de execução.
 *
 * Permite armazenar números (long double), booleanos, strings ou vetores de ValueWrapper.
 * Os tipos são definidos na ordem: tipos primitivos primeiro, depois containers.
 */
struct ValueWrapper
{
    std::variant<std::monostate, long double, bool, std::string, std::vector<ValueWrapper>, std::shared_ptr<CChannelValue>> data;

    ValueWrapper() : data(std::monostate{}) {}
    ValueWrapper(long double d) : data(d) {}
    ValueWrapper(bool b) : data(b) {}
    ValueWrapper(const std::string &s) : data(s) {}
    ValueWrapper(std::string &&s) : data(std::move(s)) {}
    ValueWrapper(const char *s) : data(std::string(s)) {}
    ValueWrapper(const std::vector<ValueWrapper> &vec) : data(vec) {}
    ValueWrapper(std::vector<ValueWrapper> &&vec) : data(std::move(vec)) {}
    ValueWrapper(std::shared_ptr<CChannelValue> cch) : data(cch) {}

    bool isInitialized() const
    {
        return !std::holds_alternative<std::monostate>(data);
    }

    operator std::variant<std::monostate, long double, bool, std::string, std::vector<ValueWrapper>, std::shared_ptr<CChannelValue>>() const
    {
        return data;
    }
};

/**
 * @brief Classe Interpreter.
 *
 * Responsável pela execução do programa, avaliando expressões,
 * executando statements e gerenciando escopos e funções.
 */
class Interpreter
{
private:
    struct Scope
    {
        std::map<std::string, std::shared_ptr<ValueWrapper>> variables;
    };

    std::vector<Scope> scopes;
    std::map<std::string, FuncDef *> functions;
    bool break_flag;
    bool continue_flag;
    bool return_flag;
    ValueWrapper return_value;

    // N5: Recursion depth guard
    size_t recursion_depth = 0;
    size_t max_recursion_depth = 1000;

    struct DepthGuard {
        Interpreter &interp;
        bool committed = false;
        explicit DepthGuard(Interpreter &i) : interp(i) {
            if (interp.recursion_depth >= interp.max_recursion_depth) {
                throw RunTimeError("Recursão máxima excedida (" +
                    std::to_string(interp.max_recursion_depth) + ")");
            }
            ++interp.recursion_depth;
            committed = true;
        }
        ~DepthGuard() { if (committed) interp.recursion_depth--; }
    };

    struct ScopeGuard {
        Interpreter &interp;
        explicit ScopeGuard(Interpreter &i) : interp(i) { interp.push_scope(); }
        ~ScopeGuard() { interp.pop_scope(); }
    };

    ValueWrapper evaluate(Expression *expr);
    void execute_stmt(Node *stmt);
    ValueWrapper execute_function(FuncDef *func, const Arguments &args);
    void push_scope();
    void pop_scope();
    bool is_true(const ValueWrapper &value);
    void run_server(SChannel *schannel);
    void run_client(CChannel *cchannel);
    std::string convert_value_to_string(const ValueWrapper &value);

    /**
     * @brief Snapshot isolado para um braço de `par`.
     *
     * Copia scopes/functions; valores de escopos externos continuam
     * partilhados via shared_ptr (mutações visíveis no pai).
     * Flags e profundidade de recursão são privados por thread.
     */
    Interpreter create_par_worker() const;

    /** Executa um statement num worker isolado (usado pelos braços de `par`). */
    void run_isolated_par_arm(Node *stmt);

    ValueWrapper evaluateConstant(Constant *constant);
    ValueWrapper evaluateID(ID *id);
    ValueWrapper evaluateArray(Array *array);
    ValueWrapper evaluateAccess(Access *access);
    ValueWrapper* resolveAccessLvalue(Access *access);
    ValueWrapper evaluateFunctionCall(Call *call);
    ValueWrapper evaluateRelational(Relational *relational);
    ValueWrapper evaluateArithmetic(Arithmetic *arithmetic);
    ValueWrapper evaluateUnary(Unary *unary);
    ValueWrapper evaluateLogical(Logical *logical);

    // Builtin function helpers
    ValueWrapper builtin_print(Call *call);
    ValueWrapper builtin_len(Call *call);
    ValueWrapper builtin_to_num(Call *call);
    ValueWrapper builtin_to_string(Call *call);
    ValueWrapper builtin_isnum(Call *call);
    ValueWrapper builtin_isalpha(Call *call);
    ValueWrapper builtin_exp(Call *call);
    ValueWrapper builtin_randf(Call *call);
    ValueWrapper builtin_randi(Call *call);
    ValueWrapper builtin_input(Call *call);

    // Channel builtins
    ValueWrapper builtin_send(Call *call);
    ValueWrapper builtin_close(Call *call);

public:
    Interpreter();
    void execute(Module *module);
    void set_max_recursion_depth(size_t d) { max_recursion_depth = d; }
    size_t get_max_recursion_depth() const { return max_recursion_depth; }
};

// Free functions
std::string unescape_string(const std::string &input);
std::ostream &operator<<(std::ostream &os, const ValueWrapper &v);

// Safe numeric conversions (T12)
// Rejeita não-finito, fracionário, negativo, overflow.
size_t  to_index(long double value, const char *context);
uint16_t to_port(long double value, const char *context);

#endif // INTERPRETER_CORE_HPP
