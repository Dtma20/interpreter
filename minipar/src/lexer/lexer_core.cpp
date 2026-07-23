/**
 * @file lexer_core.cpp
 * @brief Implementação do analisador léxico para a linguagem Minipar.
 *
 * Este arquivo implementa a classe Lexer, responsável por ler o código fonte e
 * convertê-lo em uma sequência de tokens. Utiliza expressões regulares para
 * identificar padrões e agrupar caracteres em tokens, além de manter o controle
 * de linhas.
 */

#include "../../include/lexer/lexer_core.hpp"
#include "../../include/lexer/lexer_rules.hpp"
#include <regex>
#include <algorithm>
#include <iostream>
#include "../../include/debug.hpp"

/**
 * @brief Aplica processamento de escape em valor de string (T13).
 *
 * Reconhece: \\, \", \n, \t, \r. Outros escapes mantêm o caractere
 * após a barra. Consistente com unescape_string() do módulo interpreter.
 */
static std::string unescape_lexer(const std::string &input)
{
    std::string result;
    for (size_t i = 0; i < input.length(); ++i)
    {
        if (input[i] == '\\' && i + 1 < input.length())
        {
            ++i;
            switch (input[i])
            {
            case 'n':
                result += '\n';
                break;
            case 't':
                result += '\t';
                break;
            case 'r':
                result += '\r';
                break;
            case '\\':
                result += '\\';
                break;
            case '"':
                result += '"';
                break;
            default:
                result += input[i];
                break;
            }
        }
        else
        {
            result += input[i];
        }
    }
    return result;
}

/**
 * @brief Construtor do Lexer.
 *
 * Inicializa o lexer com o código fonte fornecido e define a linha inicial como 1.
 * Também chama a função para inicializar a tabela de tokens (token_table).
 *
 * @param data String contendo o código fonte a ser analisado.
 */
Lexer::Lexer(const std::string &data) : data(data), line(1)
{
    initializeTokenTable(token_table);
}

/**
 * @brief Varredura prévia para detectar strings e comentários não terminados (T13).
 *
 * Percorre data caractere a caractere, rastreando estado de string e
 * comentário de bloco. Lança SyntaxError com a linha de início se encontrar
 * uma construção não terminada (fim de arquivo ou quebra de linha dentro de
 * string). Também rastreia corretamente escapes (\\) e comentários de linha (#).
 *
 * @throws SyntaxError se string ou comentário de bloco não estiver fechado.
 */
void Lexer::detect_unterminated()
{
    bool in_string = false;
    bool in_mcomment = false;
    bool escape_next = false;
    int string_start_line = 0;
    int mcomment_start_line = 0;
    int current_line = 1;

    for (size_t i = 0; i < data.size(); ++i)
    {
        char c = data[i];

        if (c == '\n')
        {
            current_line++;
        }

        if (in_string)
        {
            if (c == '\n')
            {
                throw SyntaxError(string_start_line,
                                  "string não terminada (quebra de linha dentro do literal)");
            }
            if (escape_next)
            {
                escape_next = false;
                continue;
            }
            if (c == '\\')
            {
                escape_next = true;
                continue;
            }
            if (c == '"')
            {
                in_string = false;
                continue;
            }
            continue;
        }
        if (in_mcomment)
        {
            if (c == '*' && i + 1 < data.size() && data[i + 1] == '/')
            {
                in_mcomment = false;
                ++i;
            }
            continue;
        }
        if (c == '"')
        {
            in_string = true;
            string_start_line = current_line;
            escape_next = false;
            continue;
        }
        if (c == '/' && i + 1 < data.size() && data[i + 1] == '*')
        {
            in_mcomment = true;
            mcomment_start_line = current_line;
            ++i; // consome '*'
            continue;
        }
        if (c == '#')
        {
            while (i + 1 < data.size() && data[i + 1] != '\n')
            {
                ++i;
            }
            continue;
        }
    }
    if (in_string)
    {
        throw SyntaxError(string_start_line,
                          "string não terminada (fim de arquivo antes do fechamento)");
    }
    if (in_mcomment)
    {
        throw SyntaxError(mcomment_start_line,
                          "comentário de bloco não terminado (fim de arquivo antes de */)");
    }
}

/**
 * @brief Executa a análise léxica no código fonte.
 *
 * Antes da tokenização:
 * 1. Reseta contador de linha (scan() repetível, T13).
 * 2. Varre data para detectar strings/comentários não terminados
 *    e lança SyntaxError se necessário (T13).
 *
 * Utiliza expressão regular compilada uma única vez (static const,
 * std::regex::optimize) para identificar tokens. Valores de string
 * têm aspas removidas e escapes processados (unescape_lexer, T13).
 *
 * @return std::vector<std::pair<Token, int>> Vetor de pares (token, linha).
 */
std::vector<std::pair<Token, int>> Lexer::scan()
{
    // T13: reset para comportamento repetível
    line = 1;

    LOG_DEBUG("Lexer: Iniciando scan(), tamanho da entrada: " << data.size());
    std::vector<std::pair<Token, int>> tokens;

    if (data.empty())
    {
        std::cerr << "Erro: Código fonte vazio em Lexer::scan\n";
        LOG_DEBUG("Lexer: Erro, código fonte vazio");
        return tokens;
    }
    if (TOKEN_PATTERNS.empty())
    {
        std::cerr << "Erro: TOKEN_PATTERNS vazio\n";
        LOG_DEBUG("Lexer: Erro, TOKEN_PATTERNS vazio");
        return tokens;
    }

    // T13: pré-varredura — detecta construções não terminadas
    try
    {
        detect_unterminated();
    }
    catch (const SyntaxError &)
    {
        throw; // propaga para main.cpp
    }

    try
    {
        // T13: compila regex uma única vez com optimize
        LOG_DEBUG("Lexer: Compilando expressão regular TOKEN_REGEX (static)");
        static const std::regex compiled_regex(TOKEN_REGEX, std::regex::optimize);

        auto words_begin = std::sregex_iterator(data.begin(), data.end(), compiled_regex);
        auto words_end = std::sregex_iterator();

        if (words_begin == words_end)
        {
            std::cerr << "Aviso: Nenhum token encontrado em '" << data << "'\n";
            LOG_DEBUG("Lexer: Nenhum token encontrado na entrada: " << data);
        }

        for (std::sregex_iterator it = words_begin; it != words_end; ++it)
        {
            std::smatch match = *it;
            if (!match.ready())
            {
                std::cerr << "Erro: Match não está pronto\n";
                LOG_DEBUG("Lexer: Erro, match não está pronto");
                continue;
            }

            std::string type = "";
            std::string value = match.str();
            LOG_DEBUG("Lexer: Processando match, valor bruto: " << value);

            for (size_t i = 1; i < match.size() && i <= TOKEN_PATTERNS.size(); ++i)
            {
                if (match[i].matched)
                {
                    type = TOKEN_PATTERNS[i - 1].first;
                    LOG_DEBUG("Lexer: Tipo identificado: " << type);
                    break;
                }
            }

            if (type.empty())
            {
                std::cerr << "Aviso: Tipo de token não identificado para '" << value << "'\n";
                LOG_DEBUG("Lexer: Tipo não identificado para valor: " << value);
                continue;
            }

            if (type == "WHITESPACE")
            {
                LOG_DEBUG("Lexer: Ignorando WHITESPACE");
                continue;
            }
            else if (type == "NEWLINE")
            {
                line++;
                LOG_DEBUG("Lexer: NEWLINE detectado, linha atual: " << line);
                continue;
            }
            else if (type == "SCOMMENT" || type == "MCOMMENT")
            {
                if (type == "MCOMMENT")
                {
                    line += std::count(value.begin(), value.end(), '\n');
                    LOG_DEBUG("Lexer: MCOMMENT detectado, linhas incrementadas para: " << line);
                }
                else
                {
                    LOG_DEBUG("Lexer: SCOMMENT detectado");
                }
                continue;
            }
            else if (type == "NAME")
            {
                auto it = token_table.find(value);
                type = (it != token_table.end()) ? it->second : "ID";
                LOG_DEBUG("Lexer: NAME ajustado para: " << type);
            }
            else if (type == "STRING")
            {
                // T13: remove aspas e processa escapes
                if (value.size() >= 2)
                {
                    std::string raw = value.substr(1, value.size() - 2);
                    value = unescape_lexer(raw);
                    LOG_DEBUG("Lexer: STRING ajustada, valor final: " << value);
                }
                else
                {
                    std::cerr << "Aviso: String malformada '" << value << "'\n";
                    LOG_DEBUG("Lexer: STRING malformada: " << value);
                }
            }
            else if (type == "OTHER")
            {
                type = value;
                LOG_DEBUG("Lexer: OTHER ajustado para: " << type);
            }

            tokens.emplace_back(Token(type, value), line);
            LOG_DEBUG("Lexer: Token gerado: {tag: " << type << ", value: " << value << ", line: " << line << "}");
        }
    }
    catch (const std::regex_error &e)
    {
        std::cerr << "Erro na construção da regex: " << e.what() << " (código: " << e.code() << ")\n";
        LOG_DEBUG("Lexer: Erro na construção da regex: " << e.what());
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erro inesperado no Lexer: " << e.what() << "\n";
        LOG_DEBUG("Lexer: Erro inesperado: " << e.what());
    }

    LOG_DEBUG("Lexer: Finalizando scan(), total de tokens: " << tokens.size());
    return tokens;
}
