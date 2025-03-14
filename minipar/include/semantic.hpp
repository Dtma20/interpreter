#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include "ast.hpp"
#include "error.hpp"
#include "token.hpp"

/**
 * @brief Interface para o analisador semântico.
 *
 * Define os métodos básicos que qualquer implementação de análise semântica deve possuir.
 */
class ISemanticAnalyzer
{
public:
    virtual ~ISemanticAnalyzer() = default;

    /**
     * @brief Visita um nó da AST e realiza a análise semântica correspondente.
     *
     * @param node Ponteiro para o nó da AST a ser analisado.
     */
    virtual void visit(Node *node) = 0;

    /**
     * @brief Método genérico para visitar nós não explicitamente tratados.
     *
     * @param node Ponteiro para o nó da AST.
     */
    virtual void generic_visit(Node *node) = 0;
};

/**
 * @brief Classe responsável pela análise semântica do código fonte.
 *
 * Verifica a coerência dos tipos de dados, escopos de variáveis e declarações de funções,
 * garantindo que o código seja semanticamente válido.
 */
class SemanticAnalyzer : public ISemanticAnalyzer
{
private:
    std::vector<Node *> context_stack;                         // Pilha de contexto para rastrear escopos e blocos.
    std::unordered_map<std::string, FuncDef *> function_table; // Tabela de funções definidas no código.
    std::vector<std::string> default_func_names;               // Lista de funções embutidas (built-in).
    std::vector<std::unordered_map<std::string, std::string>> scope_stack; // Pilha de escopos
    /**
     * @brief Avalia um nó da AST e retorna seu tipo como string.
     *
     * @param node Ponteiro para o nó a ser avaliado.
     * @return O tipo do nó como string, se aplicável.
     */
    std::string evaluate(Node *node) const;

public:
    /**
     * @brief Construtor do analisador semântico.
     */
    SemanticAnalyzer();

    /**
     * @brief Método genérico para visitar nós não explicitamente tratados.
     *
     * @param node Ponteiro para o nó da AST.
     */
    void generic_visit(Node *node) override;

    /**
     * @brief Visita um nó da AST e realiza a análise semântica correspondente.
     *
     * @param node Ponteiro para o nó da AST a ser analisado.
     */
    void visit(Node *node) override;

    /**
     * @brief Analisa atribuições de valores a variáveis.
     *
     * @param node Ponteiro para o nó de atribuição.
     */
    void visit_Assign(Assign *node);

    /**
     * @brief Analisa instruções de retorno de função.
     *
     * @param node Ponteiro para o nó de retorno.
     */
    void visit_Return(Return *node);

    /**
     * @brief Analisa a instrução `break`, verificando se está em um laço válido.
     *
     * @param node Ponteiro para o nó `break`.
     */
    void visit_Break(Break *node);

    /**
     * @brief Analisa a instrução `continue`, verificando se está em um laço válido.
     *
     * @param node Ponteiro para o nó `continue`.
     */
    void visit_Continue(Continue *node);

    /**
     * @brief Analisa definições de funções e armazena-as na tabela de símbolos.
     *
     * @param node Ponteiro para o nó de definição de função.
     */
    void visit_FuncDef(FuncDef *node);

    /**
     * @brief Analisa instruções condicionais `if`.
     *
     * @param node Ponteiro para o nó `if`.
     */
    void visit_If(If *node);

    /**
     * @brief Analisa laços `while`.
     *
     * @param node Ponteiro para o nó `while`.
     */
    void visit_While(While *node);

    /**
     * @brief Analisa operações paralelas dentro do código.
     *
     * @param node Ponteiro para o nó `par`.
     */
    void visit_Par(Par *node);

    /**
     * @brief Analisa criação de canais de comunicação entre processos.
     *
     * @param node Ponteiro para o nó `CChannel`.
     */
    void visit_CChannel(CChannel *node);

    /**
     * @brief Analisa comunicação via canais.
     *
     * @param node Ponteiro para o nó `SChannel`.
     */
    void visit_SChannel(SChannel *node);

    /**
     * @brief Analisa constantes no código e retorna seu tipo, se possível.
     *
     * @param node Ponteiro para o nó da constante.
     * @return Tipo da constante, se aplicável.
     */
    std::optional<std::string> visit_Constant(const Constant *node) const;

    /**
     * @brief Analisa identificadores de variáveis e verifica sua existência.
     *
     * @param node Ponteiro para o nó identificador.
     * @return Tipo da variável, se encontrada.
     */
    std::optional<std::string> visit_ID(const ID *node) const;

    /**
     * @brief Analisa acessos a variáveis ou estruturas de dados.
     *
     * @param node Ponteiro para o nó de acesso.
     * @return Tipo do valor acessado, se aplicável.
     */
    std::optional<std::string> visit_Access(const Access *node) const;

    /**
     * @brief Analisa operações lógicas (`&&`, `||`).
     *
     * @param node Ponteiro para o nó lógico.
     * @return Tipo do resultado da operação.
     */
    std::optional<std::string> visit_Logical(const Logical *node) const;

    /**
     * @brief Analisa operações relacionais (`<`, `>`, `==`, `!=`).
     *
     * @param node Ponteiro para o nó relacional.
     * @return Tipo do resultado da comparação.
     */
    std::optional<std::string> visit_Relational(const Relational *node) const;

    /**
     * @brief Analisa operações aritméticas (`+`, `-`, `*`, `/`).
     *
     * @param node Ponteiro para o nó aritmético.
     * @return Tipo do resultado da operação.
     */
    std::optional<std::string> visit_Arithmetic(const Arithmetic *node) const;

    /**
     * @brief Analisa operações unárias (`-`, `!`).
     *
     * @param node Ponteiro para o nó unário.
     * @return Tipo do resultado da operação.
     */
    std::optional<std::string> visit_Unary(const Unary *node) const;

    /**
     * @brief Analisa chamadas de função e verifica seus argumentos.
     *
     * @param node Ponteiro para o nó de chamada de função.
     * @return Tipo do valor retornado pela função, se conhecido.
     */
    std::optional<std::string> visit_Call(const Call *node) const;

    /**
     * @brief Analisa um bloco de código, verificando cada instrução dentro dele.
     *
     * @param block Vetor contendo os nós das instruções do bloco.
     */
    void visit_block(const Body &block);

    std::optional<std::string> visit_Array(const Array *node) const;
};

#endif // SEMANTIC_HPP
