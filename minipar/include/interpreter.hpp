#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <stack>
#include <variant>
#include "../include/ast.hpp"
#include "../include/error.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h> 

static std::mutex cout_mutex;

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
            std::string buffer;
            char ch;
            while (true) {
                ssize_t r = ::recv(sock_fd_, &ch, 1, 0);
                if (r <= 0 || ch == delimiter)
                    break;
                buffer.push_back(ch);
            }
            return buffer;
        }

        const std::string& host() const noexcept { return host_; }
        uint16_t port()          const noexcept { return port_; }
        int      fd()            const noexcept { return sock_fd_; }
    
    private:
        int         sock_fd_;
        std::string host_;
        uint16_t    port_;
    };

/**
 * @brief Wrapper para std::variant que representa valores em tempo de execução.
 *
 * Permite armazenar números (double), booleanos, strings ou vetores de ValueWrapper.
 * Os tipos são definidos na ordem: tipos primitivos primeiro, depois containers.
 */
struct ValueWrapper
{
    
    std::variant<std::monostate, double, bool, std::string, std::vector<ValueWrapper>, std::shared_ptr<CChannelValue>> data;

    ValueWrapper() : data(std::monostate{}) {}

    ValueWrapper(double d) : data(d) {}

    ValueWrapper(bool b) : data(b) {}

    ValueWrapper(const std::string &s) : data(s) {}

    ValueWrapper(const char *s) : data(std::string(s)) {}

    ValueWrapper(const std::vector<ValueWrapper> &vec) : data(vec) {}

    ValueWrapper(std::shared_ptr<CChannelValue> cch) : data(cch) {}

    bool isInitialized() const
    {
        return !std::holds_alternative<std::monostate>(data);
    }

    operator std::variant<std::monostate, double, bool, std::string, std::vector<ValueWrapper>, std::shared_ptr<CChannelValue>>() const
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
    /**
     * @brief Estrutura para representar um escopo.
     *
     * Um escopo contém um mapeamento entre nomes de variáveis e seus valores.
     */
    struct Scope
    {
        std::map<std::string, std::shared_ptr<ValueWrapper>> variables; // Mapeamento de variáveis para seus valores.
    };

    std::vector<Scope> scopes;                  // Pilha de escopos para gerenciar variáveis.
    std::map<std::string, FuncDef *> functions; // Tabela de funções definidas pelo usuário.
    bool break_flag;                            // Flag para controle de instrução break.
    bool continue_flag;                         // Flag para controle de instrução continue.
    bool return_flag;                           // Flag para controle de retorno de função.
    ValueWrapper return_value;                  // Valor de retorno de uma função.

    /**
     * @brief Avalia uma expressão da AST e retorna seu valor.
     *
     * @param expr Ponteiro para a expressão a ser avaliada.
     * @return Valor resultante da avaliação.
     */
    ValueWrapper evaluate(Expression *expr);

    /**
     * @brief Executa um statement da AST.
     *
     * Analisa o nó e executa a operação correspondente.
     *
     * @param stmt Ponteiro para o statement a ser executado.
     */
    void execute_stmt(Node *stmt);

    /**
     * @brief Executa uma função definida pelo usuário.
     *
     * Cria um novo escopo, associa os argumentos aos parâmetros, executa o corpo da função
     * e retorna o valor de retorno.
     *
     * @param func Ponteiro para a definição da função.
     * @param args Lista de argumentos passados para a função.
     * @return Valor retornado pela função.
     */
    ValueWrapper execute_function(FuncDef *func, const Arguments &args);

    /**
     * @brief Cria um novo escopo.
     *
     * Empilha um novo escopo na pilha de escopos para isolar variáveis.
     */
    void push_scope();

    /**
     * @brief Remove o escopo atual.
     *
     * Desempilha o escopo atual da pilha de escopos.
     */
    void pop_scope();

    /**
     * @brief Verifica se um valor é considerado verdadeiro.
     *
     * Define a veracidade de um ValueWrapper baseado no seu tipo.
     *
     * @param value Valor a ser avaliado.
     * @return true se o valor for verdadeiro; false caso contrário.
     */
    bool is_true(const ValueWrapper &value);

    /**
     * @brief Executa um servidor para um canal de serviço (SChannel).
     *
     * Configura e executa um servidor TCP para comunicação através de um SChannel.
     *
     * @param schannel Ponteiro para o SChannel a ser executado.
     */
    void run_server(SChannel *schannel);

    /**
     * @brief Executa um cliente para um canal de cliente (CChannel).
     *
     * Estabelece uma conexão TCP com o servidor e envia mensagens
     * para o servidor.
     *
     * @param cchannel Ponteiro para o CChannel a ser executado.
     */
    void run_client(CChannel *cchannel);

    /**
     * @brief Converte um ValueWrapper para sua representação em string.
     *
     * Utiliza std::visit para converter o valor de cada tipo armazenado na variante.
     *
     * @param value Valor a ser convertido.
     * @return Representação em string do valor.
     */
    std::string convert_value_to_string(const ValueWrapper &value);

    ValueWrapper evaluateConstant(Constant *constant);
    ValueWrapper evaluateID(ID *id);
    ValueWrapper evaluateArray(Array *array);
    ValueWrapper evaluateAccess(Access *access);
    ValueWrapper evaluateFunctionCall(Call *call);
    ValueWrapper evaluateRelational(Relational *relational);
    ValueWrapper evaluateArithmetic(Arithmetic *arithmetic);
    ValueWrapper evaluateUnary(Unary *unary);
    ValueWrapper evaluateLogical(Logical *logical);

public:
    /**
     * @brief Construtor do Interpreter.
     *
     * Inicializa as flags de controle, a pilha de escopos e a tabela de funções.
     */
    Interpreter();

    /**
     * @brief Executa o módulo (programa) representado pela AST.
     *
     * Itera sobre os statements do módulo e os executa sequencialmente.
     *
     * @param module Ponteiro para o módulo a ser executado.
     */
    void execute(Module *module);
};

#endif // INTERPRETER_HPP
