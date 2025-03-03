#include "../include/lexer.hpp"
#include <regex>
#include <algorithm>
#include <iostream>

/**
 * @brief Construtor do Lexer.
 *
 * Inicializa o lexer com o código fonte fornecido e configura o número da linha inicial.
 * Também inicializa a tabela de tokens com as palavras reservadas e tipos.
 *
 * @param data Código fonte a ser analisado.
 */
Lexer::Lexer(const std::string& data) : data(data), line(1) {
    initializeTokenTable();
}

/**
 * @brief Inicializa a tabela de tokens.
 *
 * Popula o dicionário (mapa) token_table com as palavras reservadas e seus respectivos tipos,
 * facilitando a identificação de palavras-chave durante a análise léxica.
 */
void Lexer::initializeTokenTable() {
    token_table["number"] = "TYPE";
    token_table["bool"] = "TYPE";
    token_table["string"] = "TYPE";
    token_table["void"] = "TYPE";
    token_table["true"] = "TRUE";
    token_table["false"] = "FALSE";
    token_table["func"] = "FUNC";
    token_table["while"] = "WHILE";
    token_table["if"] = "IF";
    token_table["else"] = "ELSE";
    token_table["return"] = "RETURN";
    token_table["break"] = "BREAK";
    token_table["continue"] = "CONTINUE";
    token_table["par"] = "PAR";
    token_table["seq"] = "SEQ";
    token_table["c_channel"] = "C_CHANNEL";
    token_table["s_channel"] = "S_CHANNEL";
}

/**
 * @brief Realiza a varredura (scan) do código fonte para extrair os tokens.
 *
 * Utiliza expressões regulares definidas (TOKEN_REGEX e TOKEN_PATTERNS) para identificar
 * os padrões de tokens no código fonte. Cada token encontrado é associado ao número da linha onde ocorreu.
 * São tratados comentários, espaços em branco, quebras de linha e literais de string.
 *
 * @return Vetor de pares contendo o token identificado e o número da linha em que ele foi encontrado.
 */
std::vector<std::pair<Token, int>> Lexer::scan() {
    std::vector<std::pair<Token, int>> tokens;

    if (data.empty()) {
        std::cerr << "Erro: Código fonte vazio em Lexer::scan\n";
        return tokens;
    }
    if (TOKEN_PATTERNS.empty()) {
        std::cerr << "Erro: TOKEN_PATTERNS vazio\n";
        return tokens;
    }

    try {
        std::regex compiled_regex(TOKEN_REGEX);
        auto words_begin = std::sregex_iterator(data.begin(), data.end(), compiled_regex);
        auto words_end = std::sregex_iterator();

        if (words_begin == words_end) {
            std::cerr << "Aviso: Nenhum token encontrado em '" << data << "'\n";
        }

        for (std::sregex_iterator it = words_begin; it != words_end; ++it) {
            std::smatch match = *it;
            if (!match.ready()) {
                std::cerr << "Erro: Match não está pronto\n";
                continue;
            }

            std::string kind = "";
            std::string value = match.str();

            for (size_t i = 1; i < match.size() && i <= TOKEN_PATTERNS.size(); ++i) {
                if (match[i].matched) {
                    kind = TOKEN_PATTERNS[i - 1].first;
                    break;
                }
            }

            if (kind.empty()) {
                std::cerr << "Aviso: Tipo de token não identificado para '" << value << "'\n";
                continue;
            }

            if (kind == "WHITESPACE") {
                continue;
            } else if (kind == "NEWLINE") {
                line++;
                continue;
            } else if (kind == "SCOMMENT" || kind == "MCOMMENT") {
                if (kind == "MCOMMENT") {
                    line += std::count(value.begin(), value.end(), '\n');
                }
                continue;
            } else if (kind == "NAME") {
                auto it = token_table.find(value);
                kind = (it != token_table.end()) ? it->second : "ID";
            } else if (kind == "STRING") {
                if (value.size() >= 2) {
                    value = value.substr(1, value.size() - 2);
                } else {
                    std::cerr << "Aviso: String malformada '" << value << "'\n";
                }
            } else if (kind == "OTHER") {
                kind = value;
            }

            tokens.emplace_back(Token(kind, value), line);
        }
    } catch (const std::regex_error& e) {
        std::cerr << "Erro na construção da regex: " << e.what() << " (código: " << e.code() << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "Erro inesperado no Lexer: " << e.what() << "\n";
    }

    return tokens;
}