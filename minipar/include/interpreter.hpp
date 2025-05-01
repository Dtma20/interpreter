#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <map>
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

/**
 * @brief Wrapper para std::variant que representa valores em tempo de execução.
 *
 * Permite armazenar números (double), booleanos, strings ou vetores de ValueWrapper.
 * Os tipos são definidos na ordem: tipos primitivos primeiro, depois containers.
 */
struct ValueWrapper
{
    std::variant<std::monostate, double, bool, std::string, std::vector<ValueWrapper>> data;

    // Construtor padrão: inicializa como não inicializado
    ValueWrapper() : data(std::monostate{}) {}

    // Construtor para double
    ValueWrapper(double d) : data(d) {}

    // Construtor para bool
    ValueWrapper(bool b) : data(b) {}

    // Construtor para std::string
    ValueWrapper(const std::string &s) : data(s) {}

    // Construtor para const char* (opcional)
    ValueWrapper(const char *s) : data(std::string(s)) {}

    // Construtor para std::vector<ValueWrapper>
    ValueWrapper(const std::vector<ValueWrapper> &vec) : data(vec) {}

    // Método para verificar se está inicializado
    bool isInitialized() const
    {
        return !std::holds_alternative<std::monostate>(data);
    }

    // Operador de conversão para facilitar o uso (opcional)
    operator std::variant<std::monostate, double, bool, std::string, std::vector<ValueWrapper>>() const
    {
        return data;
    }
};

/// Um “ponteiro” para outro ValueWrapper, para
/// fazer binding por referência sem copiar o vetor.
struct ValueWrapperPtr {
    ValueWrapper *ref;  ///< endereço do ValueWrapper real

    // Construtores
    ValueWrapperPtr() : ref(nullptr) {}
    explicit ValueWrapperPtr(ValueWrapper *r) : ref(r) {}

    // Copy (mantém a referência)
    ValueWrapperPtr(const ValueWrapperPtr &o) : ref(o.ref) {}
    ValueWrapperPtr &operator=(const ValueWrapperPtr &o) {
        ref = o.ref;
        return *this;
    }

    // Encaminha acesso ao variant interno (sem cópia)
    std::variant<
        std::monostate,
        double,
        bool,
        std::string,
        std::vector<ValueWrapper>
    > &data() {
        return ref->data;
    }
    const std::variant<
        std::monostate,
        double,
        bool,
        std::string,
        std::vector<ValueWrapper>
    > &data() const {
        return ref->data;
    }

    // Encaminha demais métodos existentes em ValueWrapper
    bool isInitialized() const {
        return ref->isInitialized();
    }

    operator std::variant<
        std::monostate,
        double,
        bool,
        std::string,
        std::vector<ValueWrapper>
    >() const {
        return ref->data;
    }

    // Se em algum ponto seu interpretador precisar de um ValueWrapper&:
    operator ValueWrapper&() const {
        return *ref;
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
        std::map<std::string, std::shared_ptr<ValueWrapper>> variables;// Mapeamento de variáveis para seus valores.
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
