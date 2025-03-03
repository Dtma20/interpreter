#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <map>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <stack>
#include "../include/ast.hpp"
#include "../include/error.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/**
 * @brief Tipo variante para representar valores em tempo de execução.
 *
 * Pode armazenar um número (double), uma string ou um booleano.
 */
using Value = std::variant<double, std::string, bool>;

/**
 * @brief Classe Interpreter.
 *
 * Responsável pela execução do programa, avaliando expressões, 
 * executando statements e gerenciando escopos e funções.
 */
class Interpreter {
private:
    /**
     * @brief Estrutura para representar um escopo.
     *
     * Um escopo contém um mapeamento entre nomes de variáveis e seus valores.
     */
    struct Scope {
        std::map<std::string, Value> variables; ///< Mapeamento de variáveis para seus valores.
    };

    std::vector<Scope> scopes;               ///< Pilha de escopos para gerenciar variáveis.
    std::map<std::string, FuncDef*> functions; ///< Tabela de funções definidas pelo usuário.
    bool break_flag;                         ///< Flag para controle de instrução break.
    bool continue_flag;                      ///< Flag para controle de instrução continue.
    bool return_flag;                        ///< Flag para controle de retorno de função.
    Value return_value;                      ///< Valor de retorno de uma função.

    /**
     * @brief Avalia uma expressão da AST e retorna seu valor.
     *
     * @param expr Ponteiro para a expressão a ser avaliada.
     * @return Valor resultante da avaliação.
     */
    Value evaluate(Expression* expr);

    /**
     * @brief Executa um statement da AST.
     *
     * Analisa o nó e executa a operação correspondente.
     *
     * @param stmt Ponteiro para o statement a ser executado.
     */
    void execute_stmt(Node* stmt);

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
    Value execute_function(FuncDef* func, const Arguments& args);

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
     * Define a veracidade de um Value baseado no seu tipo.
     *
     * @param value Valor a ser avaliado.
     * @return true se o valor for verdadeiro; false caso contrário.
     */
    bool is_true(const Value& value);

    /**
     * @brief Executa um servidor para um canal de serviço (SChannel).
     *
     * Configura e executa um servidor TCP para comunicação através de um SChannel.
     *
     * @param schannel Ponteiro para o SChannel a ser executado.
     */
    void run_server(SChannel* schannel);

    /**
     * @brief Converte um Value para sua representação em string.
     *
     * Utiliza std::visit para converter o valor de cada tipo armazenado na variante.
     *
     * @param value Valor a ser convertido.
     * @return Representação em string do valor.
     */
    std::string convert_value_to_string(const Value& value);

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
    void execute(Module* module);
};

#endif // INTERPRETER_HPP
