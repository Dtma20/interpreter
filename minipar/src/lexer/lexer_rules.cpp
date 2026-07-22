/**
 * @file lexer_rules.cpp
 * @brief Implementação da inicialização da tabela de tokens do lexer.
 *
 * Define as palavras-chave e tipos da linguagem Minipar que são mapeadas
 * para suas respectivas tags de token durante a análise léxica.
 */

#include "../../include/lexer/lexer_rules.hpp"

/**
 * @brief Inicializa a tabela de tokens.
 *
 * Preenche o mapa token_table com palavras-chave e seus respectivos tipos ou tags.
 * Essa tabela é utilizada para identificar se um token reconhecido como "NAME"
 * corresponde a uma palavra-chave da linguagem (como "if", "while", "func", etc.).
 */
void initializeTokenTable(std::unordered_map<std::string, std::string> &table)
{
    table["num"] = "TYPE";
    table["bool"] = "TYPE";
    table["string"] = "TYPE";
    table["void"] = "TYPE";
    table["array"] = "TYPE";
    table["true"] = "TRUE";
    table["false"] = "FALSE";
    table["func"] = "FUNC";
    table["while"] = "WHILE";
    table["if"] = "IF";
    table["else"] = "ELSE";
    table["return"] = "RETURN";
    table["break"] = "BREAK";
    table["continue"] = "CONTINUE";
    table["par"] = "PAR";
    table["seq"] = "SEQ";
    table["c_channel"] = "C_CHANNEL";
    table["s_channel"] = "S_CHANNEL";
    table["for"] = "FOR";
}
