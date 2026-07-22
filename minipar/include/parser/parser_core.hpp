#ifndef PARSER_CORE_HPP
#define PARSER_CORE_HPP

#pragma once
#include <memory>
#include <vector>
#include <unordered_set>
#include "../ast.hpp"
#include "../lexer.hpp"
#include "../symtable.hpp"
#include "../error.hpp"
#include "../token.hpp"

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
     * @param tag A tag do token esperado.
     * @return true se o token corresponder, false caso contrário.
     */
    virtual bool match(const std::string &tag) = 0;

    /**
     * @brief Inicia o processo de análise sintática e retorna o módulo principal.
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
     * @param tokens Lista de tokens com suas respectivas linhas extraídas pelo lexer.
     */
    Parser(std::vector<std::pair<Token, int>> tokens);

    /**
     * @brief Verifica se o token atual corresponde a uma determinada tag.
     * @param tag A tag do token esperado.
     * @return true se o token corresponder, false caso contrário.
     */
    bool match(const std::string &tag) override;

    /**
     * @brief Inicia o processo de análise sintática e retorna o módulo principal.
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

    std::unique_ptr<Module> program();
    Body stmts();
    std::unique_ptr<Node> stmt();
    Body block(const Parameters &params = {});
    Parameters params();
    std::pair<std::string, std::pair<std::string, std::unique_ptr<Expression>>> param();
    Arguments args();
    void skipWhitespace();
    Token peekNext();

    // Expressões
    std::unique_ptr<Expression> disjunction();
    std::unique_ptr<Expression> conjunction();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> arithmetic();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> primary();
    std::unique_ptr<Expression> local();

    std::string var(const std::string &id_type);

    // Métodos auxiliares para modularização do stmt()
    std::unique_ptr<Node> stmtUnaryPrefix();
    std::unique_ptr<Node> stmtId();
    std::unique_ptr<Node> stmtFunc();
    std::unique_ptr<Node> stmtIf();
    std::unique_ptr<Node> stmtWhile();
    std::unique_ptr<Node> stmtReturn();
    std::unique_ptr<Node> stmtBreak();
    std::unique_ptr<Node> stmtContinue();
    std::unique_ptr<Node> stmtSeq();
    std::unique_ptr<Node> stmtPar();
    std::unique_ptr<Node> stmtCChannel();
    std::unique_ptr<Node> stmtSChannel();
    std::unique_ptr<Node> stmtFor();

    // Métodos auxiliares para o processamento de statements que iniciam com ID
    std::unique_ptr<Node> processArrayAccessStmt(const std::string &id_name);
    std::unique_ptr<Node> processPostfixUnaryStmt(const std::string &id_name);
    std::unique_ptr<Node> processTypeDeclarationStmt(const std::string &id_name);
    std::unique_ptr<Node> processSimpleAssignStmt(const std::string &id_name);
    std::unique_ptr<Node> processFunctionCallStmt(const std::string &id_name);
};

#endif // PARSER_CORE_HPP
