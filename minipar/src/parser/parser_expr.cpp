/**
 * @file parser_expr.cpp
 * @brief Implementação dos métodos de parsing de expressões.
 *
 * Contém: disjunction(), conjunction(), equality(), comparison(),
 * arithmetic(), term(), unary(), primary(), local().
 */

#include "../../include/parser/parser_core.hpp"
#include "../../include/debug.hpp"
#include "../../include/error.hpp"
#include "../../include/ast.hpp"
#include <stdexcept>

/**
 * @brief Analisa uma expressão de disjunção (operador OR).
 */
std::unique_ptr<Expression> Parser::disjunction()
{
    auto left = conjunction();
    while (lookahead.getTag() == "OR")
    {
        const int line = lineno;
        Token token = lookahead;
        match("OR");
        auto right = conjunction();
        left = withLine(std::make_unique<Logical>("BOOL", token, std::move(left), std::move(right)), line);
    }
    return left;
}

/**
 * @brief Analisa uma expressão de conjunção (operador AND).
 */
std::unique_ptr<Expression> Parser::conjunction()
{
    auto left = equality();
    while (lookahead.getTag() == "AND")
    {
        const int line = lineno;
        Token token = lookahead;
        match("AND");
        auto right = equality();
        left = withLine(std::make_unique<Logical>("BOOL", token, std::move(left), std::move(right)), line);
    }
    return left;
}

/**
 * @brief Analisa uma expressão que representa uma variável ou chamada de função.
 */
std::unique_ptr<Expression> Parser::local()
{
    const int line = lineno;
    std::string name = lookahead.getValue();
    match("ID");

    if (lookahead.getTag() == "LPAREN")
    {
        match("LPAREN");
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
        if (!match("RPAREN"))
        {
            throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
        }
        auto base = withLine(std::make_unique<ID>("", Token("ID", name)), line);
        return withLine(std::make_unique<Call>("", Token("ID", name),
                                               std::move(base),
                                               std::move(args), name),
                        line);
    }

    if (match("COLON"))
    {
        std::string type = lookahead.getValue();
        if (!match("TYPE"))
        {
            throw SyntaxError(lineno, "Esperado um tipo após ':' em lugar de " + lookahead.getValue());
        }
        return withLine(std::make_unique<ID>(type, Token("ID", name), true), line);
    }

    return withLine(std::make_unique<ID>("", Token("ID", name)), line);
}

/**
 * @brief Analisa uma expressão de igualdade (==, !=).
 */
std::unique_ptr<Expression> Parser::equality()
{
    auto left = comparison();
    while (lookahead.getTag() == "EQ" || lookahead.getTag() == "NEQ")
    {
        const int line = lineno;
        Token token = lookahead;
        if (match("EQ") || match("NEQ"))
        {
            auto right = comparison();
            left = withLine(std::make_unique<Relational>("BOOL", token, std::move(left), std::move(right)), line);
        }
    }
    return left;
}

/**
 * @brief Analisa uma expressão de comparação (<, >, LTE, GTE).
 */
std::unique_ptr<Expression> Parser::comparison()
{
    auto left = arithmetic();
    while (lookahead.getTag() == "LTE" || lookahead.getTag() == "GTE" ||
           lookahead.getTag() == "<" || lookahead.getTag() == ">")
    {
        const int line = lineno;
        Token token = lookahead;
        if (match("LTE") || match("GTE") || match("<") || match(">"))
        {
            auto right = arithmetic();
            left = withLine(std::make_unique<Relational>("BOOL", token, std::move(left), std::move(right)), line);
        }
    }
    return left;
}

/**
 * @brief Analisa uma expressão aritmética de adição e subtração.
 */
std::unique_ptr<Expression> Parser::arithmetic()
{
    auto left = term();
    while (lookahead.getTag() == "+" || lookahead.getTag() == "-")
    {
        const int line = lineno;
        std::string op = lookahead.getTag();
        match(op);
        auto right = term();
        left = withLine(std::make_unique<Arithmetic>("NUM", Token(op, op), std::move(left), std::move(right)), line);
    }
    return left;
}

/**
 * @brief Analisa uma expressão aritmética de multiplicação e divisão.
 */
std::unique_ptr<Expression> Parser::term()
{
    auto left = unary();
    while (lookahead.getTag() == "*" || lookahead.getTag() == "/")
    {
        const int line = lineno;
        Token token = lookahead;
        if (match("*") || match("/"))
        {
            auto right = unary();
            left = withLine(std::make_unique<Arithmetic>("NUM", token, std::move(left), std::move(right)), line);
        }
    }
    return left;
}

/**
 * @brief Analisa uma expressão unária.
 */
std::unique_ptr<Expression> Parser::unary()
{
    if (lookahead.getTag() == "-")
    {
        const int line = lineno;
        Token token = lookahead;
        match("-");
        auto expr = unary();
        return withLine(std::make_unique<Unary>("NUM", token, std::move(expr)), line);
    }
    else if (lookahead.getTag() == "!")
    {
        const int line = lineno;
        Token token = lookahead;
        match("!");
        auto expr = unary();
        return withLine(std::make_unique<Unary>("BOOL", token, std::move(expr)), line);
    }
    else if (lookahead.getTag() == "INC")
    {
        const int line = lineno;
        Token token = lookahead;
        match("INC");
        auto expr = unary();
        return withLine(std::make_unique<Unary>("NUM", token, std::move(expr)), line);
    }
    else if (lookahead.getTag() == "DEC")
    {
        const int line = lineno;
        Token token = lookahead;
        match("DEC");
        auto expr = unary();
        return withLine(std::make_unique<Unary>("NUM", token, std::move(expr)), line);
    }
    return primary();
}

/**
 * @brief Analisa uma expressão primária.
 */
std::unique_ptr<Expression> Parser::primary()
{
    if (lookahead.getTag() == "NUM")
    {
        const int line = lineno;
        std::string value = lookahead.getValue();
        match("NUM");
        return withLine(std::make_unique<Constant>("NUM", Token("NUM", value)), line);
    }
    else if (lookahead.getTag() == "STRING")
    {
        const int line = lineno;
        std::string value = lookahead.getValue();
        match("STRING");
        return withLine(std::make_unique<Constant>("STRING", Token("STRING", value)), line);
    }
    else if (lookahead.getTag() == "TRUE")
    {
        const int line = lineno;
        match("TRUE");
        return withLine(std::make_unique<Constant>("BOOL", Token("TRUE", "true")), line);
    }
    else if (lookahead.getTag() == "FALSE")
    {
        const int line = lineno;
        match("FALSE");
        return withLine(std::make_unique<Constant>("BOOL", Token("FALSE", "false")), line);
    }
    else if (lookahead.getTag() == "ID")
    {
        const int line = lineno;
        std::string name = lookahead.getValue();
        match("ID");
        std::unique_ptr<Expression> expr = withLine(std::make_unique<ID>("", Token("ID", name)), line);

        if (lookahead.getTag() == "LPAREN")
        {
            match("LPAREN");
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
            if (!match("RPAREN"))
            {
                throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
            }
            return withLine(std::make_unique<Call>("", Token("ID", name), std::move(expr), std::move(args), name), line);
        }
        else
        {
            while (lookahead.getTag() == "LBRACK")
            {
                const int access_line = lineno;
                match("LBRACK");
                auto index = disjunction();
                if (!match("RBRACK"))
                {
                    throw SyntaxError(lineno, "Esperado ']' no lugar de " + lookahead.getValue());
                }
                expr = withLine(std::make_unique<Access>("STRING", Token("ACCESS", "[]"), std::move(expr), std::move(index)), access_line);
            }

            if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC")
            {
                const int op_line = lineno;
                Token token = lookahead;
                match(lookahead.getTag());
                return withLine(std::make_unique<Unary>("NUM", token, std::move(expr), true), op_line);
            }
            return expr;
        }
    }
    else if (lookahead.getTag() == "LPAREN")
    {
        match("LPAREN");
        auto expr = disjunction();
        if (!match("RPAREN"))
        {
            throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
        }
        return expr;
    }
    else if (lookahead.getTag() == "LBRACK")
    {
        const int line = lineno;
        match("LBRACK");
        std::vector<std::unique_ptr<Expression>> elements;

        if (lookahead.getTag() != "RBRACK")
        {
            elements.push_back(disjunction());
            while (lookahead.getTag() == ",")
            {
                match(",");
                elements.push_back(disjunction());
            }
        }

        if (!match("RBRACK"))
        {
            throw SyntaxError(lineno, "Esperado ']' no lugar de " + lookahead.getValue());
        }

        return withLine(std::make_unique<Array>(std::move(elements)), line);
    }
    throw SyntaxError(lineno, "Esperado literal, identificador ou expressão em parênteses em lugar de " + lookahead.getValue());
}
