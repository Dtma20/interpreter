#ifndef LEXER_HPP
#define LEXER_HPP

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include "token.hpp"

/**
 * @brief Interface abstrata para o lexer da linguagem Minipar.
 *
 * Define o contrato básico que qualquer implementação de lexer deve seguir.
 * O objetivo principal é escanear uma entrada e retornar uma lista de tokens
 * associados às suas respectivas linhas no código-fonte.
 */
class ILexer
{
public:
    /**
     * @brief Destrutor virtual padrão.
     *
     * Garante que classes derivadas possam liberar recursos adequadamente
     * quando destruídas através de um ponteiro para ILexer.
     */
    virtual ~ILexer() = default;

    /**
     * @brief Escaneia a entrada e retorna uma lista de tokens com suas linhas.
     *
     * Método abstrato que deve ser implementado por classes derivadas para
     * realizar a tokenização do código-fonte.
     *
     * @return std::vector<std::pair<Token, int>> Lista de pares contendo cada token
     *         e o número da linha onde foi encontrado.
     */
    virtual std::vector<std::pair<Token, int>> scan() = 0;
};

/**
 * @brief Implementação concreta do lexer para a linguagem Minipar.
 *
 * Esta classe realiza a análise léxica de uma string de entrada, convertendo-a
 * em uma sequência de tokens com base em uma tabela de regras predefinida.
 * Os tokens são associados às suas respectivas linhas no código-fonte.
 */
class Lexer : public ILexer
{
public:
    /**
     * @brief Construtor do Lexer.
     *
     * Inicializa o lexer com o código-fonte a ser analisado e configura a linha inicial.
     *
     * @param data String contendo o código-fonte a ser tokenizado.
     */
    Lexer(const std::string &data);

    /**
     * @brief Escaneia o código-fonte e retorna a lista de tokens.
     *
     * Percorre a string de entrada, identifica padrões com base na tabela de tokens
     * e retorna uma lista de pares (token, linha). Ignora espaços em branco e
     * atualiza o contador de linhas ao encontrar quebras de linha.
     *
     * @return std::vector<std::pair<Token, int>> Lista de pares contendo cada token
     *         identificado e o número da linha correspondente.
     */
    std::vector<std::pair<Token, int>> scan() override;

private:
    std::string data; ///< Código-fonte a ser analisado.
    int line;         ///< Contador da linha atual durante a análise.
    
    /**
     * @brief Tabela de mapeamento de palavras-chave e tipos para tags de tokens.
     *
     * Armazena associações entre identificadores (como "if", "num") e suas
     * respectivas categorias ou tags (como "IF", "TYPE").
     */
    std::unordered_map<std::string, std::string> token_table;

    /**
     * @brief Inicializa a tabela de tokens.
     *
     * Preenche a token_table com palavras-chave, tipos e outros identificadores
     * reconhecidos pela linguagem Minipar, mapeando-os para suas tags correspondentes.
     */
    void initializeTokenTable();
};

#endif // LEXER_HPP