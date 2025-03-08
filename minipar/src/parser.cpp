/**
 * @file parser.cpp
 * @brief Implementação do analisador sintático para a linguagem Minipar.
 *
 * Este arquivo implementa a classe Parser, responsável por converter uma lista de tokens,
 * gerada pelo lexer, em uma árvore de sintaxe abstrata (AST). O parser utiliza técnicas
 * de descida recursiva para interpretar a estrutura do programa, identificando declarações,
 * expressões, funções, estruturas de controle (if, while, for, etc.), declarações de canais e outros.
 */

#include "../include/parser.hpp"
#include "../include/debug.hpp"
#include <stdexcept>
#include <iostream>

/**
 * @brief Construtor do Parser.
 *
 * Inicializa o parser com a lista de tokens e configura o ponteiro de posição (pos),
 * o token de lookahead (próximo token) e o número da linha (lineno). Também cria uma tabela
 * de símbolos compartilhada e insere as funções padrão na mesma.
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
    // Inserir funções padrão na tabela de símbolos
    for (const auto &[name, type] : DEFAULT_FUNCTION_NAMES)
    {
        symtable->insert(name, Symbol(name, "FUNC"));
    }
}

/**
 * @brief Tenta casar (match) o token atual com um token esperado.
 *
 * Se o token atual possuir a tag igual à esperada, avança a posição para o próximo token,
 * atualizando o lookahead e o número da linha.
 *
 * @param tag A tag do token esperado.
 * @return true se o token atual casar com o token esperado; false caso contrário.
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
 *
 * Chama a função program() que inicia a análise sintática e retorna o nó raiz da árvore sintática.
 *
 * @return std::unique_ptr<Module> Ponteiro único para o nó raiz (Module) da AST.
 */
std::unique_ptr<Module> Parser::start()
{
    return program();
}

/**
 * @brief Analisa o programa completo.
 *
 * Cria um módulo (Module) contendo um corpo (Body) formado a partir dos statements extraídos.
 *
 * @return std::unique_ptr<Module> Ponteiro para o objeto Module que representa o programa.
 */
std::unique_ptr<Module> Parser::program()
{
    return std::make_unique<Module>(std::make_unique<Body>(std::move(stmts())));
}

/**
 * @brief Analisa uma sequência de statements.
 *
 * Enquanto o token lookahead não for "EOF", chama a função stmt() para cada instrução
 * e as acumula em um Body.
 *
 * @return Body contendo todos os statements do programa.
 */
Body Parser::stmts()
{
    Body stmts;
    while (lookahead.getTag() != "EOF")
    {
        skipWhitespace();
        if (lookahead.getTag() != "EOF")
        {
            stmts.push_back(stmt());
        }
    }
    return stmts;
}

/**
 * @brief Avança o parser ignorando tokens de espaço em branco e quebras de linha.
 *
 * Remove todos os tokens cujo tipo é "WHITESPACE" ou "NEWLINE", garantindo que o parser
 * comece sempre em um token significativo.
 */
void Parser::skipWhitespace()
{
    while (lookahead.getTag() == "WHITESPACE" || lookahead.getTag() == "NEWLINE")
    {
        match(lookahead.getTag());
    }
}

/**
 * @brief Analisa um statement (instrução) do programa.
 *
 * Dependendo do token lookahead, identifica e cria o nó correspondente, como:
 * - Atribuição (incluindo atribuição a índices de array)
 * - Declaração de variável com tipo
 * - Chamada de função
 * - Definição de função
 * - Estruturas condicionais (if/else)
 * - Estruturas de repetição (while, for)
 * - Comandos de controle (return, break, continue)
 * - Declaração de canais (C_CHANNEL e S_CHANNEL)
 * - Estruturas de execução sequencial (seq) ou paralela (par)
 *
 * @return std::unique_ptr<Node> Ponteiro para o nó representando o statement.
 * @throws SyntaxError em caso de erro de sintaxe.
 */
std::unique_ptr<Node> Parser::stmt()
{
    LOG_DEBUG("Parser: Iniciando stmt(), lookahead: {tag: " << lookahead.getTag()
                                                            << ", value: " << lookahead.getValue()
                                                            << ", line: " << lineno << "}");
    if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC")
        return stmtUnaryPrefix();
    else if (lookahead.getTag() == "ID")
        return stmtId();
    else if (lookahead.getTag() == "FUNC")
        return stmtFunc();
    else if (lookahead.getTag() == "IF")
        return stmtIf();
    else if (lookahead.getTag() == "ELSE")
        throw SyntaxError(lineno, "'else' encontrado sem 'if' correspondente");
    else if (lookahead.getTag() == "WHILE")
        return stmtWhile();
    else if (lookahead.getTag() == "RETURN")
        return stmtReturn();
    else if (lookahead.getTag() == "BREAK")
        return stmtBreak();
    else if (lookahead.getTag() == "CONTINUE")
        return stmtContinue();
    else if (lookahead.getTag() == "SEQ")
        return stmtSeq();
    else if (lookahead.getTag() == "PAR")
        return stmtPar();
    else if (lookahead.getTag() == "C_CHANNEL")
        return stmtCChannel();
    else if (lookahead.getTag() == "S_CHANNEL")
        return stmtSChannel();
    else if (lookahead.getTag() == "FOR")
        return stmtFor();
    else
    {
        LOG_DEBUG("Parser: Erro, token inesperado: " << lookahead.getTag());
        throw SyntaxError(lineno, lookahead.getValue() + " não inicia instrução válida");
    }
}

// Processa operador unário pré-fixado (++ ou -- antes de um ID)
std::unique_ptr<Node> Parser::stmtUnaryPrefix()
{
    LOG_DEBUG("Parser: Detectado operador unário pré-fixado, processando...");
    Token token = lookahead;
    match(lookahead.getTag());
    skipWhitespace();
    if (lookahead.getTag() != "ID")
    {
        throw SyntaxError(lineno, "Esperado identificador após '" + token.getValue() +
                                      "' em lugar de " + lookahead.getValue());
    }
    std::string id_name = lookahead.getValue();
    match("ID");
    auto id = std::make_unique<ID>("", Token("ID", id_name));
    auto unary = std::make_unique<Unary>("NUM", token, std::move(id), false); // false indica pré-fixado
    skipWhitespace();
    return unary;
}

// Processa instruções iniciadas por um ID, delegando os diversos casos
///
/// Instruções que começam com um ID podem ser de vários tipos:
///  - Atribuição com tipo (ex.: x: int = 5)
///  - Atribuição simples (ex.: x = 5)
///  - Acessos a índices em arrays (ex.: x[y])
///  - Chamada de função (ex.: x(5, 6))
///  - Incremento/Decremento pós-fixado (ex.: x++ ou x--)
///
/// @param id_name Nome do ID
/// @return Um nó do tipo ArrayAccess, Assign, TypeDecl, Call, PostfixUnary ou Unary,
/// dependendo do caso
std::unique_ptr<Node> Parser::stmtId()
{
    LOG_DEBUG("Parser: Detectado ID, processando...");
    std::string id_name = lookahead.getValue();
    match("ID");
    skipWhitespace();

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
///
/// Acessos a índice em arrays podem ser feitos com ou sem atribuição. Se houver
/// atribuição, o valor à direita do sinal de atribuição será atribuído ao elemento
/// no índice especificado. Caso contrário, o valor do elemento no índice especificado
/// será retornado.
///
/// @param id_name Nome do array
/// @return Um nó do tipo Access ou Unary, dependendo do caso
std::unique_ptr<Node> Parser::processArrayAccessStmt(const std::string &id_name)
{
    match("LBRACK");
    auto index = disjunction();
    if (!match("RBRACK"))
        throw SyntaxError(lineno, "Esperado ']' no lugar de " + lookahead.getValue());
    skipWhitespace();
    if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC")
    {
        // Pós-incremento ou pós-decremento
        Token token = lookahead;
        match(lookahead.getTag());
        auto access = std::make_unique<Access>("NUM", Token("ACCESS", "[]"),
                                               std::make_unique<ID>("", Token("ID", id_name)),
                                               std::move(index));
        auto unary = std::make_unique<Unary>("NUM", token, std::move(access), true);
        skipWhitespace();
        return unary;
    }
    if (!match("ASSIGN"))
        throw SyntaxError(lineno, "Esperado '=' após acesso a índice em lugar de " + lookahead.getValue());
    skipWhitespace();
    auto right = arithmetic();
    skipWhitespace();
    auto access = std::make_unique<Access>("NUM", Token("ACCESS", "[]"),
                                           std::make_unique<ID>("", Token("ID", id_name)),
                                           std::move(index));
    return std::make_unique<Assign>(std::move(access), std::move(right));
}

// Processa operador unário pós-fixado (ex.: x++ ou x--)
std::unique_ptr<Node> Parser::processPostfixUnaryStmt(const std::string &id_name)
{
    Token token = lookahead;
    match(lookahead.getTag());
    auto id = std::make_unique<ID>("", Token("ID", id_name));
    auto unary = std::make_unique<Unary>("NUM", token, std::move(id), true);
    skipWhitespace();
    return unary;
}

// Processa declaração com tipo, seja para array ou para declaração simples com atribuição
///
/// Processa declarações como `x: int = 5` ou `x: array[10] = 5`. Se o tipo for
/// 'array', espera-se que o próximo token seja '[' e contenha a expressão de tamanho.
/// Se o próximo token for '=', atribui o valor da expressão à direita do sinal de
/// atribuição ao identificador.
///
/// @param id_name Nome do identificador
/// @return Um nó do tipo ArrayDecl, Assign ou ID, dependendo do caso
std::unique_ptr<Node> Parser::processTypeDeclarationStmt(const std::string &id_name)
{
    match("COLON");
    skipWhitespace();
    if (lookahead.getValue() == "array")
    {
        match("TYPE");
        skipWhitespace();
        if (lookahead.getTag() == "LBRACK")
        {
            match("LBRACK");
            auto size_expr = disjunction();
            if (!match("RBRACK"))
                throw SyntaxError(lineno, "Esperado ']' após expressão de tamanho");
            skipWhitespace();
            if (lookahead.getTag() == "ASSIGN")
            {
                match("ASSIGN");
                skipWhitespace();
                auto right = arithmetic();
                skipWhitespace();
                auto array_decl = std::make_unique<ArrayDecl>(id_name, std::move(size_expr));
                auto assign = std::make_unique<Assign>(
                    std::make_unique<ID>("ID", Token("ID", id_name)),
                    std::move(right));
                Body seq_body;
                seq_body.push_back(std::move(array_decl));
                seq_body.push_back(std::move(assign));
                return std::make_unique<Seq>(std::make_unique<Body>(std::move(seq_body)));
            }
            else
            {
                return std::make_unique<ArrayDecl>(id_name, std::move(size_expr));
            }
        }
        else
        {
            throw SyntaxError(lineno, "Esperado '[' após 'array'");
        }
    }
    else
    {
        std::string type = lookahead.getValue();
        if (!match("TYPE"))
            throw SyntaxError(lineno, "Esperado um tipo após ':' em lugar de " + lookahead.getValue());
        skipWhitespace();
        if (!match("ASSIGN"))
            throw SyntaxError(lineno, "Esperado '=' após tipo em lugar de " + lookahead.getValue());
        skipWhitespace();
        auto right = arithmetic();
        skipWhitespace();
        auto id = std::make_unique<ID>(type, Token("ID", id_name), true);
        return std::make_unique<Assign>(std::move(id), std::move(right));
    }
}

// Processa atribuição simples (ex.: x = ...)
std::unique_ptr<Node> Parser::processSimpleAssignStmt(const std::string &id_name)
{
    match("ASSIGN");
    skipWhitespace();
    auto right = arithmetic();
    skipWhitespace();
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
    skipWhitespace();
    return std::make_unique<Call>("", Token("ID", id_name),
                                  std::make_unique<ID>("", Token("ID", id_name)),
                                  std::move(args), id_name);
}

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
    skipWhitespace();
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
    skipWhitespace();
    auto func_def = std::make_unique<FuncDef>(name, type, std::move(params),
                                              std::make_unique<Body>(std::move(body)));
    symtable->insert(name, Symbol(name, "FUNC"));
    return func_def;
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
    skipWhitespace();
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
    skipWhitespace();

    std::unique_ptr<Body> else_stmt = nullptr;
    if (lookahead.getTag() == "ELSE")
    {
        LOG_DEBUG("Parser: Detectado ELSE, processando...");
        match("ELSE");
        skipWhitespace();
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
    skipWhitespace();
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
    skipWhitespace();
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
    skipWhitespace();
    return std::make_unique<While>(std::move(cond), std::make_unique<Body>(std::move(body)));
}

// Processa comando RETURN
std::unique_ptr<Node> Parser::stmtReturn()
{
    LOG_DEBUG("Parser: Detectado RETURN, processando...");
    match("RETURN");
    skipWhitespace();
    auto expr = disjunction();
    skipWhitespace();
    return std::make_unique<Return>(std::move(expr));
}

// Processa comando BREAK
std::unique_ptr<Node> Parser::stmtBreak()
{
    LOG_DEBUG("Parser: Detectado BREAK, processando...");
    match("BREAK");
    skipWhitespace();
    return std::make_unique<Break>();
}

// Processa comando CONTINUE
std::unique_ptr<Node> Parser::stmtContinue()
{
    LOG_DEBUG("Parser: Detectado CONTINUE, processando...");
    match("CONTINUE");
    skipWhitespace();
    return std::make_unique<Continue>();
}

// Processa sequência de instruções (SEQ)
std::unique_ptr<Node> Parser::stmtSeq()
{
    LOG_DEBUG("Parser: Detectado SEQ, processando...");
    match("SEQ");
    skipWhitespace();
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' após 'seq' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' após corpo do seq no lugar de " + lookahead.getValue());
    skipWhitespace();
    return std::make_unique<Seq>(std::make_unique<Body>(std::move(body)));
}

// Processa execução paralela (PAR)
std::unique_ptr<Node> Parser::stmtPar()
{
    LOG_DEBUG("Parser: Detectado PAR, processando...");
    match("PAR");
    skipWhitespace();
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' após 'par' no lugar de " + lookahead.getValue());
    Body body;
    while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        body.push_back(stmt());
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' após corpo do par no lugar de " + lookahead.getValue());
    skipWhitespace();
    return std::make_unique<Par>(std::make_unique<Body>(std::move(body)));
}

// Processa declaração de canal cliente (C_CHANNEL)
/**
 * @brief Processa a declaração de um canal de cliente (C_CHANNEL).
 *
 * A declaração de um canal de cliente define um canal para se conectar a um
 * cliente. Ela tem a seguinte forma:
 * C_CHANNEL nome { localhost, porta }
 *
 * @return Um nó do tipo CChannel.
 * @throws SyntaxError se a sintaxe estiver incorreta.
 */
std::unique_ptr<Node> Parser::stmtCChannel()
{
    LOG_DEBUG("Parser: Detectado C_CHANNEL, processando...");

    // Verifica e consome o token 'C_CHANNEL'
    match("C_CHANNEL");

    // Captura e valida o identificador do canal
    std::string name = lookahead.getValue();
    if (!match("ID"))
        throw SyntaxError(lineno, "Esperado identificador para c_channel em lugar de " + lookahead.getValue());

    skipWhitespace();

    // Verifica e consome '{'
    if (!match("LBRACE"))
        throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());

    // Captura e valida o endereço localhost
    auto localhost = disjunction();
    if (!match(","))
        throw SyntaxError(lineno, "Esperado ',' após localhost em lugar de " + lookahead.getValue());

    // Captura e valida o número da porta
    auto port = disjunction();
    if (!match("RBRACE"))
        throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());

    skipWhitespace();

    // Cria e retorna um nó CChannel com os valores capturados
    return std::make_unique<CChannel>(name, std::move(localhost), std::move(port));
}

// Processa declaração de canal servidor (S_CHANNEL)
///
/// A declaração de canal servidor define um canal para se conectar a um
/// servidor. Ela tem a seguinte forma:
///  S_CHANNEL nome { funcao, descricao, localhost, porta }
///
/// @param name Nome do canal
/// @param func_name Nome da função que realizará a conexão
/// @param desc Descrição do canal
/// @param localhost Endereço do servidor
/// @param port Número da porta do servidor
/// @return Um nó do tipo SChannel
std::unique_ptr<Node> Parser::stmtSChannel()
{
    LOG_DEBUG("Parser: Detectado S_CHANNEL, processando...");
    match("S_CHANNEL");
    std::string name = lookahead.getValue();
    if (!match("ID"))
    {
        throw SyntaxError(lineno, "Esperado identificador para s_channel em lugar de " + lookahead.getValue());
    }
    skipWhitespace();
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
    skipWhitespace();
    return std::make_unique<SChannel>(name, std::move(localhost), std::move(port), func_name, std::move(desc));
}

/**
 * @brief Processa o laço FOR reescrevendo-o como uma sequência com inicialização e um laço WHILE.
 *
 * A sintaxe do laço FOR é:
 *  for (inicializacao; condicao; incremento) {
 *      // Corpo do laço
 *  }
 *
 * que é transformado em:
 *
 *  {
 *      inicializacao;
 *      while (condicao) {
 *          // Corpo do laço
 *          incremento;
 *      }
 *  }
 *
 * @return std::unique_ptr<Node> Uma instância de Seq com a sequência de inicialização,
 *    incremento e corpo do laço.
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

    // Reescreve o FOR como uma sequência de inicialização seguida de um WHILE
    Body while_body;
    for (auto &s : body)
        while_body.push_back(std::move(s));
    while_body.push_back(std::move(incr));

    Body for_body;
    for_body.push_back(std::move(init));
    for_body.push_back(std::make_unique<While>(std::move(cond), std::make_unique<Body>(std::move(while_body))));
    return std::make_unique<Seq>(std::make_unique<Body>(std::move(for_body)));
}

/**
 * @brief Analisa a lista de parâmetros de uma função.
 *
 * Os parâmetros devem estar delimitados por parênteses e separados por vírgulas.
 *
 * @return Parameters Um mapa com os nomes dos parâmetros e seus tipos (e valor padrão, se houver).
 * @throws SyntaxError se a sintaxe dos parâmetros estiver incorreta.
 */
Parameters Parser::params()
{
    Parameters parameters;
    if (!match("LPAREN"))
    {
        throw SyntaxError(lineno, "esperando ( no lugar de " + lookahead.getValue());
    }
    if (lookahead.getValue() != ")")
    {
        auto p = param();
        parameters.insert({std::move(p.first), std::move(p.second)});
    }
    while (lookahead.getTag() == ",")
    {
        match(",");
        auto p = param();
        parameters.insert({std::move(p.first), std::move(p.second)});
    }
    if (!match("RPAREN"))
    {
        throw SyntaxError(lineno, "esperando ) no lugar de " + lookahead.getValue());
    }
    return parameters;
}

/**
 * @brief Analisa um parâmetro individual.
 *
 * Um parâmetro é composto por um identificador, seguido por ':' e um tipo.
 * Pode ter um valor padrão opcional.
 *
 * @return std::pair<std::string, std::pair<std::string, std::unique_ptr<Expression>>>
 *         Um par contendo o nome do parâmetro e um par com o tipo e o valor padrão.
 * @throws SyntaxError se houver erro na definição do parâmetro.
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
    if (lookahead.getTag() == "=")
    {
        match("ASSIGN");
        default_value = disjunction();
    }
    return std::make_pair(name, std::make_pair(type, std::move(default_value)));
}

/**
 * @brief Analisa uma expressão de disjunção (operador OR).
 *
 * Constrói uma árvore de expressão para operações OR, utilizando o método conjunction() para os operandos.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
 */
std::unique_ptr<Expression> Parser::disjunction()
{
    skipWhitespace(); // Garante que inicia com token válido
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
 *
 * Constrói uma árvore de expressão para operações AND, utilizando o método equality() para os operandos.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
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
 *
 * Se o identificador for seguido por '(', interpreta como chamada de função; se vier ':',
 * indica declaração de variável com tipo.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
 */
std::unique_ptr<Expression> Parser::local()
{
    std::string name = lookahead.getValue();
    match("ID");

    // Verifica se é chamada de função
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

    // Se houver ':' para declaração de variável com tipo
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
 * @brief Retorna o valor de um identificador em um contexto específico.
 *
 * @param context Contexto em que o identificador é esperado (ex: "FUNC").
 * @return std::string Nome do identificador.
 * @throws SyntaxError se o token atual não for um identificador.
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

/**
 * @brief Analisa uma expressão de igualdade (==, !=).
 *
 * Combina expressões de comparação para formar uma expressão que avalia igualdade ou desigualdade.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
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
 *
 * Combina expressões aritméticas utilizando os operadores de comparação.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
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
 *
 * Combina termos para formar operações de soma e subtração.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
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
 *
 * Combina termos para formar operações de multiplicação e divisão.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
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
 *
 * Trata operadores unários, como a negação aritmética (-) e lógica (!), chamando primary() para
 * a expressão literal ou agrupada.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão resultante.
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
 * @brief Retorna o próximo token sem avançar a posição atual.
 *
 * @return Token Próximo token ou "EOF" se não houver.
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
 * @brief Analisa uma expressão primária.
 *
 * Trata literais (números, strings, booleanos), identificadores, chamadas de função,
 * acessos a elementos de array, expressões agrupadas em parênteses e construções de arrays.
 *
 * @return std::unique_ptr<Expression> Ponteiro para a expressão primária.
 * @throws SyntaxError se nenhum literal, identificador ou expressão válida for encontrada.
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
        else if (lookahead.getTag() == "LBRACK")
        {
            match("LBRACK");
            auto index = disjunction();
            if (!match("RBRACK"))
            {
                throw SyntaxError(lineno, "Esperado ']' no lugar de " + lookahead.getValue());
            }
            if (lookahead.getTag() == "INC" || lookahead.getTag() == "DEC")
            {
                Token token = lookahead;
                match(lookahead.getTag());
                auto access = std::make_unique<Access>("STRING", Token("ACCESS", "[]"),
                                                       std::make_unique<ID>("", Token("ID", name)),
                                                       std::move(index));
                return std::make_unique<Unary>("NUM", token, std::move(access), true); // true para pós-fixado
            }
            return std::make_unique<Access>("STRING", Token("ACCESS", "[]"),
                                            std::make_unique<ID>("", Token("ID", name)),
                                            std::move(index));
        }
        else if (lookahead.getTag() == "INC") // x++
        {
            Token token = lookahead;
            match("INC");
            auto id = std::make_unique<ID>("", Token("ID", name));
            return std::make_unique<Unary>("NUM", token, std::move(id), true);
        }
        else if (lookahead.getTag() == "DEC") // x--
        {
            Token token = lookahead;
            match("DEC");
            auto id = std::make_unique<ID>("", Token("ID", name));
            return std::make_unique<Unary>("NUM", token, std::move(id), true);
        }
        return std::make_unique<ID>("", Token("ID", name));
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
