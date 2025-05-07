#ifndef ERROR_HPP
#define ERROR_HPP

#pragma once
#include <stdexcept>
#include <string>

/**
 * @brief Exceção para erros de sintaxe.
 *
 * Lançada quando ocorre um erro de sintaxe durante o processo de análise do código fonte.
 */
class SyntaxError : public std::runtime_error
{
public:
    /**
     * @brief Construtor da exceção de erro de sintaxe.
     *
     * Cria uma mensagem de erro formatada com o número da linha e a descrição do problema.
     *
     * @param line Número da linha onde ocorreu o erro.
     * @param msg Descrição do erro de sintaxe.
     */
    SyntaxError(int line, const std::string &msg);
};

/**
 * @brief Exceção para erros semânticos.
 *
 * Lançada quando ocorre um erro de semântica durante a análise ou execução do código.
 */
class SemanticError : public std::runtime_error
{
public:
    /**
     * @brief Construtor da exceção de erro semântico.
     *
     * Cria uma mensagem de erro baseada na descrição do problema semântico encontrado.
     *
     * @param msg Descrição do erro semântico.
     */
    explicit SemanticError(const std::string &msg);
};

/**
 * @brief Exceção para erros em tempo de execução.
 *
 * Lançada quando ocorre um erro durante a execução do programa, como divisão por zero ou
 * referência a variável não definida.
 */
class RunTimeError : public std::runtime_error
{
public:
    /**
     * @brief Construtor da exceção de erro em tempo de execução.
     *
     * Cria uma mensagem de erro com a descrição do problema ocorrido durante a execução.
     *
     * @param msg Descrição do erro em tempo de execução.
     */
    explicit RunTimeError(const std::string &msg);
};

#endif // ERROR_HPP
