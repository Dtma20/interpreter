#include "../include/token.hpp"
#include <regex>

/**
 * @brief Construtor padrão da classe Token.
 *
 * Inicializa a tag e o valor do token como strings vazias.
 */
Token::Token() : tag(""), value("") {}

/**
 * @brief Construtor parametrizado da classe Token.
 *
 * Inicializa o token com a tag e o valor informados.
 *
 * @param tag Tipo ou categoria do token.
 * @param value Valor textual do token.
 */
Token::Token(const std::string &tag, const std::string &value) : tag(tag), value(value) {}

/**
 * @brief Retorna a tag do token.
 *
 * @return A tag (tipo) do token.
 */
std::string Token::getTag() const { return tag; }

/**
 * @brief Retorna o valor do token.
 *
 * @return O valor textual do token.
 */
std::string Token::getValue() const { return value; }

/**
 * @brief Converte o token para uma representação em string.
 *
 * @return Uma string no formato "{{value, tag}}" representando o token.
 */
std::string Token::toString() const { return "{{" + value + ", " + tag + "}}"; }

/**
 * @brief Padrões de tokens.
 *
 * Cada par contém:
 *  - O nome do tipo de token (por exemplo, "NAME", "NUMBER", "STRING", etc.)
 *  - A expressão regular que define o padrão para esse token.
 */
const std::vector<std::pair<std::string, std::string>> TOKEN_PATTERNS = {
    {"NAME", "[A-Za-z_][A-Za-z0-9_]*"},
    {"NUM", "\\b\\d+\\.\\d+|\\.\\d+|\\d+\\b"},
    {"RARROW", "->"},
    {"STRING", "\"(?:[^\"]*)\""},
    {"SCOMMENT", "#.*"},
    {"MCOMMENT", "/\\*[\\s\\S]*?\\*/"},
    {"OR", "\\|\\|"},
    {"AND", "&&"},
    {"EQ", "=="},
    {"NEQ", "!="},
    {"LTE", "<="},
    {"GTE", ">="},
    {"COLON", ":"},
    {"ASSIGN", "="},
    {"LPAREN", "\\("},
    {"RPAREN", "\\)"},
    {"LBRACK", "\\["},
    {"RBRACK", "\\]"},
    {"LBRACE", "\\{"},
    {"RBRACE", "\\}"},
    {"NEWLINE", "\n"},
    {"WHITESPACE", "\\s+"},
    {"FOR", "for"},
    {"INC", "\\+\\+"},
    {"DEC", "--"},
    {"OTHER", "."}};

/**
 * @brief Conjunto de tokens que iniciam statements.
 *
 * Define os tipos de tokens que representam o início de uma instrução,
 * facilitando a identificação de blocos de código durante a análise.
 */
const std::unordered_set<std::string> STATEMENT_TOKENS = {
    "ID", "FUNC", "IF", "ELSE", "WHILE", "RETURN", "BREAK", "CONTINUE",
    "SEQ", "PAR", "C_CHANNEL", "S_CHANNEL", "FOR"};

/**
 * @brief Funções padrão e seus tipos de retorno.
 *
 * Mapeia o nome das funções padrão para o tipo de retorno esperado.
 * Essas funções são automaticamente registradas no ambiente.
 */
const std::unordered_map<std::string, std::string> DEFAULT_FUNCTION_NAMES = {
    {"print", "VOID"},
    {"input", "STRING"},
    {"sleep", "VOID"},
    {"to_number", "NUMBER"},
    {"to_string", "STRING"},
    {"to_bool", "BOOL"},
    {"send", "STRING"},
    {"close", "VOID"},
    {"len", "NUMBER"},
    {"isalpha", "BOOL"},
    {"isnum", "BOOL"},
};

/**
 * @brief Expressão regular combinada para tokens.
 *
 * Constrói uma única expressão regular concatenando os padrões definidos em TOKEN_PATTERNS,
 * separando-os pelo operador '|' (ou lógico) para facilitar a identificação dos tokens.
 */
const std::string TOKEN_REGEX = []()
{
    std::string regex;
    for (const auto &pair : TOKEN_PATTERNS)
    {
        if (!regex.empty())
        {
            regex += "|";
        }
        regex += "(" + pair.second + ")";
    }
    return regex;
}();