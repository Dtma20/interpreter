/**
 * @file parser_expr.cpp
 * @brief Implementação dos métodos de parsing de expressões.
 *
 * Contém: disjunction(), conjunction(), equality(), comparison(),
 * arithmetic(), term(), unary(), primary(), local().
 */

#include "../../include/parser/parser_core.hpp"
#include "../../include/debug.hpp"
#include <stdexcept>

/**
 * @brief Analisa uma expressão de disjunção (operador OR).
 */
std::unique_ptr<Expression> Parser::disjunction()
{
    skipWhitespace();
    auto left = conjunction();
    while (lookahead.getTag() == "OR")
    {
        Token token = lookahead;
        match("OR");
        skipWhitespace();
        auto right = conjunction();
        left = std::make_unique<Logical>("BOOL", token, std::move(left), std::move(right));
    }
    return left;
}

/**
 * @brief Analisa uma expressão de conjunção (operador AND).
 */
std::unique_ptr<Expression> Parser::conjunction()
{
    skipWhitespace();
    auto left = equality();
    while (lookahead.getTag() == "AND")
    {
        Token token = lookahead;
        match("AND");
        skipWhitespace();
        auto right = equality();
        left = std::make_unique<Logical>("BOOL", token, std::move(left), std::move(right));
    }
    return left;
}

/**
 * @brief Analisa uma expressão que representa uma variável ou chamada de função.
 */
std::unique_ptr<Expression> Parser::local()
{
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
        return std::make_unique<Call>("", Token("ID", name),
                                      std::make_unique<ID>("", Token("ID", name)),
                                      std::move(args), name);
    }

    if (match("COLON"))
    {
        std::string type = lookahead.getValue();
        if (!match("TYPE"))
        {
            throw SyntaxError(lineno, "Esperado um tipo após ':' em lugar de " + lookahead.getValue());
        }
        return std::make_unique<ID>(type, Token("ID", name), true);
    }

    return std::make_unique<ID>("", Token("ID", name));
}

/**
 * @brief Analisa uma expressão de igualdade (==, !=).
 */
std::unique_ptr<Expression> Parser::equality()
{
    auto left = comparison();
    while (lookahead.getTag() == "EQ" || lookahead.getTag() == "NEQ")
    {
        Token token = lookahead;
        if (match("EQ") || match("NEQ"))
        {
            auto right = comparison();
            left = std::make_unique<Relational>("BOOL", token, std::move(left), std::move(right));
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
        Token token = lookahead;
        if (match("LTE") || match("GTE") || match("<") || match(">"))
        {
            auto right = arithmetic();
            left = std::make_unique<Relational>("BOOL", token, std::move(left), std::move(right));
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
        std::string op = lookahead.getTag();
        match(op);
        auto right = term();
        left = std::make_unique<Arithmetic>("NUM", Token(op, op), std::move(left), std::move(right));
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
        Token token = lookahead;
        if (match("*") || match("/"))
        {
            auto right = unary();
            left = std::make_unique<Arithmetic>("NUM", token, std::move(left), std::move(right));
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
        Token token = lookahead;
        match("-");
        auto expr = unary();
        return std::make_unique<Unary>("NUM", token, std::move(expr));
    }
    else if (lookahead.getTag() == "!")
    {
        Token token = lookahead;
        match("!");
        auto expr = unary();
        return std::make_unique<Unary>("BOOL", token, std::move(expr));
    }
    else if (lookahead.getTag() == "INC")
    {
        Token token = lookahead;
        match("INC");
        auto expr = unary();
        return std::make_unique<Unary>("NUM", token, std::move(expr));
    }
    else if (lookahead.getTag() == "DEC")
    {
        Token token = lookahead;
        match("DEC");
        auto expr = unary();
        return std::make_unique<Unary>("NUM", token, std::move(expr));
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
        std::string value = lookahead.getValue();
        match("NUM");
        return std::make_unique<Constant>("NUM", Token("NUM", value));
    }
    else if (lookahead.getTag() == "STRING")
    {
        std::string value = lookahead.getValue();
        match("STRING");
        return std::make_unique<Constant>("STRING", Token("STRING", value));
    }
    else if (lookahead.getTag() == "TRUE")
    {
        match("TRUE");
        return std::make_unique<Constant>("BOOL", Token("TRUE", "true"));
    }
    else if (lookahead.getTag() == "FALSE")
    {
        match("FALSE");
        return std::make_unique<Constant>("BOOL", Token("FALSE", "false"));
    }
    else if (lookahead.getTag() == "ID")
    {
        std::string name = lookahead.getValue();
        match("ID");
        std::unique_ptr<Expression> expr = std::make_unique<ID>("", Token("ID", name));
    
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
            return std::make_unique<Call>("", Token("ID", name), std::move(expr), std::move(args), name);
        }
        else
        {
            while (lookahead.getTag() == "LBRACK")
            {
                match("LBRACK");
                auto index = disjunction();
                if (!match("RBRACK"))
                {
                    throw SyntaxError(lineno, "Esperado ']' no lugar de " + lookahead.getValue());
                }
                expr = std::make_unique<Access>("STRING", Token("ACCESS", "[]"), std::move(expr), std::move(index));
            }
    
            if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC")
            {
                Token token = lookahead;
                match(lookahead.getTag());
                return std::make_unique<Unary>("NUM", token, std::move(expr), true);
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

        return std::make_unique<Array>(std::move(elements));
    }
    throw SyntaxError(lineno, "Esperado literal, identificador ou expressão em parênteses em lugar de " + lookahead.getValue());
}
