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
 * Um canal de cliente precisa do seu nome e do número do socket criado.
 */
class CChannelValue {
    public:
        CChannelValue(const std::string &host, uint16_t port)
            : sock_fd_(-1), host_(host), port_(port)
        {
            struct addrinfo hints{}, *res = nullptr;
            hints.ai_family   = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags    = AI_ADDRCONFIG;
    
            int err = ::getaddrinfo(host_.c_str(),
                                    std::to_string(port_).c_str(),
                                    &hints, &res);
            if (err != 0) {
                throw std::runtime_error("CChannel: getaddrinfo falhou para '" +
                                         host_ + "': " + gai_strerror(err));
            }

            for (auto ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
                sock_fd_ = ::socket(ptr->ai_family,
                                    ptr->ai_socktype,
                                    ptr->ai_protocol);
                if (sock_fd_ < 0) continue;
    
                if (::connect(sock_fd_, ptr->ai_addr, ptr->ai_addrlen) == 0) {
                    break;
                }
                ::close(sock_fd_);
                sock_fd_ = -1;
            }
    
            freeaddrinfo(res);
    
            if (sock_fd_ < 0) {
                throw std::runtime_error("CChannel: falha ao conectar em " +
                                         host_ + ":" + std::to_string(port_));
            }
        }

        ~CChannelValue() {
            if (sock_fd_ >= 0) {
                ::close(sock_fd_);
            }
        }

        CChannelValue(const CChannelValue&) = delete;
        CChannelValue& operator=(const CChannelValue&) = delete;
    
        CChannelValue(CChannelValue&& other) noexcept
            : sock_fd_(other.sock_fd_),
              host_(std::move(other.host_)),
              port_(other.port_)
        {
            other.sock_fd_ = -1;
        }
    
        CChannelValue& operator=(CChannelValue&& other) noexcept {
            if (this != &other) {
                if (sock_fd_ >= 0) ::close(sock_fd_);
                sock_fd_ = other.sock_fd_;
                host_    = std::move(other.host_);
                port_    = other.port_;
                other.sock_fd_ = -1;
            }
            return *this;
        }

        void close() noexcept {
            if (sock_fd_ >= 0) {
                ::close(sock_fd_);
                sock_fd_ = -1;
            }
        }

        void send_raw(const std::string &msg) const {
            ssize_t total = 0, len = msg.size();
            const char *data = msg.data();
            while (total < len) {
                ssize_t sent = ::send(sock_fd_, data + total, len - total, 0);
                if (sent <= 0) {
                    throw std::runtime_error("CChannel: erro ao enviar dados");
                }
                total += sent;
            }
        }
    
        std::string recv_until(char delimiter = '\n') const {
            std::string result;
            while (true) {
                // Drain buffered data first
                if (read_pos_ < read_buffer_.size()) {
                    size_t delim_pos = read_buffer_.find(delimiter, read_pos_);
                    if (delim_pos != std::string::npos) {
                        result.append(read_buffer_, read_pos_, delim_pos - read_pos_);
                        read_pos_ = delim_pos + 1;
                        return result;
                    }
                    result.append(read_buffer_, read_pos_, read_buffer_.size() - read_pos_);
                    read_buffer_.clear();
                    read_pos_ = 0;
                }
                // Refill
                char block[4096];
                ssize_t r = ::recv(sock_fd_, block, sizeof(block), 0);
                if (r < 0) {
                    throw std::runtime_error("CChannel: erro ao receber dados");
                }
                if (r == 0) {
                    return result; // EOF: return what was accumulated
                }
                read_buffer_.append(block, static_cast<size_t>(r));
            }
        }

        const std::string& host() const noexcept { return host_; }
        uint16_t port()          const noexcept { return port_; }
        int      fd()            const noexcept { return sock_fd_; }
    
    private:
        int         sock_fd_;
        std::string host_;
        uint16_t    port_;
        mutable std::string read_buffer_;
        mutable size_t read_pos_ = 0;
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
        explicit DepthGuard(Interpreter &i) : interp(i) {
            if (++interp.recursion_depth > interp.max_recursion_depth) {
                throw RunTimeError("Recursão máxima excedida (" +
                    std::to_string(interp.max_recursion_depth) + ")");
            }
        }
        ~DepthGuard() { interp.recursion_depth--; }
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

#endif // INTERPRETER_CORE_HPP
