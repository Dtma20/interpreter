#pragma once

#include <memory>
#include <string>
#include <vector>
#include "ast_base.hpp"

/**
 * @brief Representa um identificador (variável) na AST.
 */
class ID : public Expression
{
public:
    ID(const std::string &type, const Token &token, bool decl = false);
    bool isDecl() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    bool decl;
};

/**
 * @brief Representa acesso a índice (array/string) na AST.
 */
class Access : public Expression
{
public:
    Access(const std::string &type, const Token &token, std::unique_ptr<Expression> base, std::unique_ptr<Expression> index);
    Expression *getBase() const;
    Expression *getIndex() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> base;
    std::unique_ptr<Expression> index;
};

/**
 * @brief Representa uma operação lógica (AND, OR) na AST.
 */
class Logical : public Expression
{
public:
    Logical(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
    Expression *getLeft() const;
    Expression *getRight() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

/**
 * @brief Representa uma operação relacional (==, !=, <, >, <=, >=) na AST.
 */
class Relational : public Expression
{
public:
    Relational(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
    Expression *getLeft() const;
    Expression *getRight() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

/**
 * @brief Representa uma operação aritmética (+, -, *, /) na AST.
 */
class Arithmetic : public Expression
{
public:
    Arithmetic(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
    Expression *getLeft() const;
    Expression *getRight() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

/**
 * @brief Representa uma operação unária (-, !, ++, --) na AST.
 */
class Unary : public Expression
{
public:
    Unary(const std::string &type, const Token &token, std::unique_ptr<Expression> expr, bool isPostfix = false);
    Expression *getExpr() const;
    bool isPostfix() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> expr;
    bool m_isPostfix;
};

/**
 * @brief Representa uma chamada de função na AST.
 */
class Call : public Expression
{
public:
    Call(const std::string &type, const Token &token, std::unique_ptr<Expression> base, Arguments args, const std::string &oper);
    Expression *getBase() const;
    const Arguments &getArgs() const;
    std::string getOper() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> base;
    Arguments args;
    std::string oper;
};
