#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../token.hpp"

/**
 * @brief Declarações antecipadas das classes da AST.
 */
class Node;
class Expression;
class Statement;
class Constant;
class ID;
class Access;
class Logical;
class Relational;
class Arithmetic;
class Unary;
class Call;
class Array;
class ArrayDecl;
class Module;
class Assign;
class Return;
class Break;
class Continue;
class FuncDef;
class If;
class While;
class Par;
class Seq;
class Channel;
class SChannel;
class CChannel;

/**
 * @brief Alias para um conjunto de nós representando um corpo de instruções.
 */
using Body = std::vector<std::unique_ptr<Node>>;

/**
 * @brief Alias para os argumentos passados em chamadas de função.
 */
using Arguments = std::vector<std::unique_ptr<Expression>>;

/**
 * @brief Alias para os parâmetros de funções.
 *
 * Cada entrada mapeia o nome do parâmetro para um par onde o primeiro elemento é o tipo
 * e o segundo é um ponteiro opcional para o valor padrão (como expressão).
 */
using ParamInfo  = std::pair<std::string, std::unique_ptr<Expression>>;
using ParamData  = std::pair<std::string, ParamInfo>;
using Parameters = std::vector<ParamData>;

/**
 * @brief Classe base para todos os nós da AST.
 */
class Node
{
public:
    virtual ~Node() = default;
    virtual std::vector<Node *> getAttributes() = 0;

    int getLine() const { return line_; }
    void setLine(int l) { line_ = l; }

protected:
    int line_ = 0;
};

/**
 * @brief Classe base para expressões na AST.
 *
 * Contém informações comuns, como o tipo da expressão e o token associado.
 */
class Expression : public Node
{
public:
    Expression() = default;
    Expression(const std::string &type, const Token &token);
    virtual ~Expression() = default;

    const std::string& getType() const;
    const Token& getToken() const;
    const std::string& getName() const;

protected:
    std::string type;
    Token token;
};

/**
 * @brief Classe base para instruções na AST.
 */
class Statement : public Node
{
public:
    virtual ~Statement() = default;
};
