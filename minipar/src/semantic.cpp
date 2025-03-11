#include "../include/semantic.hpp"
#include <stdexcept>
#include <algorithm>
#include <iostream>

// Função auxiliar para normalizar tipos
static std::string normalize_type(const std::string &type)
{
    if (type == "STRING")
        return "string";
    if (type == "NUM")
        return "num";
    if (type == "BOOL")
        return "bool";
    return type;
}

/**
 * @brief Construtor do SemanticAnalyzer.
 */
SemanticAnalyzer::SemanticAnalyzer()
{
    std::cout << "[DEBUG] SemanticAnalyzer: Construtor chamado, inicializando default_func_names" << std::endl;
    default_func_names = {"print", "input", "sleep", "to_num", "to_string",
                          "to_bool", "send", "close", "len", "isalpha", "isnum"};
    std::cout << "[DEBUG] SemanticAnalyzer: Construtor concluído" << std::endl;
}

/**
 * @brief Avalia um nó da AST e retorna o tipo resultante normalizado.
 */
std::string SemanticAnalyzer::evaluate(Node *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando evaluate para nó " << (node ? typeid(*node).name() : "nulo") << std::endl;
    if (!node)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Nó nulo, retornando string vazia" << std::endl;
        return "";
    }
    std::string raw_type;
    if (auto *constant = dynamic_cast<Constant *>(node))
    {
        raw_type = visit_Constant(constant).value_or("");
    }
    else if (auto *id = dynamic_cast<ID *>(node))
    {
        raw_type = visit_ID(id).value_or("");
    }
    else if (auto *access = dynamic_cast<Access *>(node))
    {
        raw_type = visit_Access(access).value_or("");
    }
    else if (auto *logical = dynamic_cast<Logical *>(node))
    {
        raw_type = visit_Logical(logical).value_or("");
    }
    else if (auto *relational = dynamic_cast<Relational *>(node))
    {
        raw_type = visit_Relational(relational).value_or("");
    }
    else if (auto *arithmetic = dynamic_cast<Arithmetic *>(node))
    {
        raw_type = visit_Arithmetic(arithmetic).value_or("");
    }
    else if (auto *unary = dynamic_cast<Unary *>(node))
    {
        raw_type = visit_Unary(unary).value_or("");
    }
    else if (auto *call = dynamic_cast<Call *>(node))
    {
        raw_type = visit_Call(call).value_or("");
    }
    else
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Tipo não identificado para o nó" << std::endl;
        return "";
    }
    std::string normalized_type = normalize_type(raw_type);
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo normalizado: " << normalized_type << std::endl;
    return normalized_type;
}

/**
 * @brief Visita um nó da AST para realizar a análise semântica.
 */
void SemanticAnalyzer::visit(Node *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit para nó " << (node ? typeid(*node).name() : "nulo") << std::endl;
    if (!node)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Nó nulo, retornando" << std::endl;
        return;
    }
    if (auto *module = dynamic_cast<Module *>(node))
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Detectado Module, chamando generic_visit" << std::endl;
        generic_visit(module);
    }
    else if (auto *assign = dynamic_cast<Assign *>(node))
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
    std::cout << "[DEBUG] SemanticAnalyzer: Visit concluído para nó " << typeid(*node).name() << std::endl;
}

/**
 * @brief Visita um nó genericamente, atualizando o contexto.
 */
void SemanticAnalyzer::generic_visit(Node *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando generic_visit para nó " << typeid(*node).name() << std::endl;
    context_stack.push_back(node);
    if (auto *module = dynamic_cast<Module *>(node))
    {
        if (auto *seq = dynamic_cast<Seq *>(module->getStmt()))
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Visitando sequência de statements no módulo" << std::endl;
            for (const auto &stmt_ptr : seq->getBody())
            {
                visit(stmt_ptr.get());
            }
        }
        else
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - Módulo inválido: esperado Seq como raiz" << std::endl;
            throw std::runtime_error("Módulo inválido: esperado Seq como raiz");
        }
    }
    context_stack.pop_back();
    std::cout << "[DEBUG] SemanticAnalyzer: generic_visit concluído para nó " << typeid(*node).name() << std::endl;
}

/**
 * @brief Valida uma atribuição, verificando compatibilidade de tipos.
 */
void SemanticAnalyzer::visit_Assign(Assign *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Assign" << std::endl;
    std::string left_type = evaluate(node->getLeft());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo do lado esquerdo: " << left_type << std::endl;
    std::string right_type = evaluate(node->getRight());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo do lado direito: " << right_type << std::endl;

    // Verifica se o lado esquerdo é um lvalue válido (ID ou Access)
    std::string var_name;
    if (auto *id = dynamic_cast<ID *>(node->getLeft()))
    {
        var_name = id->getToken().getValue();
    }
    else if (auto *access = dynamic_cast<Access *>(node->getLeft()))
    {
        var_name = "elemento de array"; // Simplificação para mensagem de erro
    }
    else
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - atribuição não é para uma variável ou acesso" << std::endl;
        throw SemanticError("atribuição precisa ser feita para uma variável ou elemento de array");
    }

    // Verificação de compatibilidade de tipos
    if (left_type == right_type)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Atribuição válida" << std::endl;
    }
    else if (left_type == "num" && right_type == "string")
    {
        // Exemplo de conversão implícita (ajustar conforme regras da linguagem)
        std::cout << "[DEBUG] SemanticAnalyzer: Conversão implícita de string para num permitida" << std::endl;
    }
    else
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro de tipo - esperado " << left_type << ", encontrado " << right_type << std::endl;
        throw SemanticError("(Erro de Tipo) " + var_name + " espera " + left_type + ", mas recebeu " + right_type);
    }
}

/**
 * @brief Valida um comando de retorno (return).
 */
void SemanticAnalyzer::visit_Return(Return *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Return" << std::endl;
    bool in_function = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<FuncDef *>(ctx))
        {
            in_function = true;
            std::cout << "[DEBUG] SemanticAnalyzer: Return encontrado dentro de uma função" << std::endl;
            break;
        }
    }
    if (!in_function)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - return fora de uma função" << std::endl;
        throw SemanticError("return encontrado fora de uma declaração de função");
    }

    FuncDef *function = nullptr;
    for (auto it = context_stack.rbegin(); it != context_stack.rend(); ++it)
    {
        if (auto *func = dynamic_cast<FuncDef *>(*it))
        {
            function = func;
            std::cout << "[DEBUG] SemanticAnalyzer: Função encontrada: " << func->getName() << std::endl;
            break;
        }
    }
    std::string expr_type = evaluate(node->getExpr());
    if (expr_type != function->getReturnType())
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro de tipo no return - esperado " << function->getReturnType() << ", encontrado " << expr_type << std::endl;
        throw SemanticError("retorno em " + function->getName() + " tem tipo diferente do definido");
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Return válido" << std::endl;
}

/**
 * @brief Valida um comando break.
 */
void SemanticAnalyzer::visit_Break(Break *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Break" << std::endl;
    bool in_loop = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<While *>(ctx))
        {
            in_loop = true;
            std::cout << "[DEBUG] SemanticAnalyzer: Break encontrado dentro de um loop" << std::endl;
            break;
        }
    }
    if (!in_loop)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - break fora de um loop" << std::endl;
        throw SemanticError("break encontrado fora de uma declaração de um loop");
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Break válido" << std::endl;
}

/**
 * @brief Valida um comando continue.
 */
void SemanticAnalyzer::visit_Continue(Continue *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Continue" << std::endl;
    bool in_loop = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<While *>(ctx))
        {
            in_loop = true;
            std::cout << "[DEBUG] SemanticAnalyzer: Continue encontrado dentro de um loop" << std::endl;
            break;
        }
    }
    if (!in_loop)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - continue fora de um loop" << std::endl;
        throw SemanticError("continue encontrado fora de uma declaração de um loop");
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Continue válido" << std::endl;
}

/**
 * @brief Valida a declaração de uma função.
 */
void SemanticAnalyzer::visit_FuncDef(FuncDef *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_FuncDef para função " << node->getName() << std::endl;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<If *>(ctx) || dynamic_cast<While *>(ctx) || dynamic_cast<Par *>(ctx))
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - função declarada em escopo local inválido" << std::endl;
            throw SemanticError("não é possível declarar funções dentro de escopos locais");
        }
    }
    if (function_table.find(node->getName()) == function_table.end())
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Registrando função " << node->getName() << " na tabela de funções" << std::endl;
        function_table[node->getName()] = node;
    }
    generic_visit(node);
    std::cout << "[DEBUG] SemanticAnalyzer: visit_FuncDef concluído para " << node->getName() << std::endl;
}

/**
 * @brief Valida uma estrutura condicional if.
 */
void SemanticAnalyzer::visit_If(If *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_If" << std::endl;
    std::string cond_type = evaluate(node->getCondition());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo da condição: " << cond_type << std::endl;
    if (cond_type != "bool")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - condição do if não é bool" << std::endl;
        throw SemanticError("esperado bool, mas encontrado " + cond_type);
    }
    context_stack.push_back(node);
    std::cout << "[DEBUG] SemanticAnalyzer: Visitando bloco do if" << std::endl;
    visit_block(node->getBody());
    if (node->getElseStmt())
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Visitando bloco do else" << std::endl;
        visit_block(*node->getElseStmt());
    }
    context_stack.pop_back();
    std::cout << "[DEBUG] SemanticAnalyzer: visit_If concluído" << std::endl;
}

/**
 * @brief Valida uma estrutura de repetição while.
 */
void SemanticAnalyzer::visit_While(While *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_While" << std::endl;
    std::string cond_type = evaluate(node->getCondition());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo da condição: " << cond_type << std::endl;
    if (cond_type != "bool")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - condição do while não é bool" << std::endl;
        throw SemanticError("esperado bool, mas encontrado " + cond_type);
    }
    context_stack.push_back(node);
    std::cout << "[DEBUG] SemanticAnalyzer: Visitando bloco do while" << std::endl;
    visit_block(node->getBody());
    context_stack.pop_back();
    std::cout << "[DEBUG] SemanticAnalyzer: visit_While concluído" << std::endl;
}

/**
 * @brief Valida um bloco de execução paralela.
 */
void SemanticAnalyzer::visit_Par(Par *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Par" << std::endl;
    for (const auto &inst : node->getBody())
    {
        if (!dynamic_cast<Call *>(inst.get()))
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - statement em Par não é uma chamada de função" << std::endl;
            throw SemanticError("esperado apenas funções em um bloco de execução paralela");
        }
        std::cout << "[DEBUG] SemanticAnalyzer: Chamada de função válida em Par" << std::endl;
    }
    std::cout << "[DEBUG] SemanticAnalyzer: visit_Par concluído" << std::endl;
}

/**
 * @brief Valida um canal de cliente (CChannel).
 */
void SemanticAnalyzer::visit_CChannel(CChannel *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_CChannel para " << node->getName() << std::endl;
    std::string localhost_type = evaluate(node->getLocalhostNode());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo de localhost: " << localhost_type << std::endl;
    if (localhost_type != "string")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - localhost não é string" << std::endl;
        throw SemanticError("localhost em " + node->getName() + " precisa ser string");
    }
    std::string port_type = evaluate(node->getPortNode());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo de port: " << port_type << std::endl;
    if (port_type != "num")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - port não é num" << std::endl;
        throw SemanticError("port em " + node->getName() + " precisa ser num");
    }
    std::cout << "[DEBUG] SemanticAnalyzer: visit_CChannel concluído" << std::endl;
}

/**
 * @brief Valida um canal de serviço (SChannel).
 */
void SemanticAnalyzer::visit_SChannel(SChannel *node)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_SChannel para " << node->getName() << std::endl;
    auto it = function_table.find(node->getFuncName());
    if (it == function_table.end())
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - função " << node->getFuncName() << " não declarada" << std::endl;
        throw SemanticError("função " + node->getFuncName() + " não declarada");
    }
    FuncDef *func = it->second;
    std::cout << "[DEBUG] SemanticAnalyzer: Função associada: " << func->getName() << std::endl;
    if (func->getReturnType() != "string")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - retorno da função não é string" << std::endl;
        throw SemanticError("função base de " + node->getName() + " precisa ter retorno string");
    }
    int required_params = 0;
    for (const auto &param : func->getParams())
    {
        if (param.second.second == nullptr)
        {
            required_params++;
        }
    }
    int call_args = 0; // Corrigir para contar argumentos reais se aplicável
    std::cout << "[DEBUG] SemanticAnalyzer: Parâmetros requeridos: " << required_params << ", argumentos fornecidos: " << call_args << std::endl;
    if (required_params > call_args)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - número insuficiente de argumentos" << std::endl;
        throw SemanticError("Esperado " + std::to_string(required_params) + " argumentos, mas encontrado " +
                            std::to_string(call_args));
    }
    std::string description_type = evaluate(node->getDescription());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo de description: " << description_type << std::endl;
    if (description_type != "string")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - description não é string" << std::endl;
        throw SemanticError("description em " + node->getName() + " precisa ser string");
    }
    std::string localhost_type = evaluate(node->getLocalhostNode());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo de localhost: " << localhost_type << std::endl;
    if (localhost_type != "string")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - localhost não é string" << std::endl;
        throw SemanticError("localhost em " + node->getName() + " precisa ser string");
    }
    std::string port_type = evaluate(node->getPortNode());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo de port: " << port_type << std::endl;
    if (port_type != "num")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - port não é num" << std::endl;
        throw SemanticError("port em " + node->getName() + " precisa ser num");
    }
    std::cout << "[DEBUG] SemanticAnalyzer: visit_SChannel concluído" << std::endl;
}

/**
 * @brief Avalia uma constante e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_Constant(const Constant *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Constant" << std::endl;
    std::string type = node->getType();
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo da constante: " << type << std::endl;
    return type;
}

/**
 * @brief Avalia um identificador e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_ID(const ID *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_ID para " << node->getToken().getValue() << std::endl;
    std::string type = node->getType();
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo do identificador: " << type << std::endl;
    return type;
}

/**
 * @brief Avalia um acesso a elemento e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_Access(const Access *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Access" << std::endl;
    if (node->getType() != "STRING")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - acesso por índice não é em uma string" << std::endl;
        throw SemanticError("Acesso por index é válido apenas em strings");
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo do acesso: string" << std::endl;
    return "string"; // Normalizado para "string" em vez de "STRING"
}

/**
 * @brief Avalia uma operação lógica e retorna "bool".
 */
std::optional<std::string> SemanticAnalyzer::visit_Logical(const Logical *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Logical para operação " << node->getToken().getValue() << std::endl;
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipos dos operandos: " << left_type << " e " << right_type << std::endl;
    if (left_type != "bool" || right_type != "bool")
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - operandos não são bool" << std::endl;
        throw SemanticError("(Erro de Tipo) Esperado bool, mas encontrado " + left_type +
                            " e " + right_type + " na operação " + node->getToken().getValue());
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Operação lógica válida, retornando bool" << std::endl;
    return "bool";
}

/**
 * @brief Avalia uma operação relacional e retorna "bool".
 */
std::optional<std::string> SemanticAnalyzer::visit_Relational(const Relational *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Relational para operação " << node->getToken().getValue() << std::endl;
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipos dos operandos: " << left_type << " e " << right_type << std::endl;
    if (node->getToken().getValue() == "==" || node->getToken().getValue() == "!=")
    {
        if (left_type != right_type)
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - tipos diferentes em comparação" << std::endl;
            throw SemanticError("(Erro de Tipo) Esperado tipos iguais, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    else
    {
        if (left_type != "num" || right_type != "num")
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - operandos não são num" << std::endl;
            throw SemanticError("(Erro de Tipo) Esperado num, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Operação relacional válida, retornando bool" << std::endl;
    return "bool";
}

/**
 * @brief Avalia uma operação aritmética e retorna o tipo do operando.
 */
std::optional<std::string> SemanticAnalyzer::visit_Arithmetic(const Arithmetic *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Arithmetic para operação " << node->getToken().getValue() << std::endl;
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipos dos operandos: " << left_type << " e " << right_type << std::endl;
    if (node->getToken().getValue() == "+")
    {
        if (left_type != right_type)
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - tipos diferentes na soma" << std::endl;
            throw SemanticError("(Erro de Tipo) Esperado tipos iguais, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    else
    {
        if (left_type != "num" || right_type != "num")
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - operandos não são num" << std::endl;
            throw SemanticError("(Erro de Tipo) Esperado num, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Operação aritmética válida, retornando " << left_type << std::endl;
    return left_type;
}

/**
 * @brief Avalia uma operação unária e retorna o tipo do operando.
 */
std::optional<std::string> SemanticAnalyzer::visit_Unary(const Unary *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Unary para operação " << node->getToken().getValue() << std::endl;
    std::string expr_type = evaluate(node->getExpr());
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo do operando: " << expr_type << std::endl;
    if (node->getToken().getTag() == "-")
    {
        if (expr_type != "num")
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - operando não é num" << std::endl;
            throw SemanticError("(Erro de Tipo) Esperado num, mas encontrado " +
                                expr_type + " na operação " + node->getToken().getValue());
        }
    }
    else if (node->getToken().getTag() == "!")
    {
        if (expr_type != "bool")
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - operando não é bool" << std::endl;
            throw SemanticError("(Erro de Tipo) Esperado bool, mas encontrado " +
                                expr_type + " na operação " + node->getToken().getValue());
        }
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Operação unária válida, retornando " << expr_type << std::endl;
    return expr_type;
}

/**
 * @brief Visita um bloco de código (Body).
 */
void SemanticAnalyzer::visit_block(const Body &block)
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_block" << std::endl;
    for (const auto &stmt : block)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Visitando statement no bloco" << std::endl;
        visit(stmt.get());
    }
    std::cout << "[DEBUG] SemanticAnalyzer: visit_block concluído" << std::endl;
}

/**
 * @brief Avalia uma chamada de função e retorna o tipo de retorno.
 */
std::optional<std::string> SemanticAnalyzer::visit_Call(const Call *node) const
{
    std::string func_name = node->getOper().empty() ? node->getToken().getValue() : node->getOper();
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Call para função " << func_name << std::endl;
    for (const auto &arg : node->getArgs())
    {
        std::string arg_type = evaluate(arg.get());
        std::cout << "[DEBUG] SemanticAnalyzer: Tipo do argumento: " << arg_type << std::endl;
    }
    auto it = function_table.find(func_name);
    if (it == function_table.end())
    {
        if (std::find(default_func_names.begin(), default_func_names.end(), func_name) == default_func_names.end())
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - função " << func_name << " não declarada" << std::endl;
            throw SemanticError("função " + func_name + " não declarada");
        }
        else
        {
            std::string type = DEFAULT_FUNCTION_NAMES.at(func_name);
            std::cout << "[DEBUG] SemanticAnalyzer: Função padrão " << func_name << " encontrada, retornando " << type << std::endl;
            return type;
        }
    }
    FuncDef *function = it->second;
    std::cout << "[DEBUG] SemanticAnalyzer: Função definida encontrada: " << function->getName() << std::endl;
    int required_params = 0;
    for (const auto &param : function->getParams())
    {
        if (param.second.second == nullptr)
        {
            required_params++;
        }
    }
    int call_args = node->getArgs().size();
    std::cout << "[DEBUG] SemanticAnalyzer: Parâmetros requeridos: " << required_params << ", argumentos fornecidos: " << call_args << std::endl;
    if (required_params > call_args)
    {
        std::cout << "[DEBUG] SemanticAnalyzer: Erro - número insuficiente de argumentos" << std::endl;
        throw SemanticError("Esperado " + std::to_string(required_params) + " argumentos, mas encontrado " +
                            std::to_string(call_args));
    }
    std::string return_type = function->getReturnType();
    std::cout << "[DEBUG] SemanticAnalyzer: Tipo de retorno da função: " << return_type << std::endl;
    return return_type;
}

/**
 * @brief Avalia um array e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_Array(const Array *node) const
{
    std::cout << "[DEBUG] SemanticAnalyzer: Iniciando visit_Array" << std::endl;
    std::string element_type = "";
    for (const auto &element : node->getElements())
    {
        std::string current_type = evaluate(element.get());
        std::cout << "[DEBUG] SemanticAnalyzer: Tipo do elemento: " << current_type << std::endl;
        if (element_type.empty())
        {
            element_type = current_type;
        }
        else if (current_type != element_type)
        {
            std::cout << "[DEBUG] SemanticAnalyzer: Erro - tipos diferentes no array" << std::endl;
            throw SemanticError("Todos os elementos do array devem ser do mesmo tipo");
        }
    }
    std::cout << "[DEBUG] SemanticAnalyzer: Array válido, retornando ARRAY" << std::endl;
    return "ARRAY";
}