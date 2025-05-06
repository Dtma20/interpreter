#include "../include/error.hpp"
#include <sstream>

/**
 * @brief Construtor para exceção de erro de sintaxe.
 *
 * Cria uma exceção do tipo SyntaxError indicando um erro de sintaxe,
 * incluindo a linha onde o erro ocorreu e uma mensagem descritiva.
 *
 * @param line Número da linha em que o erro de sintaxe foi detectado.
 * @param msg Mensagem detalhada descrevendo o erro.
 */
SyntaxError::SyntaxError(int line, const std::string &msg)
    : std::runtime_error("linha " + std::to_string(line) + ": " + msg) {}

/**
 * @brief Construtor para exceção de erro semântico.
 *
 * Cria uma exceção do tipo SemanticError indicando um erro de semântica,
 * com uma mensagem descritiva do problema ocorrido.
 *
 * @param msg Mensagem detalhada descrevendo o erro semântico.
 */
SemanticError::SemanticError(const std::string &msg)
    : std::runtime_error(msg) {}

/**
 * @brief Construtor para exceção de erro em tempo de execução.
 *
 * Cria uma exceção do tipo RunTimeError indicando um erro ocorrido durante a execução,
 * com uma mensagem explicativa do problema.
 *
 * @param msg Mensagem detalhada descrevendo o erro em tempo de execução.
 */
RunTimeError::RunTimeError(const std::string &msg)
    : std::runtime_error(msg) {}