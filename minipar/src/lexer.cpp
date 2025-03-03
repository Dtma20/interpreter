#include "../include/lexer.hpp"
#include <regex>
#include <algorithm>
#include <iostream>
#include "../include/debug.hpp"

Lexer::Lexer(const std::string& data) : data(data), line(1) {
    initializeTokenTable();
}

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
    token_table["for"] = "FOR";
}

std::vector<std::pair<Token, int>> Lexer::scan() {
    LOG_DEBUG("Lexer: Iniciando scan(), tamanho da entrada: " << data.size());
    std::vector<std::pair<Token, int>> tokens;

    if (data.empty()) {
        std::cerr << "Erro: Código fonte vazio em Lexer::scan\n";
        LOG_DEBUG("Lexer: Erro, código fonte vazio");
        return tokens;
    }
    if (TOKEN_PATTERNS.empty()) {
        std::cerr << "Erro: TOKEN_PATTERNS vazio\n";
        LOG_DEBUG("Lexer: Erro, TOKEN_PATTERNS vazio");
        return tokens;
    }

    try {
        LOG_DEBUG("Lexer: Compilando expressão regular TOKEN_REGEX");
        std::regex compiled_regex(TOKEN_REGEX);
        auto words_begin = std::sregex_iterator(data.begin(), data.end(), compiled_regex);
        auto words_end = std::sregex_iterator();

        if (words_begin == words_end) {
            std::cerr << "Aviso: Nenhum token encontrado em '" << data << "'\n";
            LOG_DEBUG("Lexer: Nenhum token encontrado na entrada: " << data);
        }

        for (std::sregex_iterator it = words_begin; it != words_end; ++it) {
            std::smatch match = *it;
            if (!match.ready()) {
                std::cerr << "Erro: Match não está pronto\n";
                LOG_DEBUG("Lexer: Erro, match não está pronto");
                continue;
            }

            std::string kind = "";
            std::string value = match.str();
            LOG_DEBUG("Lexer: Processando match, valor bruto: " << value);

            for (size_t i = 1; i < match.size() && i <= TOKEN_PATTERNS.size(); ++i) {
                if (match[i].matched) {
                    kind = TOKEN_PATTERNS[i - 1].first;
                    LOG_DEBUG("Lexer: Tipo identificado: " << kind);
                    break;
                }
            }

            if (kind.empty()) {
                std::cerr << "Aviso: Tipo de token não identificado para '" << value << "'\n";
                LOG_DEBUG("Lexer: Tipo não identificado para valor: " << value);
                continue;
            }

            if (kind == "WHITESPACE") {
                LOG_DEBUG("Lexer: Ignorando WHITESPACE");
                continue;
            } else if (kind == "NEWLINE") {
                line++;
                LOG_DEBUG("Lexer: NEWLINE detectado, linha atual: " << line);
                continue;
            } else if (kind == "SCOMMENT" || kind == "MCOMMENT") {
                if (kind == "MCOMMENT") {
                    line += std::count(value.begin(), value.end(), '\n');
                    LOG_DEBUG("Lexer: MCOMMENT detectado, linhas incrementadas para: " << line);
                } else {
                    LOG_DEBUG("Lexer: SCOMMENT detectado");
                }
                continue;
            } else if (kind == "NAME") {
                auto it = token_table.find(value);
                kind = (it != token_table.end()) ? it->second : "ID";
                LOG_DEBUG("Lexer: NAME ajustado para: " << kind);
            } else if (kind == "STRING") {
                if (value.size() >= 2) {
                    value = value.substr(1, value.size() - 2);
                    LOG_DEBUG("Lexer: STRING ajustada, valor final: " << value);
                } else {
                    std::cerr << "Aviso: String malformada '" << value << "'\n";
                    LOG_DEBUG("Lexer: STRING malformada: " << value);
                }
            } else if (kind == "OTHER") {
                kind = value;
                LOG_DEBUG("Lexer: OTHER ajustado para: " << kind);
            }

            tokens.emplace_back(Token(kind, value), line);
            LOG_DEBUG("Lexer: Token gerado: {tag: " << kind << ", value: " << value << ", line: " << line << "}");
        }
    } catch (const std::regex_error& e) {
        std::cerr << "Erro na construção da regex: " << e.what() << " (código: " << e.code() << ")\n";
        LOG_DEBUG("Lexer: Erro na construção da regex: " << e.what());
    } catch (const std::exception& e) {
        std::cerr << "Erro inesperado no Lexer: " << e.what() << "\n";
        LOG_DEBUG("Lexer: Erro inesperado: " << e.what());
    }

    LOG_DEBUG("Lexer: Finalizando scan(), total de tokens: " << tokens.size());
    return tokens;
}