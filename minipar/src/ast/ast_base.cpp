#include "../../include/ast/ast_base.hpp"
#include "../../include/token.hpp"

Expression::Expression(const std::string &type, const Token &token)
    : type(type), token(token) {}

std::string Expression::getType() const
{
    return type;
}

Token Expression::getToken() const
{
    return token;
}

std::string Expression::getName() const
{
    return token.getValue();
}
