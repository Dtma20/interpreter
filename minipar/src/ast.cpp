#include "../include/ast.hpp"

/**
 * @brief Representa uma expressão na árvore sintática (AST).
 *
 * A classe Expression é a classe base para todas as expressões.
 * Ela armazena o tipo da expressão (por exemplo, "NUM", "STRING", etc.)
 * e o token associado (que contém informações como valor e posição).
 */
Expression::Expression(const std::string &type, const Token &token)
    : type(type), token(token) {}

/**
 * @brief Obtém o tipo da expressão.
 * @return Uma string representando o tipo da expressão.
 */
std::string Expression::getType() const
{
    return type;
}

/**
 * @brief Obtém o token associado à expressão.
 * @return O token associado.
 */
Token Expression::getToken() const
{
    return token;
}

/**
 * @brief Obtém o nome da expressão, utilizando o valor do token.
 * @return Uma string com o valor do token.
 */
std::string Expression::getName() const
{
    return token.getValue();
}

/**
 * @brief Construtor da classe Constant.
 *
 * Representa uma constante na AST, como números, strings ou valores booleanos.
 *
 * @param type Tipo da constante.
 * @param token Token associado à constante.
 */
Constant::Constant(const std::string &type, const Token &token)
    : Expression(type, token) {}

/**
 * @brief Construtor da classe ID (identificador).
 *
 * Representa um identificador na AST. Pode ser utilizado para declarações ou referências.
 *
 * @param type Tipo do identificador.
 * @param token Token associado ao identificador.
 * @param decl Indica se o identificador representa uma declaração.
 */
ID::ID(const std::string &type, const Token &token, bool decl)
    : Expression(type, token), decl(decl) {}

/**
 * @brief Verifica se o identificador representa uma declaração.
 * @return true se for declaração, false caso contrário.
 */
bool ID::isDecl() const
{
    return decl;
}

/**
 * @brief Construtor da classe Access.
 *
 * Representa o acesso a um elemento de um identificador, por exemplo, acesso a um índice de array.
 *
 * @param type Tipo do acesso.
 * @param token Token associado ao acesso.
 * @param id Ponteiro único para o identificador base.
 * @param expr Ponteiro único para a expressão que indica o índice ou campo de acesso.
 */
Access::Access(const std::string &type, const Token &token, std::unique_ptr<ID> id, std::unique_ptr<Expression> expr)
    : Expression(type, token), id(std::move(id)), expr(std::move(expr)) {}

/**
 * @brief Obtém o identificador associado ao acesso.
 * @return Ponteiro para o objeto ID.
 */
ID *Access::getId() const
{
    return id.get();
}

/**
 * @brief Obtém a expressão associada ao acesso.
 * @return Ponteiro para a expressão.
 */
Expression *Access::getExpr() const
{
    return expr.get();
}

/**
 * @brief Construtor da classe Logical.
 *
 * Representa uma operação lógica (como AND ou OR) na AST.
 *
 * @param type Tipo da operação lógica.
 * @param token Token associado à operação.
 * @param left Ponteiro único para a expressão do lado esquerdo.
 * @param right Ponteiro único para a expressão do lado direito.
 */
Logical::Logical(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : Expression(type, token), left(std::move(left)), right(std::move(right)) {}

/**
 * @brief Obtém a expressão à esquerda da operação lógica.
 * @return Ponteiro para a expressão à esquerda.
 */
Expression *Logical::getLeft() const
{
    return left.get();
}

/**
 * @brief Obtém a expressão à direita da operação lógica.
 * @return Ponteiro para a expressão à direita.
 */
Expression *Logical::getRight() const
{
    return right.get();
}

/**
 * @brief Construtor da classe Relational.
 *
 * Representa uma operação relacional (por exemplo, ==, <, >) na AST.
 *
 * @param type Tipo da operação relacional.
 * @param token Token associado à operação.
 * @param left Ponteiro único para a expressão do lado esquerdo.
 * @param right Ponteiro único para a expressão do lado direito.
 */
Relational::Relational(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : Expression(type, token), left(std::move(left)), right(std::move(right)) {}

/**
 * @brief Obtém a expressão à esquerda da operação relacional.
 * @return Ponteiro para a expressão à esquerda.
 */
Expression *Relational::getLeft() const
{
    return left.get();
}

/**
 * @brief Obtém a expressão à direita da operação relacional.
 * @return Ponteiro para a expressão à direita.
 */
Expression *Relational::getRight() const
{
    return right.get();
}

/**
 * @brief Construtor da classe Arithmetic.
 *
 * Representa uma operação aritmética (como +, -, *, /) na AST.
 *
 * @param type Tipo da operação aritmética.
 * @param token Token associado à operação.
 * @param left Ponteiro único para a expressão do lado esquerdo.
 * @param right Ponteiro único para a expressão do lado direito.
 */
Arithmetic::Arithmetic(const std::string &type, const Token &token, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : Expression(type, token), left(std::move(left)), right(std::move(right)) {}

/**
 * @brief Obtém a expressão à esquerda da operação aritmética.
 * @return Ponteiro para a expressão à esquerda.
 */
Expression *Arithmetic::getLeft() const
{
    return left.get();
}

/**
 * @brief Obtém a expressão à direita da operação aritmética.
 * @return Ponteiro para a expressão à direita.
 */
Expression *Arithmetic::getRight() const
{
    return right.get();
}

#include "../include/ast.hpp"

/**
 * @brief Construtor para uma operação unária.
 *
 * @param type Tipo da operação.
 * @param token Token representando o operador unário.
 * @param expr Ponteiro único para o operando.
 * @param isPostfix Indica se o operador é pós-fixado.
 */
Unary::Unary(const std::string &type, const Token &token, std::unique_ptr<Expression> expr, bool isPostfix)
    : Expression(type, token), expr(std::move(expr)), m_isPostfix(isPostfix) {}

/**
 * @brief Obtém o operando da operação unária.
 *
 * @return Ponteiro para a expressão.
 */
Expression *Unary::getExpr() const
{
    return expr.get();
}

/**
 * @brief Verifica se o operador é pós-fixado.
 *
 * @return true se o operador é pós-fixado (ex.: x++ ou x--); false caso contrário.
 */
bool Unary::isPostfix() const
{
    return m_isPostfix;
}

/**
 * @brief Construtor da classe Call.
 *
 * Representa uma chamada de função na AST.
 *
 * @param type Tipo da chamada.
 * @param token Token associado à chamada.
 * @param id Ponteiro único para o identificador da função.
 * @param args Lista de argumentos passados para a função.
 * @param oper Operador associado à chamada (se aplicável).
 */
Call::Call(const std::string &type, const Token &token, std::unique_ptr<ID> id, Arguments args, const std::string &oper)
    : Expression(type, token), id(std::move(id)), args(std::move(args)), oper(oper) {}

/**
 * @brief Obtém o identificador da função chamada.
 * @return Ponteiro para o objeto ID.
 */
ID *Call::getId() const
{
    return id.get();
}

/**
 * @brief Obtém os argumentos passados para a função.
 * @return Referência constante para a lista de argumentos.
 */
const Arguments &Call::getArgs() const
{
    return args;
}

/**
 * @brief Obtém o operador associado à chamada.
 * @return Uma string contendo o operador.
 */
std::string Call::getOper() const
{
    return oper;
}

/**
 * @brief Construtor da classe Module.
 *
 * Representa um módulo (arquivo ou unidade de código) que contém um corpo (lista de statements).
 *
 * @param stmts Ponteiro único para o corpo do módulo.
 */
Module::Module(std::unique_ptr<Body> stmts)
    : stmts(std::move(stmts)) {}

/**
 * @brief Obtém os statements do módulo.
 * @return Referência constante para o corpo do módulo.
 */
const Body &Module::getStmts() const
{
    return *stmts;
}

/**
 * @brief Construtor da classe Assign.
 *
 * Representa uma operação de atribuição na AST.
 *
 * @param left Ponteiro único para a expressão do lado esquerdo.
 * @param right Ponteiro único para a expressão do lado direito.
 */
Assign::Assign(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : left(std::move(left)), right(std::move(right)) {}

/**
 * @brief Obtém a expressão do lado esquerdo da atribuição.
 * @return Ponteiro para a expressão.
 */
Expression *Assign::getLeft() const
{
    return left.get();
}

/**
 * @brief Obtém a expressão do lado direito da atribuição.
 * @return Ponteiro para a expressão.
 */
Expression *Assign::getRight() const
{
    return right.get();
}

/**
 * @brief Construtor da classe Return.
 *
 * Representa a instrução de retorno de uma função na AST.
 *
 * @param expr Ponteiro único para a expressão a ser retornada.
 */
Return::Return(std::unique_ptr<Expression> expr)
    : expr(std::move(expr)) {}

/**
 * @brief Obtém a expressão associada à instrução de retorno.
 * @return Ponteiro para a expressão.
 */
Expression *Return::getExpr() const
{
    return expr.get();
}

/**
 * @brief Construtor da classe Break.
 *
 * Representa a instrução de interrupção (break) na AST.
 */
Break::Break() {}

/**
 * @brief Construtor da classe Continue.
 *
 * Representa a instrução de continuação (continue) em loops na AST.
 */
Continue::Continue() {}

/**
 * @brief Construtor da classe FuncDef.
 *
 * Representa a definição de uma função na AST, contendo o nome, tipo de retorno,
 * parâmetros e o corpo (lista de instruções) da função.
 *
 * @param name Nome da função.
 * @param return_type Tipo de retorno da função.
 * @param params Mapa de parâmetros da função (nome, tipo e valor padrão opcional).
 * @param body Corpo da função (lista de statements).
 */
FuncDef::FuncDef(const std::string &name, const std::string &return_type, Parameters &&params, std::unique_ptr<Body> body)
    : name(name), return_type(return_type), params(std::move(params)), body(std::move(body)) {}

/**
 * @brief Obtém o nome da função.
 * @return Uma string com o nome da função.
 */
std::string FuncDef::getName() const { return name; }

/**
 * @brief Obtém o tipo de retorno da função.
 * @return Uma string com o tipo de retorno.
 */
std::string FuncDef::getReturnType() const { return return_type; }

/**
 * @brief Obtém os parâmetros da função.
 * @return Referência constante para o mapa de parâmetros.
 */
const Parameters &FuncDef::getParams() const { return params; }

/**
 * @brief Obtém o corpo da função (lista de statements).
 * @return Referência constante para o corpo da função.
 */
const Body &FuncDef::getBody() const { return *body; }

/**
 * @brief Construtor da classe If.
 *
 * Representa uma estrutura condicional (if/else) na AST.
 *
 * @param condition Expressão condicional.
 * @param body Corpo da instrução if.
 * @param else_stmt Corpo da instrução else (opcional).
 */
If::If(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body, std::unique_ptr<Body> else_stmt)
    : condition(std::move(condition)), body(std::move(body)), else_stmt(std::move(else_stmt)) {}

/**
 * @brief Obtém a condição da estrutura if.
 * @return Ponteiro para a expressão condicional.
 */
Expression *If::getCondition() const
{
    return condition.get();
}

/**
 * @brief Obtém o corpo da instrução if.
 * @return Referência constante para o corpo do if.
 */
const Body &If::getBody() const
{
    return *body;
}

/**
 * @brief Obtém o corpo da instrução else, se existir.
 * @return Ponteiro para o corpo do else ou nullptr se não existir.
 */
const Body *If::getElseStmt() const
{
    return else_stmt ? else_stmt.get() : nullptr;
}

/**
 * @brief Construtor da classe While.
 *
 * Representa um laço de repetição (while) na AST.
 *
 * @param condition Expressão condicional do loop.
 * @param body Corpo do loop.
 */
While::While(std::unique_ptr<Expression> condition, std::unique_ptr<Body> body)
    : condition(std::move(condition)), body(std::move(body)) {}

/**
 * @brief Obtém a condição do loop while.
 * @return Ponteiro para a expressão condicional.
 */
Expression *While::getCondition() const
{
    return condition.get();
}

/**
 * @brief Obtém o corpo do loop while.
 * @return Referência constante para o corpo do loop.
 */
const Body &While::getBody() const
{
    return *body;
}

/**
 * @brief Construtor da classe Par.
 *
 * Representa um bloco de código encapsulado (por exemplo, para execução paralela).
 *
 * @param body Corpo do bloco.
 */
Par::Par(std::unique_ptr<Body> body)
    : body(std::move(body)) {}

/**
 * @brief Obtém o corpo do bloco encapsulado.
 * @return Referência constante para o corpo do bloco.
 */
const Body &Par::getBody() const
{
    return *body;
}

/**
 * @brief Construtor da classe Seq.
 *
 * Representa uma sequência de instruções na AST.
 *
 * @param body Corpo da sequência.
 */
Seq::Seq(std::unique_ptr<Body> body)
    : body(std::move(body)) {}

/**
 * @brief Obtém o corpo da sequência de instruções.
 * @return Referência constante para o corpo da sequência.
 */
const Body &Seq::getBody() const
{
    return *body;
}

/**
 * @brief Construtor da classe Channel.
 *
 * Representa um canal de comunicação, contendo o nome, endereço local e porta.
 *
 * @param name Nome do canal.
 * @param localhost Ponteiro único para a expressão que representa o endereço local.
 * @param port Ponteiro único para a expressão que representa a porta.
 */
Channel::Channel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port)
    : name(name), _localhost(std::move(localhost)), _port(std::move(port)) {}

/**
 * @brief Obtém o nome do canal.
 * @return Uma string com o nome do canal.
 */
std::string Channel::getName() const
{
    return name;
}

/**
 * @brief Obtém o endereço local do canal a partir do token da expressão.
 * @return Uma string com o endereço local.
 */
std::string Channel::getLocalhost() const
{
    return _localhost->getToken().getValue();
}

/**
 * @brief Obtém a porta do canal a partir do token da expressão.
 * @return Uma string com a porta.
 */
std::string Channel::getPort() const
{
    return _port->getToken().getValue();
}

/**
 * @brief Obtém o nó da expressão que representa o endereço local.
 * @return Ponteiro para a expressão do endereço local.
 */
Expression *Channel::getLocalhostNode() const
{
    return _localhost.get();
}

/**
 * @brief Obtém o nó da expressão que representa a porta.
 * @return Ponteiro para a expressão da porta.
 */
Expression *Channel::getPortNode() const
{
    return _port.get();
}

/**
 * @brief Construtor da classe SChannel.
 *
 * Representa um canal de serviço, que além dos atributos de Channel,
 * possui o nome de uma função associada e uma descrição.
 *
 * @param name Nome do canal.
 * @param localhost Ponteiro único para a expressão do endereço local.
 * @param port Ponteiro único para a expressão da porta.
 * @param func_name Nome da função associada ao canal.
 * @param description Ponteiro único para a expressão que descreve o canal.
 */
SChannel::SChannel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port,
                   const std::string &func_name, std::unique_ptr<Expression> description)
    : Channel(name, std::move(localhost), std::move(port)), func_name(func_name), description(std::move(description)) {}

/**
 * @brief Obtém o nome da função associada ao canal de serviço.
 * @return Uma string com o nome da função.
 */
std::string SChannel::getFuncName() const
{
    return func_name;
}

/**
 * @brief Obtém a descrição do canal de serviço.
 * @return Ponteiro para a expressão que representa a descrição.
 */
Expression *SChannel::getDescription() const
{
    return description.get();
}

/**
 * @brief Construtor da classe CChannel.
 *
 * Representa um canal de cliente, que utiliza apenas os atributos herdados de Channel.
 *
 * @param name Nome do canal.
 * @param localhost Ponteiro único para a expressão do endereço local.
 * @param port Ponteiro único para a expressão da porta.
 */
CChannel::CChannel(const std::string &name, std::unique_ptr<Expression> localhost, std::unique_ptr<Expression> port)
    : Channel(name, std::move(localhost), std::move(port)) {}
