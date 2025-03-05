#include "../include/semantic.hpp"
#include <stdexcept>
#include <algorithm>

/**
 * @brief Construtor do SemanticAnalyzer.
 *
 * Inicializa o analisador semântico e define a lista de funções padrão suportadas,
 * como "print", "input", "sleep", "to_number", "to_string", "to_bool", "send", "close", "len", "isalpha" e "isnum".
 */
SemanticAnalyzer::SemanticAnalyzer()
{
    default_func_names = {"print", "input", "sleep", "to_number", "to_string",
                          "to_bool", "send", "close", "len", "isalpha", "isnum"};
}

/**
 * @brief Avalia um nó da AST e retorna o tipo resultante.
 *
 * Este método determina o tipo de um nó da árvore sintática abstrata chamando
 * a função de visita correspondente para cada tipo de nó (Constant, ID, Access, Logical,
 * Relational, Arithmetic, Unary ou Call).
 *
 * @param node Ponteiro para o nó da AST.
 * @return String representando o tipo da expressão ou uma string vazia se o nó for nulo.
 */
std::string SemanticAnalyzer::evaluate(Node *node) const
{
    if (!node)
        return "";
    if (auto *constant = dynamic_cast<Constant *>(node))
    {
        return visit_Constant(constant).value_or("");
    }
    else if (auto *id = dynamic_cast<ID *>(node))
    {
        return visit_ID(id).value_or("");
    }
    else if (auto *access = dynamic_cast<Access *>(node))
    {
        return visit_Access(access).value_or("");
    }
    else if (auto *logical = dynamic_cast<Logical *>(node))
    {
        return visit_Logical(logical).value_or("");
    }
    else if (auto *relational = dynamic_cast<Relational *>(node))
    {
        return visit_Relational(relational).value_or("");
    }
    else if (auto *arithmetic = dynamic_cast<Arithmetic *>(node))
    {
        return visit_Arithmetic(arithmetic).value_or("");
    }
    else if (auto *unary = dynamic_cast<Unary *>(node))
    {
        return visit_Unary(unary).value_or("");
    }
    else if (auto *call = dynamic_cast<Call *>(node))
    {
        return visit_Call(call).value_or("");
    }
    return "";
}

/**
 * @brief Visita um nó da AST para realizar a análise semântica.
 *
 * Identifica o tipo de nó e invoca o método de visita correspondente. Se o nó não
 * for de um tipo específico de statement, simplesmente avalia seu tipo.
 *
 * @param node Ponteiro para o nó da AST.
 */
void SemanticAnalyzer::visit(Node *node)
{
    if (!node)
        return;
    if (auto *assign = dynamic_cast<Assign *>(node))
    {
        visit_Assign(assign);
    }
    else if (auto *ret = dynamic_cast<Return *>(node))
    {
        visit_Return(ret);
    }
    else if (auto *brk = dynamic_cast<Break *>(node))
    {
        visit_Break(brk);
    }
    else if (auto *cont = dynamic_cast<Continue *>(node))
    {
        visit_Continue(cont);
    }
    else if (auto *func = dynamic_cast<FuncDef *>(node))
    {
        visit_FuncDef(func);
    }
    else if (auto *ifStmt = dynamic_cast<If *>(node))
    {
        visit_If(ifStmt);
    }
    else if (auto *whileStmt = dynamic_cast<While *>(node))
    {
        visit_While(whileStmt);
    }
    else if (auto *par = dynamic_cast<Par *>(node))
    {
        visit_Par(par);
    }
    else if (auto *cchannel = dynamic_cast<CChannel *>(node))
    {
        visit_CChannel(cchannel);
    }
    else if (auto *schannel = dynamic_cast<SChannel *>(node))
    {
        visit_SChannel(schannel);
    }
    else
    {
        evaluate(node);
    }
}

/**
 * @brief Visita um nó genericamente, atualizando o contexto.
 *
 * Empilha o nó atual no contexto, visita os statements contidos (por exemplo, num módulo)
 * e, ao final, remove o nó do contexto.
 *
 * @param node Ponteiro para o nó a ser visitado.
 */
void SemanticAnalyzer::generic_visit(Node *node)
{
    context_stack.push_back(node);
    if (auto *module = dynamic_cast<Module *>(node))
    {
        for (const auto &stmt_ptr : module->getStmts())
        {
            visit(stmt_ptr.get());
        }
    }
    context_stack.pop_back();
}

/**
 * @brief Valida uma atribuição, verificando se o tipo da variável (lado esquerdo)
 * é compatível com o tipo do valor atribuído (lado direito).
 *
 * Também verifica se a atribuição é feita para uma variável (ID).
 *
 * @param node Ponteiro para o nó de atribuição.
 * @throws SemanticError se a atribuição não for válida.
 */
void SemanticAnalyzer::visit_Assign(Assign *node)
{
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());

    if (!dynamic_cast<ID *>(node->getLeft()))
    {
        throw SemanticError("atribuição precisa ser feita para uma variável");
    }
    ID *var = dynamic_cast<ID *>(node->getLeft());
    if (left_type != right_type)
    {
        throw SemanticError("(Erro de Tipo) variável " + var->getToken().getValue() + " espera " + left_type);
    }
}

/**
 * @brief Valida um comando de retorno (return).
 *
 * Verifica se o 'return' ocorre dentro de uma função e se o tipo do valor retornado
 * coincide com o tipo de retorno declarado da função.
 *
 * @param node Ponteiro para o nó Return.
 * @throws SemanticError se o 'return' ocorrer fora de uma função ou com tipo incompatível.
 */
void SemanticAnalyzer::visit_Return(Return *node)
{
    bool in_function = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<FuncDef *>(ctx))
        {
            in_function = true;
            break;
        }
    }
    if (!in_function)
    {
        throw SemanticError("return encontrado fora de uma declaração de função");
    }

    FuncDef *function = nullptr;
    for (auto it = context_stack.rbegin(); it != context_stack.rend(); ++it)
    {
        if (auto *func = dynamic_cast<FuncDef *>(*it))
        {
            function = func;
            break;
        }
    }
    std::string expr_type = evaluate(node->getExpr());
    if (expr_type != function->getReturnType())
    {
        throw SemanticError("retorno em " + function->getName() + " tem tipo diferente do definido");
    }
}

/**
 * @brief Valida um comando break.
 *
 * Garante que o 'break' seja utilizado apenas dentro de loops (como while).
 *
 * @param node Ponteiro para o nó Break.
 * @throws SemanticError se o 'break' ocorrer fora de um loop.
 */
void SemanticAnalyzer::visit_Break(Break *node)
{
    bool in_loop = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<While *>(ctx))
        {
            in_loop = true;
            break;
        }
    }
    if (!in_loop)
    {
        throw SemanticError("break encontrado fora de uma declaração de um loop");
    }
}

/**
 * @brief Valida um comando continue.
 *
 * Garante que o 'continue' seja utilizado apenas dentro de loops.
 *
 * @param node Ponteiro para o nó Continue.
 * @throws SemanticError se o 'continue' ocorrer fora de um loop.
 */
void SemanticAnalyzer::visit_Continue(Continue *node)
{
    bool in_loop = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<While *>(ctx))
        {
            in_loop = true;
            break;
        }
    }
    if (!in_loop)
    {
        throw SemanticError("continue encontrado fora de uma declaração de um loop");
    }
}

/**
 * @brief Valida a declaração de uma função.
 *
 * Verifica se a função está sendo declarada em um escopo global (não dentro de if, while ou blocos paralelos)
 * e registra a função na tabela de funções.
 *
 * @param node Ponteiro para o nó FuncDef.
 * @throws SemanticError se a função for declarada em um escopo local inválido.
 */
void SemanticAnalyzer::visit_FuncDef(FuncDef *node)
{
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<If *>(ctx) || dynamic_cast<While *>(ctx) || dynamic_cast<Par *>(ctx))
        {
            throw SemanticError("não é possível declarar funções dentro de escopos locais");
        }
    }
    if (function_table.find(node->getName()) == function_table.end())
    {
        function_table[node->getName()] = node;
    }
    generic_visit(node);
}

/**
 * @brief Valida uma estrutura condicional if.
 *
 * Verifica se a condição do if é do tipo BOOL e visita os blocos do if e do else (se presente).
 *
 * @param node Ponteiro para o nó If.
 * @throws SemanticError se a condição não for do tipo BOOL.
 */
void SemanticAnalyzer::visit_If(If *node)
{
    std::string cond_type = evaluate(node->getCondition());
    if (cond_type != "BOOL")
    {
        throw SemanticError("esperado BOOL, mas encontrado " + cond_type);
    }
    context_stack.push_back(node);
    visit_block(node->getBody());
    if (node->getElseStmt())
    {
        visit_block(*node->getElseStmt());
    }
    context_stack.pop_back();
}

/**
 * @brief Valida uma estrutura de repetição while.
 *
 * Verifica se a condição do while é do tipo BOOL e visita o bloco de instruções do loop.
 *
 * @param node Ponteiro para o nó While.
 * @throws SemanticError se a condição não for do tipo BOOL.
 */
void SemanticAnalyzer::visit_While(While *node)
{
    std::string cond_type = evaluate(node->getCondition());
    if (cond_type != "BOOL")
    {
        throw SemanticError("esperado BOOL, mas encontrado " + cond_type);
    }
    context_stack.push_back(node);
    visit_block(node->getBody());
    context_stack.pop_back();
}

/**
 * @brief Valida um bloco de execução paralela.
 *
 * Garante que em um bloco de execução paralela (Par) apenas chamadas de função sejam executadas.
 *
 * @param node Ponteiro para o nó Par.
 * @throws SemanticError se algum dos statements não for uma chamada de função.
 */
void SemanticAnalyzer::visit_Par(Par *node)
{
    for (const auto &inst : node->getBody())
    {
        if (!dynamic_cast<Call *>(inst.get()))
        {
            throw SemanticError("esperado apenas funções em um bloco de execução paralela");
        }
    }
}

/**
 * @brief Valida um canal de cliente (CChannel).
 *
 * Verifica se os atributos 'localhost' e 'port' possuem os tipos STRING e NUMBER, respectivamente.
 *
 * @param node Ponteiro para o nó CChannel.
 * @throws SemanticError se os tipos não forem os esperados.
 */
void SemanticAnalyzer::visit_CChannel(CChannel *node)
{
    std::string localhost_type = evaluate(node->getLocalhostNode());
    if (localhost_type != "STRING")
    {
        throw SemanticError("localhost em " + node->getName() + " precisa ser STRING");
    }
    std::string port_type = evaluate(node->getPortNode());
    if (port_type != "NUMBER")
    {
        throw SemanticError("port em " + node->getName() + " precisa ser NUMBER");
    }
}

/**
 * @brief Valida um canal de serviço (SChannel).
 *
 * Verifica se a função associada ao canal está declarada, se possui retorno STRING,
 * se os parâmetros estão corretos e se os atributos 'description', 'localhost' e 'port'
 * possuem os tipos esperados.
 *
 * @param node Ponteiro para o nó SChannel.
 * @throws SemanticError se alguma validação falhar.
 */
void SemanticAnalyzer::visit_SChannel(SChannel *node)
{
    auto it = function_table.find(node->getFuncName());
    if (it == function_table.end())
    {
        throw SemanticError("função " + node->getFuncName() + " não declarada");
    }
    FuncDef *func = it->second;
    if (func->getReturnType() != "STRING")
    {
        throw SemanticError("função base de " + node->getName() + " precisa ter retorno STRING");
    }
    int required_params = 0;
    for (const auto &param : func->getParams())
    {
        if (param.second.second == nullptr)
        {
            required_params++;
        }
    }
    int call_args = 0;
    if (required_params > call_args)
    {
        throw SemanticError("Esperado " + std::to_string(required_params) + " argumentos, mas encontrado " +
                            std::to_string(call_args));
    }
    std::string description_type = evaluate(node->getDescription());
    if (description_type != "STRING")
    {
        throw SemanticError("description em " + node->getName() + " precisa ser STRING");
    }
    std::string localhost_type = evaluate(node->getLocalhostNode());
    if (localhost_type != "STRING")
    {
        throw SemanticError("localhost em " + node->getName() + " precisa ser STRING");
    }
    std::string port_type = evaluate(node->getPortNode());
    if (port_type != "NUMBER")
    {
        throw SemanticError("port em " + node->getName() + " precisa ser NUMBER");
    }
}

/**
 * @brief Avalia uma constante e retorna seu tipo.
 *
 * @param node Ponteiro para o nó Constant.
 * @return Tipo da constante encapsulado em std::optional.
 */
std::optional<std::string> SemanticAnalyzer::visit_Constant(const Constant *node) const
{
    return node->getType();
}

/**
 * @brief Avalia um identificador e retorna seu tipo.
 *
 * @param node Ponteiro para o nó ID.
 * @return Tipo do identificador encapsulado em std::optional.
 */
std::optional<std::string> SemanticAnalyzer::visit_ID(const ID *node) const
{
    return node->getType();
}

/**
 * @brief Avalia um acesso a elemento e retorna seu tipo.
 *
 * Verifica se o acesso por índice é realizado em uma string.
 *
 * @param node Ponteiro para o nó Access.
 * @return Tipo do acesso encapsulado em std::optional.
 * @throws SemanticError se o acesso não for feito em uma string.
 */
std::optional<std::string> SemanticAnalyzer::visit_Access(const Access *node) const
{
    if (node->getType() != "STRING")
    {
        throw SemanticError("Acesso por index é válido apenas em strings");
    }
    return node->getType();
}

/**
 * @brief Avalia uma operação lógica e retorna "BOOL".
 *
 * Verifica se ambos os operandos são do tipo BOOL.
 *
 * @param node Ponteiro para o nó Logical.
 * @return "BOOL" encapsulado em std::optional se a operação for válida.
 * @throws SemanticError se algum operando não for BOOL.
 */
std::optional<std::string> SemanticAnalyzer::visit_Logical(const Logical *node) const
{
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    if (left_type != "BOOL" || right_type != "BOOL")
    {
        throw SemanticError("(Erro de Tipo) Esperado BOOL, mas encontrado " + left_type +
                            " e " + right_type + " na operação " + node->getToken().getValue());
    }
    return "BOOL";
}

/**
 * @brief Avalia uma operação relacional e retorna "BOOL".
 *
 * Verifica se os operandos são compatíveis para comparação:
 * - Para "==" e "!=", os operandos devem ser do mesmo tipo.
 * - Para outros operadores, os operandos devem ser NUMBER.
 *
 * @param node Ponteiro para o nó Relational.
 * @return "BOOL" encapsulado em std::optional se os operandos forem compatíveis.
 * @throws SemanticError se os operandos forem incompatíveis.
 */
std::optional<std::string> SemanticAnalyzer::visit_Relational(const Relational *node) const
{
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    if (node->getToken().getValue() == "==" || node->getToken().getValue() == "!=")
    {
        if (left_type != right_type)
        {
            throw SemanticError("(Erro de Tipo) Esperado tipos iguais, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    else
    {
        if (left_type != "NUMBER" || right_type != "NUMBER")
        {
            throw SemanticError("(Erro de Tipo) Esperado NUMBER, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    return "BOOL";
}

/**
 * @brief Avalia uma operação aritmética e retorna o tipo do operando.
 *
 * Para o operador de soma, permite apenas operandos do mesmo tipo (permitindo concatenar strings ou somar números).
 * Para outros operadores, os operandos devem ser NUMBER.
 *
 * @param node Ponteiro para o nó Arithmetic.
 * @return Tipo dos operandos encapsulado em std::optional.
 * @throws SemanticError se os operandos forem incompatíveis.
 */
std::optional<std::string> SemanticAnalyzer::visit_Arithmetic(const Arithmetic *node) const
{
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    if (node->getToken().getValue() == "+")
    {
        if (left_type != right_type)
        {
            throw SemanticError("(Erro de Tipo) Esperado tipos iguais, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    else
    {
        if (left_type != "NUMBER" || right_type != "NUMBER")
        {
            throw SemanticError("(Erro de Tipo) Esperado NUMBER, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    return left_type;
}

/**
 * @brief Avalia uma operação unária e retorna o tipo do operando.
 *
 * Verifica se o operador unário '-' é aplicado a um NUMBER e se o '!' é aplicado a um BOOL.
 *
 * @param node Ponteiro para o nó Unary.
 * @return Tipo do operando encapsulado em std::optional.
 * @throws SemanticError se o tipo do operando não for compatível com o operador.
 */
std::optional<std::string> SemanticAnalyzer::visit_Unary(const Unary *node) const
{
    std::string expr_type = evaluate(node->getExpr());
    if (node->getToken().getTag() == "-")
    {
        if (expr_type != "NUMBER")
        {
            throw SemanticError("(Erro de Tipo) Esperado NUMBER, mas encontrado " +
                                expr_type + " na operação " + node->getToken().getValue());
        }
    }
    else if (node->getToken().getTag() == "!")
    {
        if (expr_type != "BOOL")
        {
            throw SemanticError("(Erro de Tipo) Esperado BOOL, mas encontrado " +
                                expr_type + " na operação " + node->getToken().getValue());
        }
    }
    return expr_type;
}

/**
 * @brief Visita um bloco de código (Body) e executa a análise semântica em cada statement.
 *
 * @param block Body contendo os statements a serem analisados.
 */
void SemanticAnalyzer::visit_block(const Body &block)
{
    for (const auto &stmt : block)
    {
        visit(stmt.get());
    }
}

/**
 * @brief Avalia uma chamada de função e retorna o tipo de retorno da função.
 *
 * Valida os argumentos passados e, caso a função não esteja na tabela de funções,
 * verifica se ela faz parte das funções padrão. Se estiver, retorna o tipo padrão; caso contrário,
 * lança um erro semântico.
 *
 * @param node Ponteiro para o nó Call.
 * @return Tipo de retorno da função encapsulado em std::optional.
 * @throws SemanticError se a função não estiver declarada ou se os argumentos forem insuficientes.
 */
std::optional<std::string> SemanticAnalyzer::visit_Call(const Call *node) const
{
    std::string func_name = node->getOper().empty() ? node->getToken().getValue() : node->getOper();
    for (const auto &arg : node->getArgs())
    {
        evaluate(arg.get());
    }
    auto it = function_table.find(func_name);
    if (it == function_table.end())
    {
        if (std::find(default_func_names.begin(), default_func_names.end(), func_name) == default_func_names.end())
        {
            throw SemanticError("função " + func_name + " não declarada");
        }
        else
        {
            return DEFAULT_FUNCTION_NAMES.at(func_name);
        }
    }
    FuncDef *function = it->second;
    int required_params = 0;
    for (const auto &param : function->getParams())
    {
        if (param.second.second == nullptr)
        {
            required_params++;
        }
    }
    int call_args = node->getArgs().size();
    if (required_params > call_args)
    {
        throw SemanticError("Esperado " + std::to_string(required_params) + " argumentos, mas encontrado " +
                            std::to_string(call_args));
    }
    return function->getReturnType();
}

/**
 * @brief Avalia um array e retorna seu tipo.
 *
 * Verifica se todos os elementos do array são do mesmo tipo.
 *
 * @param node Ponteiro para o nó Array.
 * @return "ARRAY" encapsulado em std::optional se todos os elementos tiverem o mesmo tipo.
 * @throws SemanticError se os elementos do array forem de tipos diferentes.
 */
std::optional<std::string> SemanticAnalyzer::visit_Array(const Array *node) const
{
    std::string element_type = "";
    for (const auto &element : node->getElements())
    {
        std::string current_type = evaluate(element.get());
        if (element_type.empty())
        {
            element_type = current_type;
        }
        else if (current_type != element_type)
        {
            throw SemanticError("Todos os elementos do array devem ser do mesmo tipo");
        }
    }
    return "ARRAY";
}
