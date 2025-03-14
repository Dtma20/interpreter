#ifndef AST_HPP
#define AST_HPP

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "token.hpp"

/**
 * @brief Declarações antecipadas das classes da AST.
 */
class Node;
class Expression;
class Statement;

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
using Parameters = std::unordered_map<std::string, std::pair<std::string, std::unique_ptr<Expression>>>;

/**
 * @brief Classe base para todos os nós da AST.
 */
class Node
{
public:
    virtual ~Node() = default;
    /**
     * @brief Retorna os atributos filhos deste nó.
     * @return Vetor de ponteiros para os nós atributos.
     */
    virtual std::vector<Node *> getAttributes() = 0;
};

/**
 * @brief Classe base para expressões na AST.
 *
 * Contém informações comuns, como o tipo da expressão e o token associado.
 */
class Expression : public Node
{
public:
    /**
     * @brief Construtor para uma expressão.
     * @param type Tipo da expressão.
     * @param token Token associado à expressão.
     */
    Expression() = default;
    Expression(const std::string &type, const Token &token);
    virtual ~Expression() = default;

    /**
     * @brief Obtém o tipo da expressão.
     * @return Tipo da expressão como string.
     */
    std::string getType() const;

    /**
     * @brief Obtém o token associado à expressão.
     * @return Token da expressão.
     */
    Token getToken() const;

    /**
     * @brief Obtém o nome da expressão, geralmente a partir do token.
     * @return Nome da expressão como string.
     */
    std::string getName() const;

protected:
    std::string type; // Tipo da expressão.
    Token token;      // Token associado à expressão.
};

/**
 * @brief Classe base para instruções na AST.
 */
class Statement : public Node
{
public:
    virtual ~Statement() = default;
};

/**
 * @brief Representa uma constante literal na AST.
 */
class Constant : public Expression
{
public:
    /**
     * @brief Construtor para uma constante.
     * @param type Tipo da constante.
     * @param token Token contendo o valor da constante.
     */
    Constant(const std::string &type, const Token &token);
    std::vector<Node *> getAttributes() override { return {}; }
};

/**
 * @brief Representa um identificador (variável) na AST.
 */
class ID : public Expression
{
public:
    /**
     * @brief Construtor para um identificador.
     * @param type Tipo associado ao identificador.
     * @param token Token representando o identificador.
     * @param decl Flag que indica se o identificador é uma declaração.
     */
    ID(const std::string &type, const Token &token, bool decl = false);

    /**
     * @brief Indica se o identificador é uma declaração.
     * @return true se for declaração, false caso contrário.
     */
    bool isDecl() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    bool decl; // Indica se o identificador é uma declaração.
};

class Access : public Expression
{
public:
    Access(const std::string &type, const Token &token, std::unique_ptr<Expression> base, std::unique_ptr<Expression> index);
    Expression *getBase() const;  // Alterado de getId para refletir o novo tipo
    Expression *getIndex() const; // Alterado de getExpr para maior clareza
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> base;  // Alterado de std::unique_ptr<ID>
    std::unique_ptr<Expression> index; // Renomeado de expr
};

/**
 * @brief Representa uma operação lógica (ex.: AND, OR) na AST.
 */
class Logical : public Expression
{
public:
    /**
     * @brief Construtor para uma operação lógica.
     * @param type Tipo da operação (geralmente BOOL).
     * @param token Token representando o operador lógico.
     * @param left Ponteiro único para o operando à esquerda.
     * @param right Ponteiro único para o operando à direita.
     */
    Logical(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);

    /**
     * @brief Obtém o operando à esquerda.
     * @return Ponteiro para a expressão à esquerda.
     */
    Expression *getLeft() const;

    /**
     * @brief Obtém o operando à direita.
     * @return Ponteiro para a expressão à direita.
     */
    Expression *getRight() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> left;  // Operando esquerdo.
    std::unique_ptr<Expression> right; // Operando direito.
};

/**
 * @brief Representa uma operação relacional (ex.: ==, !=, <, >) na AST.
 */
class Relational : public Expression
{
public:
    /**
     * @brief Construtor para uma operação relacional.
     * @param type Tipo da operação (geralmente BOOL).
     * @param token Token representando o operador relacional.
     * @param left Ponteiro único para o operando à esquerda.
     * @param right Ponteiro único para o operando à direita.
     */
    Relational(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);

    /**
     * @brief Obtém o operando à esquerda.
     * @return Ponteiro para a expressão à esquerda.
     */
    Expression *getLeft() const;

    /**
     * @brief Obtém o operando à direita.
     * @return Ponteiro para a expressão à direita.
     */
    Expression *getRight() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> left;  // Operando esquerdo.
    std::unique_ptr<Expression> right; // Operando direito.
};

/**
 * @brief Representa uma operação aritmética (ex.: +, -, *, /) na AST.
 */
class Arithmetic : public Expression
{
public:
    /**
     * @brief Construtor para uma operação aritmética.
     * @param type Tipo da operação (ex.: NUMBER).
     * @param token Token representando o operador aritmético.
     * @param left Ponteiro único para o operando à esquerda.
     * @param right Ponteiro único para o operando à direita.
     */
    Arithmetic(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);

    /**
     * @brief Obtém o operando à esquerda.
     * @return Ponteiro para a expressão à esquerda.
     */
    Expression *getLeft() const;

    /**
     * @brief Obtém o operando à direita.
     * @return Ponteiro para a expressão à direita.
     */
    Expression *getRight() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> left;  // Operando esquerdo.
    std::unique_ptr<Expression> right; // Operando direito.
};

/**
 * @brief Representa uma operação unária (ex.: -, !) na AST.
 */
/**
 * @brief Representa uma operação unária (ex.: -, !) na AST.
 */
class Unary : public Expression
{
public:
    /**
     * @brief Construtor para uma operação unária.
     * @param type Tipo da operação.
     * @param token Token representando o operador unário.
     * @param expr Ponteiro único para o operando.
     * @param isPostfix Indica se o operador é pós-fixado (true para x++ ou x--; false para ++x, --x, -, !).
     */
    Unary(const std::string &type, const Token &token, std::unique_ptr<Expression> expr, bool isPostfix = false);

    /**
     * @brief Obtém o operando da operação unária.
     * @return Ponteiro para a expressão.
     */
    Expression *getExpr() const;

    /**
     * @brief Verifica se o operador é pós-fixado.
     * @return true se o operador é pós-fixado (ex.: x++ ou x--); false caso contrário.
     */
    bool isPostfix() const;

    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> expr; // Operando.
    bool m_isPostfix;                 // Indica se o operador é pós-fixado.
};

/**
 * @brief Representa uma chamada de função na AST.
 */
class Call : public Expression
{
public:
    Call(const std::string &type, const Token &token, std::unique_ptr<Expression> base, Arguments args, const std::string &oper);
    Expression *getBase() const; // Alterado de getId
    const Arguments &getArgs() const;
    std::string getOper() const;
    std::vector<Node *> getAttributes() override { return {}; }
private:
    std::unique_ptr<Expression> base; // Alterado de std::unique_ptr<ID>
    Arguments args;                   // Lista de argumentos.
    std::string oper;                 // Operador ou nome.
};

// ======================================================================
// Subclasses de Statement
// ======================================================================

class Module : public Statement
{
public:
    Module(std::unique_ptr<Node> stmt) : stmt(std::move(stmt)) {}
    Node *getStmt() const { return stmt.get(); }
    std::vector<Node *> getAttributes() override { return {stmt.get()}; }

private:
    std::unique_ptr<Node> stmt;
};

class Assign : public Statement
{
public:
    /**
     * @brief Construtor para uma atribuição.
     * @param left Ponteiro único para a expressão do lado esquerdo.
     * @param right Ponteiro único para a expressão do lado direito.
     * @param isDecl Indica se é uma declaração (padrão: false).
     * @param type Tipo da variável, se for uma declaração (padrão: vazio).
     */
    Assign(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, 
           bool isDecl = false, std::string type = "");

    /**
     * @brief Obtém a expressão do lado esquerdo.
     * @return Ponteiro para a expressão.
     */
    Expression *getLeft() const;

    /**
     * @brief Obtém a expressão do lado direito.
     * @return Ponteiro para a expressão.
     */
    Expression *getRight() const;

    /**
     * @brief Verifica se a atribuição é uma declaração.
     * @return true se for uma declaração, false caso contrário.
     */
    bool isDeclaration() const;

    /**
     * @brief Obtém o tipo da variável, se for uma declaração.
     * @return String com o tipo da variável.
     */
    std::string getVarType() const;

    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> left;  // Expressão do lado esquerdo.
    std::unique_ptr<Expression> right; // Expressão do lado direito.
    bool isDecl;                       // Indica se é uma declaração.
    std::string varType;               // Tipo da variável, se for uma declaração.
};

/**
 * @brief Representa uma instrução de retorno.
 */
class Return : public Statement
{
public:
    /**
     * @brief Construtor para um retorno.
     * @param expr Ponteiro único para a expressão a ser retornada.
     */
    Return(std::unique_ptr<Expression> expr);

    /**
     * @brief Obtém a expressão associada ao retorno.
     * @return Ponteiro para a expressão.
     */
    Expression *getExpr() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> expr; // Expressão a ser retornada.
};

/**
 * @brief Representa uma instrução break.
 */
class Break : public Statement
{
public:
    /**
     * @brief Construtor para break.
     */
    Break();
    std::vector<Node *> getAttributes() override { return {}; }
};

/**
 * @brief Representa uma instrução continue.
 */
class Continue : public Statement
{
public:
    /**
     * @brief Construtor para continue.
     */
    Continue();
    std::vector<Node *> getAttributes() override { return {}; }
};

/**
 * @brief Representa a definição de uma função.
 */
class FuncDef : public Statement
{
public:
    /**
     * @brief Construtor para a definição de uma função.
     * @param name Nome da função.
     * @param return_type Tipo de retorno da função.
     * @param params Parâmetros da função.
     * @param body Ponteiro único para o corpo da função.
     */
    FuncDef(const std::string &name, const std::string &return_type, Parameters &&params, std::unique_ptr<Body> body);

    /**
     * @brief Obtém o nome da função.
     * @return Nome da função como string.
     */
    std::string getName() const;

    /**
     * @brief Obtém o tipo de retorno da função.
     * @return Tipo de retorno como string.
     */
    std::string getReturnType() const;

    /**
     * @brief Obtém os parâmetros da função.
     * @return Referência constante para os parâmetros.
     */
    const Parameters &getParams() const;

    /**
     * @brief Obtém o corpo da função.
     * @return Referência constante para o corpo (lista de statements).
     */
    const Body &getBody() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::string name;           // Nome da função.
    std::string return_type;    // Tipo de retorno.
    Parameters params;          // Parâmetros da função.
    std::unique_ptr<Body> body; // Corpo da função.
};

/**
 * @brief Representa uma instrução condicional (if/else).
 */
class If : public Statement
{
public:
    /**
     * @brief Construtor para um if.
     * @param condition Ponteiro único para a condição.
     * @param body Ponteiro único para o corpo do if.
     * @param else_stmt Ponteiro único para o corpo do else (opcional).
     */
    If(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body, std::unique_ptr<Body> else_stmt);

    /**
     * @brief Obtém a condição do if.
     * @return Ponteiro para a expressão condicional.
     */
    Expression *getCondition() const;

    /**
     * @brief Obtém o corpo do if.
     * @return Referência constante para o corpo.
     */
    const Body &getBody() const;

    /**
     * @brief Obtém o corpo do else, se existir.
     * @return Ponteiro para o corpo do else ou nullptr.
     */
    const Body *getElseStmt() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> condition; // Expressão condicional.
    std::unique_ptr<Body> body;            // Corpo do if.
    std::unique_ptr<Body> else_stmt;       // Corpo do else (opcional).
};

/**
 * @brief Representa um loop while.
 */
class While : public Statement
{
public:
    /**
     * @brief Construtor para um loop while.
     * @param condition Ponteiro único para a condição do loop.
     * @param body Ponteiro único para o corpo do loop.
     */
    While(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body);

    /**
     * @brief Obtém a condição do loop.
     * @return Ponteiro para a expressão condicional.
     */
    Expression *getCondition() const;

    /**
     * @brief Obtém o corpo do loop.
     * @return Referência constante para o corpo.
     */
    const Body &getBody() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Expression> condition; // Condição do loop.
    std::unique_ptr<Body> body;            // Corpo do loop.
};

/**
 * @brief Representa um bloco de execução paralela.
 */
class Par : public Statement
{
public:
    /**
     * @brief Construtor para um bloco paralelo.
     * @param body Ponteiro único para o corpo do bloco a ser executado em paralelo.
     */
    Par(std::unique_ptr<Body> body);

    /**
     * @brief Obtém o corpo do bloco paralelo.
     * @return Referência constante para o corpo.
     */
    const Body &getBody() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::unique_ptr<Body> body; // Corpo do bloco paralelo.
};

/**
 * @brief Representa um bloco de execução sequencial.
 */
class Seq : public Statement
{
public:
    Seq(std::unique_ptr<Body> body, bool is_block = false) : body(std::move(body)), is_block(is_block) {}
    const Body &getBody() const { return *body; }
    bool isBlock() const { return is_block; } // Agora inline
    std::vector<Node *> getAttributes() override { return {}; }

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
    /**
     * @brief Construtor para um canal.
     * @param name Nome do canal.
     * @param localhost Ponteiro único para a expressão representando o endereço local.
     * @param port Ponteiro único para a expressão representando a porta.
     */
    Channel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port);

    /**
     * @brief Obtém o nome do canal.
     * @return Nome do canal como string.
     */
    std::string getName() const;

    /**
     * @brief Obtém o valor do endereço local.
     * @return Endereço local como string.
     */
    std::string getLocalhost() const;

    /**
     * @brief Obtém o valor da porta.
     * @return Porta como string.
     */
    std::string getPort() const;

    /**
     * @brief Obtém o nó da expressão representando o endereço local.
     * @return Ponteiro para a expressão.
     */
    Expression *getLocalhostNode() const;

    /**
     * @brief Obtém o nó da expressão representando a porta.
     * @return Ponteiro para a expressão.
     */
    Expression *getPortNode() const;
    std::vector<Node *> getAttributes() override { return {}; }

protected:
    std::string name;                       // Nome do canal.
    std::unique_ptr<Expression> _localhost; // Expressão representando o endereço local.
    std::unique_ptr<Expression> _port;      // Expressão representando a porta.
};

/**
 * @brief Representa um canal de serviço (SChannel).
 *
 * Extende a classe Channel, adicionando o nome da função associada e uma descrição.
 */
class SChannel : public Channel
{
public:
    /**
     * @brief Construtor para um canal de serviço.
     * @param name Nome do canal.
     * @param localhost Ponteiro único para a expressão do endereço local.
     * @param port Ponteiro único para a expressão da porta.
     * @param func_name Nome da função associada.
     * @param description Ponteiro único para a expressão da descrição.
     */
    SChannel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port,
             const std::string &func_name, std::unique_ptr<Expression> description);

    /**
     * @brief Obtém o nome da função associada ao canal.
     * @return Nome da função como string.
     */
    std::string getFuncName() const;

    /**
     * @brief Obtém a descrição do canal.
     * @return Ponteiro para a expressão que descreve o canal.
     */
    Expression *getDescription() const;
    std::vector<Node *> getAttributes() override { return {}; }

private:
    std::string func_name;                   // Nome da função associada.
    std::unique_ptr<Expression> description; // Expressão contendo a descrição.
};

/**
 * @brief Representa um canal de cliente (CChannel).
 *
 * Extende a classe Channel sem adicionar atributos adicionais.
 */
class CChannel : public Channel
{
public:
    /**
     * @brief Construtor para um canal de cliente.
     * @param name Nome do canal.
     * @param localhost Ponteiro único para a expressão do endereço local.
     * @param port Ponteiro único para a expressão da porta.
     */
    CChannel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port);
    std::vector<Node *> getAttributes() override { return {}; }
};

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

class ArrayDecl : public Node {
    private:
        std::string name;
        std::vector<std::unique_ptr<Expression>> dimensions;
    public:
        ArrayDecl(const std::string &name, std::vector<std::unique_ptr<Expression>> dims)
            : name(name), dimensions(std::move(dims)) {}
        const std::string &getName() const { return name; }
        const std::vector<std::unique_ptr<Expression>> &getDimensions() const { return dimensions; }
        std::vector<Node *> getAttributes() override {
            std::vector<Node *> attrs;
            for (auto &dim : dimensions) {
                attrs.push_back(dim.get());
            }
            return attrs;
        }
};

#endif // AST_HPP
