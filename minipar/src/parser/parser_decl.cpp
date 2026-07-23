/**
 * @file parser_decl.cpp
 * @brief Implementação dos métodos de parsing de declarações.
 *
 * Contém: stmtFunc(), processTypeDeclarationStmt().
 */

#include "../../include/parser/parser_core.hpp"
#include "../../include/debug.hpp"
#include "../../include/error.hpp"
#include "../../include/symtable.hpp"
#include "../../include/ast.hpp"
#include <stdexcept>
#include <iostream>

// Processa definição de função
std::unique_ptr<Node> Parser::stmtFunc()
{
    LOG_DEBUG("Parser: Detectado FUNC, processando...");
    match("FUNC");
    std::string name = var("FUNC");
    Parameters params = this->params();
    if (!match("RARROW"))
        throw SyntaxError(lineno, "Esperado '->' no lugar de " + lookahead.getValue());
    std::string type = lookahead.getValue();
    if (!match("TYPE"))
        throw SyntaxError(lineno, "Tipo de retorno inválido: " + lookahead.getValue());
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
    auto func_def = std::make_unique<FuncDef>(name, type, std::move(params),
                                               std::make_unique<Body>(std::move(body)));
    symtable->insert(name, Symbol(name, "FUNC"));
    return func_def;
}

// Processa declaração com tipo, seja para array ou para declaração simples com atribuição
std::unique_ptr<Node> Parser::processTypeDeclarationStmt(const std::string &id_name)
{
    match("COLON");
    if (lookahead.getValue() == "array")
    {
        match("TYPE");

        std::vector<std::unique_ptr<Expression>> dimensions;
        while (lookahead.getTag() == "LBRACK")
        {
            match("LBRACK");
            auto size_expr = disjunction();
            if (!match("RBRACK"))
                throw SyntaxError(lineno, "Esperado ']' após expressão de tamanho");
            dimensions.push_back(std::move(size_expr));
        }

        if (dimensions.empty())
            throw SyntaxError(lineno, "Esperado pelo menos uma dimensão após 'array'");

        if (lookahead.getTag() == "ASSIGN")
        {
            match("ASSIGN");
            auto right = arithmetic();
            auto array_decl = std::make_unique<ArrayDecl>(id_name, std::move(dimensions));
            auto assign = std::make_unique<Assign>(
                std::make_unique<ID>("ID", Token("ID", id_name)),
                std::move(right));
            Body seq_body;
            seq_body.push_back(std::move(array_decl));
            seq_body.push_back(std::move(assign));
            return std::make_unique<Seq>(std::make_unique<Body>(std::move(seq_body)));
        }
        return std::make_unique<ArrayDecl>(id_name, std::move(dimensions));
    }
    else
    {
        std::string type = lookahead.getValue();
        if (!match("TYPE"))
            throw SyntaxError(lineno, "Esperado um tipo após ':' em lugar de " + lookahead.getValue());
        if (!match("ASSIGN"))
            throw SyntaxError(lineno, "Esperado '=' após tipo em lugar de " + lookahead.getValue());
        auto right = arithmetic();
        auto id = std::make_unique<ID>(type, Token("ID", id_name), true);
        return std::make_unique<Assign>(std::move(id), std::move(right));
    }
}
