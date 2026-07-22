#ifndef LEXER_RULES_HPP
#define LEXER_RULES_HPP

#pragma once
#include <unordered_map>
#include <string>

/**
 * @brief Inicializa a tabela de mapeamento de palavras-chave para tags de tokens.
 *
 * Preenche o mapa fornecido com palavras-chave, tipos e outros identificadores
 * reconhecidos pela linguagem Minipar, mapeando-os para suas tags correspondentes.
 *
 * @param table Referência para o mapa a ser preenchido com as regras de tokenização.
 */
void initializeTokenTable(std::unordered_map<std::string, std::string> &table);

#endif // LEXER_RULES_HPP
