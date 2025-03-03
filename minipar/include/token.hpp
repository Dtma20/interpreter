#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <utility>

/**
 * @brief Representa um token na análise léxica.
 *
 * Um token contém uma etiqueta (tag) que indica sua categoria e um valor associado.
 */
class Token {
public:
    /**
     * @brief Construtor padrão que inicializa um token vazio.
     */
    Token();

    /**
     * @brief Construtor que inicializa um token com um tipo e valor específicos.
     * @param tag Categoria do token (por exemplo, "IDENTIFIER", "NUMBER").
     * @param value Valor associado ao token.
     */
    Token(const std::string& tag, const std::string& value);

    /**
     * @brief Obtém a categoria (tag) do token.
     * @return String representando a categoria do token.
     */
    std::string getTag() const;

    /**
     * @brief Obtém o valor do token.
     * @return String representando o valor do token.
     */
    std::string getValue() const;

    /**
     * @brief Retorna uma representação em string do token.
     * @return String formatada representando o token.
     */
    std::string toString() const;

private:
    std::string tag;   ///< Categoria do token.
    std::string value; ///< Valor associado ao token.
};

/**
 * @brief Lista de padrões de tokens usados pelo analisador léxico.
 *
 * Cada par representa um nome de token e sua expressão correspondente.
 */
extern const std::vector<std::pair<std::string, std::string>> TOKEN_PATTERNS;

/**
 * @brief Conjunto de tokens que representam declarações na linguagem.
 */
extern const std::unordered_set<std::string> STATEMENT_TOKENS;

/**
 * @brief Mapeamento de nomes de funções padrão para suas respectivas representações.
 */
extern const std::unordered_map<std::string, std::string> DEFAULT_FUNCTION_NAMES;

/**
 * @brief Expressão regular usada para tokenização do código-fonte.
 */
extern const std::string TOKEN_REGEX;

#endif // TOKEN_HPP
