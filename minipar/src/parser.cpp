#include "../include/parser.hpp"
#include "../include/debug.hpp"
#include <stdexcept>
#include <iostream>

/**
 * @brief Construtor do Parser.
 *
 * Inicializa o parser com uma lista de tokens e configura o ponteiro de posição,
 * o lookahead (próximo token) e o número da linha. Também cria uma tabela de símbolos
 * compartilhada e insere as funções padrão.
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
 * @brief Tenta casar (match) o token atual com o token esperado.
 *
 * Se o token atual possuir a tag esperada, avança a posição para o próximo token,
 * atualizando o lookahead e o número da linha.
 *
 * @param tag A tag do token esperado.
 * @return true se o token atual casou com a tag; false caso contrário.
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
 * Retorna o nó raiz da árvore sintática (Module) gerado a partir do código fonte.
 *
 * @return Ponteiro único para um objeto Module.
 */
std::unique_ptr<Module> Parser::start()
{
    return program();
}

/**
 * @brief Analisa o programa completo.
 *
 * Cria um módulo contendo um corpo (Body) formado a partir dos statements extraídos.
 *
 * @return Ponteiro único para um objeto Module.
 */
std::unique_ptr<Module> Parser::program()
{
    return std::make_unique<Module>(std::make_unique<Body>(std::move(stmts())));
}

/**
 * @brief Analisa uma sequência de statements.
 *
 * Itera enquanto o token lookahead não for EOF, chamando stmt() para cada instrução
 * e acumulando-as em um Body.
 *
 * @return Um Body contendo os statements do programa.
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
 * Dependendo do token lookahead, identifica e cria o nó correspondente, como
 * atribuição, definição de função, if, while, return, continue, break, ou declaração de canal.
 * Agora suporta atribuição a índices de arrays (ex.: dp[j] = valor).
 *
 * @return Ponteiro único para um nó (Node) representando o statement.
 * @throws SyntaxError se houver erro de sintaxe.
 */
std::unique_ptr<Node> Parser::stmt()
{
    LOG_DEBUG("Parser: Iniciando stmt(), lookahead: {tag: " << lookahead.getTag()
                                                            << ", value: " << lookahead.getValue()
                                                            << ", line: " << lineno << "}");

    if (lookahead.getTag() == "ID")
    {
        LOG_DEBUG("Parser: Detectado ID, processando...");
        std::string id_name = lookahead.getValue();
        match("ID");
        skipWhitespace();

        if (lookahead.getTag() == "LBRACK")
        {
            match("LBRACK");
            auto index = disjunction();
            if (!match("RBRACK"))
            {
                throw SyntaxError(lineno, "Esperado ']' no lugar de " + lookahead.getValue());
            }
            skipWhitespace();
            if (!match("ASSIGN"))
            {
                throw SyntaxError(lineno, "Esperado '=' após acesso a índice em lugar de " + lookahead.getValue());
            }
            skipWhitespace();
            auto right = arithmetic();
            skipWhitespace();
            auto access = std::make_unique<Access>("", Token("ACCESS", "[]"),
                                                   std::make_unique<ID>("", Token("ID", id_name)),
                                                   std::move(index));
            return std::make_unique<Assign>(std::move(access), std::move(right));
        }
        else if (lookahead.getTag() == "COLON")
        {
            match("COLON");
            skipWhitespace();
            if (lookahead.getValue() == "array")
            {
                match("TYPE"); // "array" deve ser reconhecido como "TYPE" no lexer
                skipWhitespace();
                if (lookahead.getTag() == "LBRACK")
                {
                    match("LBRACK");
                    auto size_expr = disjunction(); // Parse da expressão de tamanho
                    if (!match("RBRACK"))
                    {
                        throw SyntaxError(lineno, "Esperado ']' após expressão de tamanho");
                    }
                    skipWhitespace();
                    // Opcional: permitir atribuição após a declaração, ex: arr : array[50] = [1,2,3]
                    if (lookahead.getTag() == "ASSIGN") {
                        match("ASSIGN"); // Consome o '='
                        skipWhitespace();
                        auto right = arithmetic(); // Analisa o lado direito da atribuição
                        skipWhitespace();
                    
                        // Passo 1: Criar a declaração do array
                        auto array_decl = std::make_unique<ArrayDecl>(id_name, std::move(size_expr));
                    
                        // Passo 2: Criar a atribuição usando o identificador do array
                        auto assign = std::make_unique<Assign>(
                            std::make_unique<ID>("ID", Token("ID", id_name)), // Lado esquerdo: referência ao array
                            std::move(right)                                  // Lado direito: valor a ser atribuído
                        );
                    
                        // Combinar declaração e atribuição em uma sequência
                        Body seq_body;
                        seq_body.push_back(std::move(array_decl));
                        seq_body.push_back(std::move(assign));
                        return std::make_unique<Seq>(std::make_unique<Body>(std::move(seq_body)));
                    } else {
                        // Apenas declaração, sem atribuição
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
                // Declaração de tipo simples: x : number = ...
                std::string type = lookahead.getValue();
                if (!match("TYPE"))
                {
                    throw SyntaxError(lineno, "Esperado um tipo após ':' em lugar de " + lookahead.getValue());
                }
                skipWhitespace();
                if (!match("ASSIGN"))
                {
                    throw SyntaxError(lineno, "Esperado '=' após tipo em lugar de " + lookahead.getValue());
                }
                skipWhitespace();
                auto right = arithmetic();
                skipWhitespace();
                auto id = std::make_unique<ID>(type, Token("ID", id_name), true);
                return std::make_unique<Assign>(std::move(id), std::move(right));
            }
        }
        else if (lookahead.getTag() == "ASSIGN")
        {
            // Atribuição simples: x = ...
            match("ASSIGN");
            skipWhitespace();
            auto right = arithmetic();
            skipWhitespace();
            auto id = std::make_unique<ID>("", Token("ID", id_name));
            return std::make_unique<Assign>(std::move(id), std::move(right));
        }
        else if (lookahead.getTag() == "LPAREN")
        {
            // Chamada de função: func()
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
            return std::make_unique<Call>("", Token("ID", id_name),
                                          std::make_unique<ID>("", Token("ID", id_name)),
                                          std::move(args), id_name);
        }
        else
        {
            LOG_DEBUG("Parser: Erro, esperado ':', '=', '[' ou '(' após identificador");
            throw SyntaxError(lineno, "Esperado ':', '=', '[' ou '(' após identificador em lugar de " + lookahead.getValue());
        }
    }
    else if (lookahead.getTag() == "FUNC")
    {
        LOG_DEBUG("Parser: Detectado FUNC, processando...");
        match("FUNC");
        std::string name = var("FUNC");
        Parameters params = this->params();
        if (!match("RARROW"))
        {
            throw SyntaxError(lineno, "Esperado '->' no lugar de " + lookahead.getValue());
        }
        std::string type = lookahead.getValue();
        if (!match("TYPE"))
        {
            throw SyntaxError(lineno, "Tipo de retorno inválido: " + lookahead.getValue());
        }
        skipWhitespace();
        if (!match("LBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
        }
        Body body;
        while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        {
            body.push_back(stmt());
        }
        if (!match("RBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
        }
        skipWhitespace();
        auto func_def = std::make_unique<FuncDef>(
            name, type, std::move(params),
            std::make_unique<Body>(std::move(body)));
        symtable->insert(name, Symbol(name, "FUNC"));
        return func_def;
    }
    else if (lookahead.getTag() == "IF")
    {
        LOG_DEBUG("Parser: Detectado IF, processando...");
        match("IF");
        if (!match("LPAREN"))
        {
            throw SyntaxError(lineno, "Esperado '(' no lugar de " + lookahead.getValue());
        }
        auto cond = disjunction();
        if (!match("RPAREN"))
        {
            throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
        }
        skipWhitespace();
        if (!match("LBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
        }
        Body body;
        while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        {
            body.push_back(stmt());
        }
        if (!match("RBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
        }
        skipWhitespace();

        std::unique_ptr<Body> else_stmt = nullptr;
        if (lookahead.getTag() == "ELSE")
        {
            LOG_DEBUG("Parser: Detectado ELSE, processando...");
            match("ELSE");
            skipWhitespace();
            if (!match("LBRACE"))
            {
                throw SyntaxError(lineno, "Esperado '{' após 'else' no lugar de " + lookahead.getValue());
            }
            Body else_body;
            while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
            {
                else_body.push_back(stmt());
            }
            if (!match("RBRACE"))
            {
                throw SyntaxError(lineno, "Esperado '}' após corpo do else no lugar de " + lookahead.getValue());
            }
            else_stmt = std::make_unique<Body>(std::move(else_body));
        }

        skipWhitespace();
        return std::make_unique<If>(
            std::move(cond),
            std::make_unique<Body>(std::move(body)),
            std::move(else_stmt));
    }
    else if (lookahead.getTag() == "ELSE")
    {
        LOG_DEBUG("Parser: Erro, 'else' encontrado sem 'if'");
        throw SyntaxError(lineno, "'else' encontrado sem 'if' correspondente");
    }
    else if (lookahead.getTag() == "WHILE")
    {
        LOG_DEBUG("Parser: Detectado WHILE, processando...");
        match("WHILE");
        if (!match("LPAREN"))
        {
            throw SyntaxError(lineno, "Esperado '(' no lugar de " + lookahead.getValue());
        }
        auto cond = disjunction();
        if (!match("RPAREN"))
        {
            throw SyntaxError(lineno, "Esperado ')' no lugar de " + lookahead.getValue());
        }
        skipWhitespace();
        if (!match("LBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
        }
        Body body;
        while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        {
            body.push_back(stmt());
        }
        if (!match("RBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '}' no lugar de " + lookahead.getValue());
        }
        skipWhitespace();
        return std::make_unique<While>(std::move(cond), std::make_unique<Body>(std::move(body)));
    }
    else if (lookahead.getTag() == "RETURN")
    {
        LOG_DEBUG("Parser: Detectado RETURN, processando...");
        match("RETURN");
        skipWhitespace();
        auto expr = disjunction();
        skipWhitespace();
        return std::make_unique<Return>(std::move(expr));
    }
    else if (lookahead.getTag() == "BREAK")
    {
        LOG_DEBUG("Parser: Detectado BREAK, processando...");
        match("BREAK");
        skipWhitespace();
        return std::make_unique<Break>();
    }
    else if (lookahead.getTag() == "CONTINUE")
    {
        LOG_DEBUG("Parser: Detectado CONTINUE, processando...");
        match("CONTINUE");
        skipWhitespace();
        return std::make_unique<Continue>();
    }
    else if (lookahead.getTag() == "SEQ")
    {
        LOG_DEBUG("Parser: Detectado SEQ, processando...");
        match("SEQ");
        skipWhitespace();
        if (!match("LBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '{' após 'seq' no lugar de " + lookahead.getValue());
        }
        Body body;
        while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        {
            body.push_back(stmt());
        }
        if (!match("RBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '}' após corpo do seq no lugar de " + lookahead.getValue());
        }
        skipWhitespace();
        return std::make_unique<Seq>(std::make_unique<Body>(std::move(body)));
    }
    else if (lookahead.getTag() == "PAR")
    {
        LOG_DEBUG("Parser: Detectado PAR, processando...");
        match("PAR");
        skipWhitespace();
        if (!match("LBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '{' após 'par' no lugar de " + lookahead.getValue());
        }
        Body body;
        while (lookahead.getTag() != "RBRACE" && lookahead.getTag() != "EOF")
        {
            body.push_back(stmt());
        }
        if (!match("RBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '}' após corpo do par no lugar de " + lookahead.getValue());
        }
        skipWhitespace();
        return std::make_unique<Par>(std::make_unique<Body>(std::move(body)));
    }
    else if (lookahead.getTag() == "C_CHANNEL")
    {
        LOG_DEBUG("Parser: Detectado C_CHANNEL, processando...");
        match("C_CHANNEL");
        std::string name = lookahead.getValue();
        if (!match("ID"))
        {
            throw SyntaxError(lineno, "Esperado identificador para c_channel em lugar de " + lookahead.getValue());
        }
        skipWhitespace();
        if (!match("LBRACE"))
        {
            throw SyntaxError(lineno, "Esperado '{' no lugar de " + lookahead.getValue());
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
        return std::make_unique<CChannel>(name, std::move(localhost), std::move(port));
    }
    else if (lookahead.getTag() == "S_CHANNEL")
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
        return std::make_unique<SChannel>(
            name,
            std::move(localhost),
            std::move(port),
            func_name,
            std::move(desc));
    }
    else if (lookahead.getTag() == "FOR")
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
        {
            body.push_back(stmt());
        }
        match("RBRACE");

        Body while_body;
        for (auto &stmt : body)
        {
            while_body.push_back(std::move(stmt));
        }
        while_body.push_back(std::move(incr));

        Body for_body;
        for_body.push_back(std::move(init));
        for_body.push_back(std::make_unique<While>(
            std::move(cond),
            std::make_unique<Body>(std::move(while_body))));
        return std::make_unique<Seq>(std::make_unique<Body>(std::move(for_body)));
    }
    else
    {
        LOG_DEBUG("Parser: Erro, token inesperado: " << lookahead.getTag());
        throw SyntaxError(lineno, lookahead.getValue() + " não inicia instrução válida");
    }
}

/**
 * @brief Analisa a lista de parâmetros de uma função.
 *
 * Espera que os parâmetros estejam entre parênteses e separados por vírgulas.
 *
 * @return Parameters, que é um mapa contendo o nome do parâmetro e um par com o tipo e um valor padrão (opcional).
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
 * Pode opcionalmente ter um valor padrão.
 *
 * @return Um par contendo o nome do parâmetro e um par com o tipo e um ponteiro para a expressão do valor padrão (se houver).
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
 * Constrói uma árvore de expressão utilizando o operador OR, chamando o método conjunction() para os operandos.
 *
 * @return Ponteiro único para uma expressão (Expression).
 */
std::unique_ptr<Expression> Parser::disjunction()
{
    skipWhitespace(); // Garante que se inicia com um token válido
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
 * Constrói uma árvore de expressão utilizando o operador AND, chamando equality() para os operandos.
 *
 * @return Ponteiro único para uma expressão (Expression).
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
 * @brief Analisa uma expressão que pode representar uma variável ou chamada de função.
 *
 * Se o identificador for seguido por '(', interpreta como uma chamada de função;
 * se for seguido por ':' indica declaração com tipo.
 *
 * @return Ponteiro único para uma expressão (Expression) representando a variável, chamada ou declaração.
 */
std::unique_ptr<Expression> Parser::local()
{
    std::string name = lookahead.getValue();
    match("ID");

    // Verifica se é uma chamada de função
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

    // Se vier o ':' para declaração de variável com tipo
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
 * @brief Retorna o valor de um identificador após uma palavra-chave (contexto) específica.
 *
 * @param context Contexto em que o identificador é esperado (por exemplo, "FUNC").
 * @return Uma string com o nome do identificador.
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
 * Constrói uma árvore de expressão utilizando os operadores de igualdade,
 * combinando expressões de comparação.
 *
 * @return Ponteiro único para uma expressão (Expression).
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
 * Constrói uma árvore de expressão utilizando os operadores de comparação,
 * combinando expressões aritméticas.
 *
 * @return Ponteiro único para uma expressão (Expression).
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
 * Constrói uma árvore de expressão para operações de soma e subtração,
 * combinando termos.
 *
 * @return Ponteiro único para uma expressão (Expression).
 */
std::unique_ptr<Expression> Parser::arithmetic()
{
    auto left = term();
    while (lookahead.getTag() == "+" || lookahead.getTag() == "-")
    {
        std::string op = lookahead.getTag();
        match(op);
        auto right = term();
        left = std::make_unique<Arithmetic>("NUMBER", Token(op, op), std::move(left), std::move(right));
    }
    return left;
}

/**
 * @brief Analisa uma expressão aritmética de multiplicação e divisão.
 *
 * Constrói uma árvore de expressão para operações de multiplicação e divisão,
 * combinando termos.
 *
 * @return Ponteiro único para uma expressão (Expression).
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
            left = std::make_unique<Arithmetic>("NUMBER", token, std::move(left), std::move(right));
        }
    }
    return left;
}

/**
 * @brief Analisa uma expressão unária.
 *
 * Trata os operadores unários, como negação aritmética e lógica, e chama primary()
 * para as expressões literais.
 *
 * @return Ponteiro único para uma expressão (Expression).
 */
std::unique_ptr<Expression> Parser::unary()
{
    if (lookahead.getTag() == "-")
    {
        Token token = lookahead;
        match("-");
        auto expr = unary();
        return std::make_unique<Unary>("NUMBER", token, std::move(expr));
    }
    else if (lookahead.getTag() == "!")
    {
        Token token = lookahead;
        match("!");
        auto expr = unary();
        return std::make_unique<Unary>("BOOL", token, std::move(expr));
    }
    return primary();
}

/**
 * @brief Retorna o próximo token sem avançar a posição.
 *
 * @return O token seguinte ou EOF se não houver.
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
 * acesso a elementos de array e expressões agrupadas em parênteses.
 *
 * @return Ponteiro único para uma expressão (Expression).
 * @throws SyntaxError se nenhum literal, identificador ou expressão válida for encontrada.
 */
std::unique_ptr<Expression> Parser::primary()
{
    if (lookahead.getTag() == "NUMBER")
    {
        std::string value = lookahead.getValue();
        match("NUMBER");
        return std::make_unique<Constant>("NUMBER", Token("NUMBER", value));
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
            return std::make_unique<Access>("STRING", Token("ACCESS", "[]"),
                                            std::make_unique<ID>("", Token("ID", name)),
                                            std::move(index));
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