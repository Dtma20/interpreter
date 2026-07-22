#pragma once

#include <memory>
#include <string>
#include <vector>
#include "ast_base.hpp"

/**
 * @brief Representa um módulo (programa raiz).
 */
class Module : public Statement
{
public:
    Module(std::unique_ptr<Node> stmt) : stmt(std::move(stmt)) {}
    Node *getStmt() const { return stmt.get(); }
    std::vector<Node *> getAttributes() override { return {stmt.get()}; }
private:
    std::unique_ptr<Node> stmt;
};

/**
 * @brief Representa uma atribuição ou declaração.
 */
class Assign : public Statement
{
public:
    Assign(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right,
           bool isDecl = false, std::string type = "");
    Expression *getLeft() const;
    Expression *getRight() const;
    bool isDeclaration() const;
    std::string getVarType() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    bool isDecl;
    std::string varType;
};

/**
 * @brief Representa uma instrução de retorno.
 */
class Return : public Statement
{
public:
    Return(std::unique_ptr<Expression> expr);
    Expression *getExpr() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> expr;
};

/**
 * @brief Representa uma instrução break.
 */
class Break : public Statement
{
public:
    Break();
    std::vector<Node *> getAttributes() override { return {}; }
};

/**
 * @brief Representa uma instrução continue.
 */
class Continue : public Statement
{
public:
    Continue();
    std::vector<Node *> getAttributes() override { return {}; }
};

/**
 * @brief Representa a definição de uma função.
 */
class FuncDef : public Statement
{
public:
    FuncDef(const std::string &name, const std::string &return_type, Parameters &&params, std::unique_ptr<Body> body);
    std::string getName() const;
    std::string getReturnType() const;
    const Parameters &getParams() const;
    const Body &getBody() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::string name;
    std::string return_type;
    Parameters params;
    std::unique_ptr<Body> body;
};

/**
 * @brief Representa uma instrução if/else.
 */
class If : public Statement
{
public:
    If(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body, std::unique_ptr<Body> else_stmt);
    Expression *getCondition() const;
    const Body &getBody() const;
    const Body *getElseStmt() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Body> body;
    std::unique_ptr<Body> else_stmt;
};

/**
 * @brief Representa um loop while.
 */
class While : public Statement
{
public:
    While(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body);
    Expression *getCondition() const;
    const Body &getBody() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Body> body;
};

/**
 * @brief Representa um bloco de execução paralela.
 */
class Par : public Statement
{
public:
    Par(std::unique_ptr<Body> body);
    const Body &getBody() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Body> body;
};

/**
 * @brief Representa um bloco de execução sequencial.
 */
class Seq : public Statement
{
public:
    Seq(std::unique_ptr<Body> body, bool is_block = false)
        : body(std::move(body)), is_block(is_block) {}
    const Body &getBody() const { return *body; }
    bool isBlock() const { return is_block; }
    std::vector<Node *> getAttributes() override {
        std::vector<Node *> attrs;
        for (const auto &stmt : *body) {
            attrs.push_back(stmt.get());
        }
        return attrs;
    }
private:
    std::unique_ptr<Body> body;
    bool is_block;
};

/**
 * @brief Classe base para canais de comunicação.
 */
class Channel : public Statement
{
public:
    Channel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port);
    std::string getName() const;
    std::string getLocalhost() const;
    std::string getPort() const;
    Expression *getLocalhostNode() const;
    Expression *getPortNode() const;
    std::vector<Node *> getAttributes() override { return {}; }
protected:
    std::string name;
    std::unique_ptr<Expression> _localhost;
    std::unique_ptr<Expression> _port;
};

/**
 * @brief Representa um canal de serviço (SChannel).
 */
class SChannel : public Channel
{
public:
    SChannel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port,
             const std::string &func_name, std::unique_ptr<Expression> description);
    std::string getFuncName() const;
    Expression *getDescription() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::string func_name;
    std::unique_ptr<Expression> description;
};

/**
 * @brief Representa um canal de cliente (CChannel).
 */
class CChannel : public Channel
{
public:
    CChannel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port);
    std::vector<Node *> getAttributes() override { return {}; }
};
