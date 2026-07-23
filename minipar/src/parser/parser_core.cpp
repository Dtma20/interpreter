/**
 * @file parser_core.cpp
 * @brief Implementação dos métodos fundamentais do Parser.
 *
 * Contém: construtor, match(), start(), program(), stmts(), block(),
 * params(), param(), args(), peekNext().
 */

#include "../../include/parser/parser_core.hpp"
#include "../../include/debug.hpp"
#include "../../include/error.hpp"
#include "../../include/symtable.hpp"
#include "../../include/ast/ast_statements.hpp"
#include <stdexcept>
#include <iostream>

/**
 * @brief Destrutor do Parser (definido aqui para shared_ptr<SymTable> com tipo incompleto no header).
 */
Parser::~Parser() = default;

/**
 * @brief Construtor do Parser.
 *
 * Inicializa o parser com a lista de tokens e configura o ponteiro de posição (pos),
 * o token de lookahead (próximo token) e o número da linha (lineno). Também cria uma
 * tabela de símbolos compartilhada e insere as funções padrão na mesma.
 *
 * @param tokens Vetor de pares contendo os tokens e os respectivos números de linha.
 */
Parser::Parser(std::vector<std::pair<Token, int>> tokens)
    : tokens(std::move(tokens)), pos(0), symtable(std::make_shared<SymTable>())
{
    if (!this->tokens.empty())
    {
        lookahead = this->tokens[pos].first;
        lineno = this->tokens[pos].second;
    }
    else
    {
        lookahead = Token("EOF", "EOF");
        lineno = 1;
    }
    for (const auto &[name, type] : DEFAULT_FUNCTION_NAMES)
    {
        symtable->insert(name, Symbol(name, "FUNC"));
    }
}

/**
 * @brief Tenta casar (match) o token atual com um token esperado.
 */
bool Parser::match(const std::string &tag)
{
    if (tag == lookahead.getTag())
    {
        pos++;
        if (pos < tokens.size())
        {
            lookahead = tokens[pos].first;
            lineno = tokens[pos].second;
        }
        else
        {
            lookahead = Token("EOF", "EOF");
        }
        return true;
    }
    return false;
}

/**
 * @brief Inicia o processo de parsing.
 */
std::unique_ptr<Module> Parser::start()
{
    return program();
}

/**
 * @brief Analisa o programa completo.
 */
std::unique_ptr<Module> Parser::program()
{
    auto body = std::make_unique<Body>(std::move(stmts()));
    auto seq = std::make_unique<Seq>(std::move(body), false);
    return std::make_unique<Module>(std::move(seq));
}

/**
 * @brief Analisa uma sequência de statements.
 */
Body Parser::stmts()
{
    Body stmts;
    while (lookahead.getTag() != "EOF")
    {
        if (lookahead.getTag() != "EOF")
        {
            stmts.push_back(stmt());
        }
    }
    return stmts;
}

/**
 * @brief Retorna o próximo token sem avançar a posição atual.
 */
Token Parser::peekNext()
{
    if (pos + 1 < tokens.size())
    {
        return tokens[pos + 1].first;
    }
    return Token("EOF", "EOF");
}

/**
 * @brief Analisa um bloco de código, como o corpo de funções e laços.
 */
Body Parser::block(const Parameters &params)
{
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    return body;
}

/**
 * @brief Analisa a lista de parâmetros de uma função.
 */
Parameters Parser::params()
{
    Parameters parameters;
    if (!match("LPAREN"))
        throw SyntaxError(lineno, "Esperado '(' para lista de parâmetros, mas encontrado " + lookahead.getValue());
    bool hasOptional = false;
    if (lookahead.getValue() != ")")
    {
        auto p = param();
        hasOptional = (p.second.second != nullptr);
        parameters.push_back(std::move(p));
    }
    while (lookahead.getTag() == ",")
    {
        match(",");
        auto p = param();
        if (hasOptional && p.second.second == nullptr)
            throw SyntaxError(lineno, "Parâmetro obrigatório '" + p.first + "' não pode vir após parâmetro opcional");
        hasOptional = hasOptional || (p.second.second != nullptr);
        parameters.push_back(std::move(p));
    }
    if (!match("RPAREN"))
        throw SyntaxError(lineno, "Esperado ')' após parâmetros, mas encontrado " + lookahead.getValue());
    return parameters;
}

/**
 * @brief Analisa um parâmetro individual.
 */
std::pair<std::string, std::pair<std::string, std::unique_ptr<Expression>>> Parser::param()
{
    std::string name = lookahead.getValue();
    if (!match("ID"))
    {
        throw SyntaxError(lineno, "nome " + name + " inválido para um parâmetro");
    }
    if (!match("COLON"))
    {
        throw SyntaxError(lineno, "esperado : no lugar de " + lookahead.getValue());
    }
    std::string type = lookahead.getValue();
    if (!match("TYPE"))
    {
        throw SyntaxError(lineno, "esperado um tipo no lugar de " + lookahead.getValue());
    }
    std::unique_ptr<Expression> default_value = nullptr;
    if (lookahead.getTag() == "ASSIGN")
    {
        match("ASSIGN");
        default_value = disjunction();
    }
    return std::make_pair(name, std::make_pair(type, std::move(default_value)));
}

/**
 * @brief Analisa uma lista de argumentos passados para uma função.
 */
Arguments Parser::args()
{
    Arguments args;
    if (lookahead.getTag() != "RPAREN")
    {
        args.push_back(disjunction());
        while (lookahead.getTag() == ",")
        {
            match(",");
            args.push_back(disjunction());
        }
    }
    return args;
}

/**
 * @brief Retorna o valor de um identificador em um contexto específico.
 */
std::string Parser::var(const std::string &context)
{
    if (lookahead.getTag() != "ID")
    {
        throw SyntaxError(lineno, "Esperado identificador após '" + context + "' em lugar de " + lookahead.getValue());
    }
    std::string name = lookahead.getValue();
    match("ID");
    return name;
}
