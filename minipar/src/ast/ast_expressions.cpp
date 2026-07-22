#include "../../include/ast/ast_expressions.hpp"
#include "../../include/token.hpp"

ID::ID(const std::string &type, const Token &token, bool decl)
    : Expression(type, token), decl(decl) {}

bool ID::isDecl() const
{
    return decl;
}

Access::Access(const std::string &type, const Token &token,
               std::unique_ptr<Expression> base, std::unique_ptr<Expression> index)
    : Expression(type, token), base(std::move(base)), index(std::move(index)) {}

Expression *Access::getBase() const
{
    return base.get();
}

Expression *Access::getIndex() const
{
    return index.get();
}

Logical::Logical(const std::string &type, const Token &token,
                 std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : Expression(type, token), left(std::move(left)), right(std::move(right)) {}

Expression *Logical::getLeft() const
{
    return left.get();
}

Expression *Logical::getRight() const
{
    return right.get();
}

Relational::Relational(const std::string &type, const Token &token,
                       std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : Expression(type, token), left(std::move(left)), right(std::move(right)) {}

Expression *Relational::getLeft() const
{
    return left.get();
}

Expression *Relational::getRight() const
{
    return right.get();
}

Arithmetic::Arithmetic(const std::string &type, const Token &token,
                       std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : Expression(type, token), left(std::move(left)), right(std::move(right)) {}

Expression *Arithmetic::getLeft() const
{
    return left.get();
}

Expression *Arithmetic::getRight() const
{
    return right.get();
}

Unary::Unary(const std::string &type, const Token &token,
             std::unique_ptr<Expression> expr, bool isPostfix)
    : Expression(type, token), expr(std::move(expr)), m_isPostfix(isPostfix) {}

Expression *Unary::getExpr() const
{
    return expr.get();
}

bool Unary::isPostfix() const
{
    return m_isPostfix;
}

Call::Call(const std::string &type, const Token &token,
           std::unique_ptr<Expression> base, Arguments args, const std::string &oper)
    : Expression(type, token), base(std::move(base)), args(std::move(args)), oper(oper) {}

Expression *Call::getBase() const
{
    return base.get();
}

const Arguments &Call::getArgs() const
{
    return args;
}

std::string Call::getOper() const
{
    return oper;
}
