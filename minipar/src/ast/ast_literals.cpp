#include "../../include/ast/ast_literals.hpp"
#include "../../include/token.hpp"

Constant::Constant(const std::string &type, const Token &token)
    : Expression(type, token) {}
