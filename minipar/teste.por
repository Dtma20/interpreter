CLASSE Lexer:
    PROPRIEDADES:
        data: string              // Código fonte de entrada
        line: int                 // Contador de linhas (inicia em 1)
        token_table: map<string, string>  // Tabela de palavras-chave (ex: "func" → "FUNC")
        TOKEN_PATTERNS: vector<pair<string, string>>  // Padrões regex para tokens
        TOKEN_REGEX: string       // Regex combinada para todos os padrões

    MÉTODO Construtor(data: string):
        Inicializa data e line
        Chama initializeTokenTable()

    MÉTODO initializeTokenTable():
        Preenche token_table com pares (palavra, tag):
            Exemplo:
                token_table["if"] = "IF"
                token_table["num"] = "TYPE"

    MÉTODO scan() → vector<pair<Token, int>>:
        SE data.empty():
            Log de erro "Código fonte vazio"
            Retorna vetor vazio

        SE TOKEN_PATTERNS.empty():
            Log de erro "Padrões de token não definidos"
            Retorna vetor vazio

        INICIALIZA:
            tokens: vector<pair<Token, int>>  // Saída final
            compiled_regex: regex(TOKEN_REGEX)  // Compila a regex combinada
            words_begin: sregex_iterator(data.begin(), data.end(), compiled_regex)
            words_end: sregex_iterator()

        SE words_begin == words_end:
            Log de aviso "Nenhum token encontrado"
            Retorna tokens

        PARA CADA match EM [words_begin, words_end):
            value: string = match.str()  // Valor bruto do token
            kind: string = ""            // Tipo do token (ex: "NUM", "STRING")

            // Determina o tipo baseado no grupo capturado na regex
            PARA i DE 1 ATÉ match.size():
                SE match[i].matched:
                    kind = TOKEN_PATTERNS[i-1].first  // Obtém o tipo do padrão
                    BREAK

            SE kind == "":
                Log de aviso "Tipo não identificado para {value}"
                CONTINUE

            // Processamento Específico por Tipo de Token
            SE kind == "WHITESPACE" OU kind == "NEWLINE":
                SE kind == "NEWLINE":
                    line += 1
                CONTINUE  // Ignora espaços e quebras de linha

            SE kind == "SCOMMENT" OU kind == "MCOMMENT":
                SE kind == "MCOMMENT":
                    line += count(value, '\n')  // Atualiza linhas em comentários multi-linha
                CONTINUE

            SE kind == "NAME":
                // Verifica se é palavra-chave
                SE token_table.contains(value):
                    kind = token_table[value]  // Substitui por tag reservada (ex: "IF")
                SENÃO:
                    kind = "ID"  // Identificador comum

            SE kind == "STRING":
                SE value.size() >= 2:
                    value = value.substr(1, value.size()-2)  // Remove aspas
                SENÃO:
                    Log de aviso "String malformada"

            SE kind == "OTHER":
                kind = value  // Operadores como "+", "=" usam o próprio valor como tipo

            // Adiciona token à lista
            tokens.emplace_back(Token(kind, value), line)

        RETORNA tokens

    TRATAMENTO DE ERROS:
        CAPTURA regex_error:
            Log de erro "Erro na regex: {e.what()}"
        CAPTURA exception genérica:
            Log de erro "Erro inesperado: {e.what()}"