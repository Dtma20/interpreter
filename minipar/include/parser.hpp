#ifndef PARSER_HPP
#define PARSER_HPP

#pragma once
#include <memory>
#include <vector>
#include <unordered_set>
#include "ast.hpp"
#include "lexer.hpp"
#include "symtable.hpp"
#include "error.hpp"
#include "token.hpp"

/**
 * @brief Interface para um analisador sintático (parser).
 *
 * Define os métodos básicos que qualquer implementação de parser deve possuir.
 */
class IParser
{
public:
    virtual ~IParser() = default;

    /**
     * @brief Verifica se o token atual corresponde a uma determinada tag.
     *
     * @param tag A tag do token esperado.
     * @return true se o token corresponder, false caso contrário.
     */
    virtual bool match(const std::string &tag) = 0;

    /**
     * @brief Inicia o processo de análise sintática e retorna o módulo principal.
     *
     * @return Um ponteiro único para o módulo raiz da AST.
     */
    virtual std::unique_ptr<Module> start() = 0;
};

/**
 * @brief Classe responsável pela análise sintática do código fonte.
 *
 * Transforma a sequência de tokens gerada pelo lexer em uma AST (Abstract Syntax Tree).
 */
class Parser : public IParser
{
public:
    /**
     * @brief Construtor da classe Parser.
     *
     * @param tokens Lista de tokens com suas respectivas linhas extraídas pelo lexer.
     */
    Parser(std::vector<std::pair<Token, int>> tokens);

    /**
     * @brief Verifica se o token atual corresponde a uma determinada tag.
     *
     * @param tag A tag do token esperado.
     * @return true se o token corresponder, false caso contrário.
     */
    bool match(const std::string &tag) override;

    /**
     * @brief Inicia o processo de análise sintática e retorna o módulo principal.
     *
     * @return Um ponteiro único para o módulo raiz da AST.
     */
    std::unique_ptr<Module> start() override;

private:
    std::vector<std::pair<Token, int>> tokens; ///< Lista de tokens extraídos do código fonte.
    size_t pos;                                ///< Posição atual no vetor de tokens.
    Token lookahead;                           ///< Token atualmente sendo analisado.
    int lineno;                                ///< Número da linha do token atual.
    std::shared_ptr<SymTable> symtable;        ///< Tabela de símbolos para gerenciamento de variáveis e funções.

    // Métodos principais de análise sintática

    /**
     * @brief Analisa o programa e retorna o módulo principal.
     *
     * @return Um ponteiro único para o módulo AST.
     */
    std::unique_ptr<Module> program();

    /**
     * @brief Analisa uma sequência de statements e os agrupa.
     *
     * @return Um vetor contendo os statements.
     */
    Body stmts();

    /**
     * @brief Analisa um único statement.
     *
     * @return Um ponteiro único para um nó AST representando o statement.
     */
    std::unique_ptr<Node> stmt();

    /**
     * @brief Analisa um bloco de código, como o corpo de funções e laços.
     *
     * @param params Parâmetros locais do bloco, se aplicável.
     * @return Um vetor contendo os statements do bloco.
     */
    Body block(const Parameters &params = {});

    /**
     * @brief Analisa uma lista de parâmetros de uma função.
     *
     * @return Um mapa contendo os parâmetros e seus valores padrões (se houver).
     */
    Parameters params();

    /**
     * @brief Analisa um único parâmetro da lista de parâmetros.
     *
     * @return Um par representando o nome do parâmetro, seu tipo e valor padrão.
     */
    std::pair<std::string, std::pair<std::string, std::unique_ptr<Expression>>> param();

    /**
     * @brief Analisa uma lista de argumentos passados para uma função.
     *
     * @return Um vetor contendo as expressões dos argumentos.
     */
    Arguments args();

    /**
     * @brief Pula tokens de espaço em branco e atualiza o lookahead.
     */
    void skipWhitespace();

    /**
     * @brief Obtém o próximo token sem consumi-lo.
     *
     * @return O próximo token na sequência.
     */
    Token peekNext();

    /**
     * @brief Analisa uma expressão lógica de disjunção (or).
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> disjunction();

    /**
     * @brief Analisa uma expressão lógica de conjunção (and).
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> conjunction();

    /**
     * @brief Analisa uma expressão de igualdade (== e !=).
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> equality();

    /**
     * @brief Analisa uma expressão relacional (<, >, <=, >=).
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> comparison();

    /**
     * @brief Analisa uma expressão aritmética de soma e subtração.
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> arithmetic();

    /**
     * @brief Analisa uma expressão aritmética de multiplicação e divisão.
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> term();

    /**
     * @brief Analisa uma expressão unária (-, !).
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> unary();

    /**
     * @brief Analisa uma expressão primária (literais, identificadores e chamadas de função).
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> primary();

    /**
     * @brief Analisa a expressão local dentro de um escopo.
     *
     * @return Um ponteiro único para a expressão AST correspondente.
     */
    std::unique_ptr<Expression> local();

    /**
     * @brief Obtém o nome de uma variável com base em seu tipo.
     *
     * @param id_type O tipo da variável.
     * @return O nome da variável.
     */
    std::string var(const std::string &id_type);

    // Métodos auxiliares para modularização do stmt()

    /**
     * @brief Processa um statement iniciando com operador unário pré-fixado (++ ou --).
     *
     * @return Um ponteiro único para o nó AST representando o operador unário pré-fixado.
     */
    std::unique_ptr<Node> stmtUnaryPrefix();

    /**
     * @brief Processa um statement que inicia com um identificador (ID).
     *
     * Analisa os diferentes casos quando um statement inicia com ID, como acesso a índices, 
     * operadores pós-fixados, declarações com tipo, atribuições simples e chamadas de função.
     *
     * @return Um ponteiro único para o nó AST correspondente.
     */
    std::unique_ptr<Node> stmtId();

    /**
     * @brief Processa a definição de função.
     *
     * @return Um ponteiro único para o nó AST representando a definição da função.
     */
    std::unique_ptr<Node> stmtFunc();

    /**
     * @brief Processa a estrutura condicional IF-ELSE.
     *
     * @return Um ponteiro único para o nó AST representando a estrutura condicional.
     */
    std::unique_ptr<Node> stmtIf();

    /**
     * @brief Processa o laço WHILE.
     *
     * @return Um ponteiro único para o nó AST representando o laço WHILE.
     */
    std::unique_ptr<Node> stmtWhile();

    /**
     * @brief Processa o comando RETURN.
     *
     * @return Um ponteiro único para o nó AST representando o comando RETURN.
     */
    std::unique_ptr<Node> stmtReturn();

    /**
     * @brief Processa o comando BREAK.
     *
     * @return Um ponteiro único para o nó AST representando o comando BREAK.
     */
    std::unique_ptr<Node> stmtBreak();

    /**
     * @brief Processa o comando CONTINUE.
     *
     * @return Um ponteiro único para o nó AST representando o comando CONTINUE.
     */
    std::unique_ptr<Node> stmtContinue();

    /**
     * @brief Processa uma sequência de instruções (SEQ).
     *
     * @return Um ponteiro único para o nó AST representando a sequência de instruções.
     */
    std::unique_ptr<Node> stmtSeq();

    /**
     * @brief Processa a execução paralela (PAR).
     *
     * @return Um ponteiro único para o nó AST representando a execução paralela.
     */
    std::unique_ptr<Node> stmtPar();

    /**
     * @brief Processa a declaração de canal cliente (C_CHANNEL).
     *
     * @return Um ponteiro único para o nó AST representando o canal cliente.
     */
    std::unique_ptr<Node> stmtCChannel();

    /**
     * @brief Processa a declaração de canal servidor (S_CHANNEL).
     *
     * @return Um ponteiro único para o nó AST representando o canal servidor.
     */
    std::unique_ptr<Node> stmtSChannel();

    /**
     * @brief Processa o laço FOR, reescrevendo-o como uma sequência com inicialização e laço WHILE.
     *
     * @return Um ponteiro único para o nó AST representando a estrutura FOR.
     */
    std::unique_ptr<Node> stmtFor();

    // Métodos auxiliares para o processamento de statements que iniciam com ID

    /**
     * @brief Processa o acesso a índices (array) e suas variações, como atribuição e pós-incremento/decremento.
     *
     * @param id_name Nome do identificador.
     * @return Um ponteiro único para o nó AST representando o acesso ao array.
     */
    std::unique_ptr<Node> processArrayAccessStmt(const std::string &id_name);

    /**
     * @brief Processa operadores unários pós-fixados (ex.: x++ ou x--).
     *
     * @param id_name Nome do identificador.
     * @return Um ponteiro único para o nó AST representando o operador unário pós-fixado.
     */
    std::unique_ptr<Node> processPostfixUnaryStmt(const std::string &id_name);

    /**
     * @brief Processa a declaração com tipo para identificadores.
     *
     * Trata declarações do tipo <ID> : <TYPE> (= ... ou array[...] ...)
     *
     * @param id_name Nome do identificador.
     * @return Um ponteiro único para o nó AST representando a declaração com tipo.
     */
    std::unique_ptr<Node> processTypeDeclarationStmt(const std::string &id_name);

    /**
     * @brief Processa uma atribuição simples (ex.: x = ...).
     *
     * @param id_name Nome do identificador.
     * @return Um ponteiro único para o nó AST representando a atribuição.
     */
    std::unique_ptr<Node> processSimpleAssignStmt(const std::string &id_name);

    /**
     * @brief Processa a chamada de função (ex.: func(...)).
     *
     * @param id_name Nome do identificador que também é usado para a chamada da função.
     * @return Um ponteiro único para o nó AST representando a chamada de função.
     */
    std::unique_ptr<Node> processFunctionCallStmt(const std::string &id_name);
};

#endif // PARSER_HPP
