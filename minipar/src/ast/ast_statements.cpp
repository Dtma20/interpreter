#include "../../include/ast/ast_statements.hpp"
#include "../../include/token.hpp"

Assign::Assign(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right,
               bool isDecl, std::string type)
    : left(std::move(left)), right(std::move(right)), isDecl(isDecl), varType(type) {}

Expression* Assign::getLeft() const { return left.get(); }
Expression* Assign::getRight() const { return right.get(); }
bool Assign::isDeclaration() const { return isDecl; }
std::string Assign::getVarType() const { return varType; }

Return::Return(std::unique_ptr<Expression> expr) : expr(std::move(expr)) {}
Expression *Return::getExpr() const { return expr.get(); }

Break::Break() {}
Continue::Continue() {}

FuncDef::FuncDef(const std::string &name, const std::string &return_type,
                 Parameters &&params, std::unique_ptr<Body> body)
    : name(name), return_type(return_type), params(std::move(params)), body(std::move(body)) {}

std::string FuncDef::getName() const { return name; }
std::string FuncDef::getReturnType() const { return return_type; }
const Parameters &FuncDef::getParams() const { return params; }
const Body &FuncDef::getBody() const { return *body; }

If::If(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body,
       std::unique_ptr<Body> else_stmt)
    : condition(std::move(condition)), body(std::move(body)), else_stmt(std::move(else_stmt)) {}

Expression *If::getCondition() const { return condition.get(); }
const Body &If::getBody() const { return *body; }
const Body *If::getElseStmt() const { return else_stmt ? else_stmt.get() : nullptr; }

While::While(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body)
    : condition(std::move(condition)), body(std::move(body)) {}

Expression *While::getCondition() const { return condition.get(); }
const Body &While::getBody() const { return *body; }

Par::Par(std::unique_ptr<Body> body) : body(std::move(body)) {}
const Body &Par::getBody() const { return *body; }

Channel::Channel(const std::string &name, std::unique_ptr<Expression> localhost,
                 std::unique_ptr<Expression> port)
    : name(name), _localhost(std::move(localhost)), _port(std::move(port)) {}

std::string Channel::getName() const { return name; }
std::string Channel::getLocalhost() const { return _localhost->getToken().getValue(); }
std::string Channel::getPort() const { return _port->getToken().getValue(); }
Expression *Channel::getLocalhostNode() const { return _localhost.get(); }
Expression *Channel::getPortNode() const { return _port.get(); }

SChannel::SChannel(const std::string &name, std::unique_ptr<Expression> localhost,
                   std::unique_ptr<Expression> port, const std::string &func_name,
                   std::unique_ptr<Expression> description)
    : Channel(name, std::move(localhost), std::move(port)),
      func_name(func_name), description(std::move(description)) {}

std::string SChannel::getFuncName() const { return func_name; }
Expression *SChannel::getDescription() const { return description.get(); }

CChannel::CChannel(const std::string &name, std::unique_ptr<Expression> localhost,
                   std::unique_ptr<Expression> port)
    : Channel(name, std::move(localhost), std::move(port)) {}
