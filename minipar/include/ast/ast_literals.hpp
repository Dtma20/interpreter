#pragma once

#include <memory>
#include <string>
#include <vector>
#include "ast_base.hpp"

/**
 * @brief Representa uma constante literal na AST.
 */
class Constant : public Expression
{
public:
    Constant(const std::string &type, const Token &token);
    std::vector<Node *> getAttributes() override { return {}; }
};

/**
 * @brief Representa um array literal na AST.
 */
class Array : public Expression
{
public:
    std::vector<std::unique_ptr<Expression>> elements;

    Array(std::vector<std::unique_ptr<Expression>> elements)
        : elements(std::move(elements)) {}

    const std::vector<std::unique_ptr<Expression>> &getElements() const
    {
        return elements;
    }

    std::vector<Node *> getAttributes() override
    {
        std::vector<Node *> attrs;
        for (auto &elem : elements)
        {
            attrs.push_back(elem.get());
        }
        return attrs;
    }
};
