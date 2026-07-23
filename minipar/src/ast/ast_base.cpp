#include "../../include/ast/ast_base.hpp"
#include "../../include/token.hpp"

Expression::Expression(const std::string &type, const Token &token)
    : type(type), token(token) {}

const std::string& Expression::getType() const
{
    return type;
}

const Token& Expression::getToken() const
{
    return token;
}

const std::string& Expression::getName() const
{
    return token.getValue();
}
