/**
 * @file parser_stmt.cpp
 * @brief Implementação dos métodos de parsing de statements.
 *
 * Contém: stmt(), stmtUnaryPrefix(), stmtId(), processArrayAccessStmt(),
 * processPostfixUnaryStmt(), processFunctionCallStmt(),
 * stmtIf(), stmtWhile(), stmtReturn(), stmtBreak(), stmtContinue(),
 * stmtSeq(), stmtPar(), stmtCChannel(), stmtSChannel(), stmtFor(),
 * processSimpleAssignStmt().
 */

#include "../../include/parser/parser_core.hpp"
#include "../../include/debug.hpp"
#include "../../include/error.hpp"
#include "../../include/ast.hpp"
#include <stdexcept>
#include <iostream>

/**
 * @brief Analisa um statement (instrução) do programa.
 */
std::unique_ptr<Node> Parser::stmt()
{
    LOG_DEBUG("Parser: Iniciando stmt(), lookahead: {tag: " << lookahead.getTag()
                                                             << ", value: " << lookahead.getValue()
                                                             << ", line: " << lineno << "}");
    // C07/C08: capturar linha do 1.º token do statement antes de match().
    const int line = lineno;
    std::unique_ptr<Node> result;

    if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC")
        result = stmtUnaryPrefix();
    else if (lookahead.getTag() == "ID")
        result = stmtId();
    else if (lookahead.getTag() == "FUNC")
        result = stmtFunc();
    else if (lookahead.getTag() == "IF")
        result = stmtIf();
    else if (lookahead.getTag() == "ELSE")
        throw SyntaxError(lineno, "'else' encontrado sem 'if' correspondente");
    else if (lookahead.getTag() == "WHILE")
        result = stmtWhile();
    else if (lookahead.getTag() == "RETURN")
        result = stmtReturn();
    else if (lookahead.getTag() == "BREAK")
        result = stmtBreak();
    else if (lookahead.getTag() == "CONTINUE")
        result = stmtContinue();
    else if (lookahead.getTag() == "SEQ")
        result = stmtSeq();
    else if (lookahead.getTag() == "PAR")
        result = stmtPar();
    else if (lookahead.getTag() == "C_CHANNEL")
        result = stmtCChannel();
    else if (lookahead.getTag() == "S_CHANNEL")
        result = stmtSChannel();
    else if (lookahead.getTag() == "FOR")
        result = stmtFor();
    else
    {
        LOG_DEBUG("Parser: Erro, token inesperado: " << lookahead.getTag());
        throw SyntaxError(lineno, lookahead.getValue() + " não inicia instrução válida");
    }

    if (result)
        result->setLine(line);
    return result;
}

// Processa operador unário pré-fixado (++ ou -- antes de um ID)
std::unique_ptr<Node> Parser::stmtUnaryPrefix()
{
    LOG_DEBUG("Parser: Detectado operador unário pré-fixado, processando...");
    Token token = lookahead;
    match(lookahead.getTag());
    if (lookahead.getTag() != "ID")
    {
        throw SyntaxError(lineno, "Esperado identificador após '" + token.getValue() +
                                       "' em lugar de " + lookahead.getValue());
    }
    std::string id_name = lookahead.getValue();
    match("ID");
    auto id = std::make_unique<ID>("", Token("ID", id_name));
    auto unary = std::make_unique<Unary>("NUM", token, std::move(id), false);
    return unary;
}

// Processa instruções iniciadas por um ID, delegando os diversos casos
std::unique_ptr<Node> Parser::stmtId()
{
    LOG_DEBUG("Parser: Detectado ID, processando...");
    std::string id_name = lookahead.getValue();
    match("ID");

    if (lookahead.getTag() == "LBRACK")
        return processArrayAccessStmt(id_name);
    else if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC")
        return processPostfixUnaryStmt(id_name);
    else if (lookahead.getTag() == "COLON")
        return processTypeDeclarationStmt(id_name);
    else if (lookahead.getTag() == "ASSIGN")
        return processSimpleAssignStmt(id_name);
    else if (lookahead.getTag() == "LPAREN")
        return processFunctionCallStmt(id_name);
    else
    {
        LOG_DEBUG("Parser: Erro, esperado ':', '=', '[', '(', '++' ou '--' após identificador");
        throw SyntaxError(lineno, "Esperado ':', '=', '[', '(', '++' ou '--' após identificador em lugar de " + lookahead.getValue());
    }
}

// Processa acesso a índice (array) e suas variações (atribuição ou pós-incremento)
std::unique_ptr<Node> Parser::processArrayAccessStmt(const std::string &id_name) {
    auto base = std::make_unique<ID>("", Token("ID", id_name));
    std::unique_ptr<Expression> current_access = std::move(base);

    while (lookahead.getTag() == "LBRACK") {
        match("LBRACK");
        auto index = disjunction();
        if (!match("RBRACK"))
            throw SyntaxError(lineno, "Esperado ']' no lugar de " + lookahead.getValue());
        current_access = std::make_unique<Access>("NUM", Token("ACCESS", "[]"),
                                                  std::move(current_access), std::move(index));
    }

    if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC") {
        Token token = lookahead;
        match(lookahead.getTag());
        auto unary = std::make_unique<Unary>("NUM", token, std::move(current_access), true);
        return unary;
    }
    if (match("ASSIGN")) {
        auto right = disjunction(); // T17: RHS aceita bool/relacional
        return std::make_unique<Assign>(std::move(current_access), std::move(right));
    }
    return current_access;
}

// Processa operador unário pós-fixado (ex.: x++ ou x--)
std::unique_ptr<Node> Parser::processPostfixUnaryStmt(const std::string &id_name)
{
    Token token = lookahead;
    match(lookahead.getTag());
    auto id = std::make_unique<ID>("", Token("ID", id_name));
    auto unary = std::make_unique<Unary>("NUM", token, std::move(id), true);
    return unary;
}

// Processa atribuição simples (ex.: x = ...)
std::unique_ptr<Node> Parser::processSimpleAssignStmt(const std::string &id_name)
{
    match("ASSIGN");
    auto right = disjunction(); // T17: RHS aceita bool/relacional
    auto id = std::make_unique<ID>("", Token("ID", id_name));
    return std::make_unique<Assign>(std::move(id), std::move(right));
}

// Processa chamada de função (ex.: func(...))
std::unique_ptr<Node> Parser::processFunctionCallStmt(const std::string &id_name)
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
        throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
    return std::make_unique<Call>("", Token("ID", id_name),
                                  std::make_unique<ID>("", Token("ID", id_name)),
                                  std::move(args), id_name);
}

// Processa estrutura condicional IF-ELSE
std::unique_ptr<Node> Parser::stmtIf()
{
    LOG_DEBUG("Parser: Detectado IF, processando...");
    match("IF");
    if (!match("LPAREN"))
        throw SyntaxError(lineno, "Esperado '(' no lugar de " + lookahead.getValue());
    auto cond = disjunction();
    if (!match("RPAREN"))
        throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());

    std::unique_ptr<Body> else_stmt = nullptr;
    if (lookahead.getTag() == "ELSE")
    {
        LOG_DEBUG("Parser: Detectado ELSE, processando...");
        match("ELSE");
        if (lookahead.getTag() == "IF")
        {
            auto else_if_stmt = stmt();
            else_stmt = std::make_unique<Body>();
            else_stmt->push_back(std::move(else_if_stmt));
        }
        else
        {
            if (!match("LBRACE"))
                throw SyntaxError(lineno, "Esperado '{' após 'else' no lugar de " + lookahead.getValue());
            Body else_body;
            while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
                else_body.push_back(stmt());
            if (!match("RBRACE"))
                throw SyntaxError(lineno, "Esperado '}' após corpo do else no lugar de " + lookahead.getValue());
            else_stmt = std::make_unique<Body>(std::move(else_body));
        }
    }
    return std::make_unique<If>(std::move(cond), std::make_unique<Body>(std::move(body)), std::move(else_stmt));
}

// Processa laço WHILE
std::unique_ptr<Node> Parser::stmtWhile()
{
    LOG_DEBUG("Parser: Detectado WHILE, processando...");
    match("WHILE");
    if (!match("LPAREN"))
        throw SyntaxError(lineno, "Esperado '(' no lugar de " + lookahead.getValue());
    auto cond = disjunction();
    if (!match("RPAREN"))
        throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
    return std::make_unique<While>(std::move(cond), std::make_unique<Body>(std::move(body)));
}

// Processa comando RETURN
std::unique_ptr<Node> Parser::stmtReturn()
{
    LOG_DEBUG("Parser: Detectado RETURN, processando...");
    match("RETURN");
    auto expr = disjunction();
    return std::make_unique<Return>(std::move(expr));
}

// Processa comando BREAK
std::unique_ptr<Node> Parser::stmtBreak()
{
    LOG_DEBUG("Parser: Detectado BREAK, processando...");
    match("BREAK");
    return std::make_unique<Break>();
}

// Processa comando CONTINUE
std::unique_ptr<Node> Parser::stmtContinue()
{
    LOG_DEBUG("Parser: Detectado CONTINUE, processando...");
    match("CONTINUE");
    return std::make_unique<Continue>();
}

// Processa sequência de instruções (SEQ)
std::unique_ptr<Node> Parser::stmtSeq()
{
    LOG_DEBUG("Parser: Detectado SEQ, processando...");
    match("SEQ");
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' após 'seq' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' após corpo do seq no lugar de " + lookahead.getValue());
    return std::make_unique<Seq>(std::make_unique<Body>(std::move(body)), true);
}

// Processa execução paralela (PAR)
std::unique_ptr<Node> Parser::stmtPar()
{
    LOG_DEBUG("Parser: Detectado PAR, processando...");
    match("PAR");
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' após 'par' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' após corpo do par no lugar de " + lookahead.getValue());
    return std::make_unique<Par>(std::make_unique<Body>(std::move(body)));
}

// Processa declaração de canal cliente (C_CHANNEL)
std::unique_ptr<Node> Parser::stmtCChannel()
{
    LOG_DEBUG("Parser: Detectado C_CHANNEL, processando...");
    match("C_CHANNEL");

    std::string name = lookahead.getValue();
    if (!match("ID"))
        throw SyntaxError(lineno, "Esperado identificador para c_channel em lugar de " + lookahead.getValue());

    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());

    auto localhost = disjunction();
    if (!match(","))
        throw SyntaxError(lineno, "Esperado ',' após localhost em lugar de " + lookahead.getValue());

    auto port = disjunction();
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());

    return std::make_unique<CChannel>(name, std::move(localhost), std::move(port));
}

// Processa declaração de canal servidor (S_CHANNEL)
std::unique_ptr<Node> Parser::stmtSChannel()
{
    LOG_DEBUG("Parser: Detectado S_CHANNEL, processando...");
    match("S_CHANNEL");
    std::string name = lookahead.getValue();
    if (!match("ID"))
    {
        throw SyntaxError(lineno, "Esperado identificador para s_channel em lugar de " + lookahead.getValue());
    }
    if (!match("LBRACE"))
    {
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
    }
    std::string func_name = lookahead.getValue();
    if (!match("ID"))
    {
        throw SyntaxError(lineno, "Esperado identificador de função em lugar de " + lookahead.getValue());
    }
    if (!match(","))
    {
        throw SyntaxError(lineno, "Esperado ',' após nome da função em lugar de " + lookahead.getValue());
    }
    auto desc = disjunction();
    if (!match(","))
    {
        throw SyntaxError(lineno, "Esperado ',' após descrição em lugar de " + lookahead.getValue());
    }
    auto localhost = disjunction();
    if (!match(","))
    {
        throw SyntaxError(lineno, "Esperado ',' após localhost em lugar de " + lookahead.getValue());
    }
    auto port = disjunction();
    if (!match("RBRACE"))
    {
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
    }
    return std::make_unique<SChannel>(name, std::move(localhost), std::move(port), func_name, std::move(desc));
}

/**
 * @brief Processa o laço FOR reescrevendo-o como uma sequência com inicialização e um laço WHILE.
 */
std::unique_ptr<Node> Parser::stmtFor()
{
    match("FOR");
    match("LPAREN");
    auto init = stmt();
    match(";");
    auto cond = disjunction();
    match(";");
    auto incr = stmt();
    match("RPAREN");
    match("LBRACE");
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    match("RBRACE");

    Body while_body;
    for (auto &s : body)
        while_body.push_back(std::move(s));
    while_body.push_back(std::move(incr));

    Body for_body;
    for_body.push_back(std::move(init));
    for_body.push_back(std::make_unique<While>(std::move(cond), std::make_unique<Body>(std::move(while_body))));
    return std::make_unique<Seq>(std::make_unique<Body>(std::move(for_body)), true);
}
