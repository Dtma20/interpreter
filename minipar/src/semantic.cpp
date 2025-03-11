#include "../include/semantic.hpp"
#include "../include/debug.hpp"
#include <stdexcept>
#include <algorithm>

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
    LOG_DEBUG("SemanticAnalyzer: Construtor chamado, inicializando default_func_names");
    default_func_names = {"print", "input", "sleep", "to_num", "to_string",
                          "to_bool", "send", "close", "len", "isalpha", "isnum"};
    LOG_DEBUG("SemanticAnalyzer: Construtor concluído");
}

/**
 * @brief Avalia um nó da AST e retorna o tipo resultante normalizado.
 */
std::string SemanticAnalyzer::evaluate(Node *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando evaluate para nó " << (node ? typeid(*node).name() : "nulo"));
    if (!node)
    {
        LOG_DEBUG("SemanticAnalyzer: Nó nulo, retornando string vazia");
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
        LOG_DEBUG("SemanticAnalyzer: Tipo não identificado para o nó");
        return "";
    }
    std::string normalized_type = normalize_type(raw_type);
    LOG_DEBUG("SemanticAnalyzer: Tipo normalizado: " << normalized_type);
    return normalized_type;
}

/**
 * @brief Visita um nó da AST para realizar a análise semântica.
 */
void SemanticAnalyzer::visit(Node *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit para nó " << (node ? typeid(*node).name() : "nulo"));
    if (!node)
    {
        LOG_DEBUG("SemanticAnalyzer: Nó nulo, retornando");
        return;
    }
    if (auto *module = dynamic_cast<Module *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: Detectado Module, chamando generic_visit");
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
    LOG_DEBUG("SemanticAnalyzer: Visit concluído para nó " << typeid(*node).name());
}

/**
 * @brief Visita um nó genericamente, atualizando o contexto.
 */
void SemanticAnalyzer::generic_visit(Node *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando generic_visit para nó " << typeid(*node).name());
    context_stack.push_back(node);
    if (auto *module = dynamic_cast<Module *>(node))
    {
        if (auto *seq = dynamic_cast<Seq *>(module->getStmt()))
        {
            LOG_DEBUG("SemanticAnalyzer: Visitando sequência de statements no módulo");
            for (const auto &stmt_ptr : seq->getBody())
            {
                visit(stmt_ptr.get());
            }
        }
        else
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - Módulo inválido: esperado Seq como raiz");
            throw std::runtime_error("Módulo inválido: esperado Seq como raiz");
        }
    }
    context_stack.pop_back();
    LOG_DEBUG("SemanticAnalyzer: generic_visit concluído para nó " << typeid(*node).name());
}

/**
 * @brief Valida uma atribuição, verificando compatibilidade de tipos.
 */
void SemanticAnalyzer::visit_Assign(Assign *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Assign");
    std::string left_type = evaluate(node->getLeft());
    LOG_DEBUG("SemanticAnalyzer: Tipo do lado esquerdo: " << left_type);
    std::string right_type = evaluate(node->getRight());
    LOG_DEBUG("SemanticAnalyzer: Tipo do lado direito: " << right_type);

    std::string var_name;
    if (auto *id = dynamic_cast<ID *>(node->getLeft()))
    {
        var_name = id->getToken().getValue();
    }
    else if (auto *access = dynamic_cast<Access *>(node->getLeft()))
    {
        var_name = "elemento de array";
    }
    else
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - atribuição não é para uma variável ou acesso");
        throw SemanticError("atribuição precisa ser feita para uma variável ou elemento de array");
    }

    if (left_type == right_type)
    {
        LOG_DEBUG("SemanticAnalyzer: Atribuição válida");
    }
    else if (left_type == "num" && right_type == "string")
    {
        LOG_DEBUG("SemanticAnalyzer: Conversão implícita de string para num permitida");
    }
    else
    {
        LOG_DEBUG("SemanticAnalyzer: Erro de tipo - esperado " << left_type << ", encontrado " << right_type);
        throw SemanticError("(Erro de Tipo) " + var_name + " espera " + left_type + ", mas recebeu " + right_type);
    }
}

/**
 * @brief Valida um comando de retorno (return).
 */
void SemanticAnalyzer::visit_Return(Return *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Return");
    bool in_function = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<FuncDef *>(ctx))
        {
            in_function = true;
            LOG_DEBUG("SemanticAnalyzer: Return encontrado dentro de uma função");
            break;
        }
    }
    if (!in_function)
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - return fora de uma função");
        throw SemanticError("return encontrado fora de uma declaração de função");
    }

    FuncDef *function = nullptr;
    for (auto it = context_stack.rbegin(); it != context_stack.rend(); ++it)
    {
        if (auto *func = dynamic_cast<FuncDef *>(*it))
        {
            function = func;
            LOG_DEBUG("SemanticAnalyzer: Função encontrada: " << func->getName());
            break;
        }
    }
    std::string expr_type = evaluate(node->getExpr());
    if (expr_type != function->getReturnType())
    {
        LOG_DEBUG("SemanticAnalyzer: Erro de tipo no return - esperado " << function->getReturnType() << ", encontrado " << expr_type);
        throw SemanticError("retorno em " + function->getName() + " tem tipo diferente do definido");
    }
    LOG_DEBUG("SemanticAnalyzer: Return válido");
}

/**
 * @brief Valida um comando break.
 */
void SemanticAnalyzer::visit_Break(Break *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Break");
    bool in_loop = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<While *>(ctx))
        {
            in_loop = true;
            LOG_DEBUG("SemanticAnalyzer: Break encontrado dentro de um loop");
            break;
        }
    }
    if (!in_loop)
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - break fora de um loop");
        throw SemanticError("break encontrado fora de uma declaração de um loop");
    }
    LOG_DEBUG("SemanticAnalyzer: Break válido");
}

/**
 * @brief Valida um comando continue.
 */
void SemanticAnalyzer::visit_Continue(Continue *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Continue");
    bool in_loop = false;
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<While *>(ctx))
        {
            in_loop = true;
            LOG_DEBUG("SemanticAnalyzer: Continue encontrado dentro de um loop");
            break;
        }
    }
    if (!in_loop)
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - continue fora de um loop");
        throw SemanticError("continue encontrado fora de uma declaração de um loop");
    }
    LOG_DEBUG("SemanticAnalyzer: Continue válido");
}

/**
 * @brief Valida a declaração de uma função.
 */
void SemanticAnalyzer::visit_FuncDef(FuncDef *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_FuncDef para função " << node->getName());
    for (auto *ctx : context_stack)
    {
        if (dynamic_cast<If *>(ctx) || dynamic_cast<While *>(ctx) || dynamic_cast<Par *>(ctx))
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - função declarada em escopo local inválido");
            throw SemanticError("não é possível declarar funções dentro de escopos locais");
        }
    }
    if (function_table.find(node->getName()) == function_table.end())
    {
        LOG_DEBUG("SemanticAnalyzer: Registrando função " << node->getName() << " na tabela de funções");
        function_table[node->getName()] = node;
    }
    generic_visit(node);
    LOG_DEBUG("SemanticAnalyzer: visit_FuncDef concluído para " << node->getName());
}

/**
 * @brief Valida uma estrutura condicional if.
 */
void SemanticAnalyzer::visit_If(If *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_If");
    std::string cond_type = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: Tipo da condição: " << cond_type);
    if (cond_type != "bool")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - condição do if não é bool");
        throw SemanticError("esperado bool, mas encontrado " + cond_type);
    }
    context_stack.push_back(node);
    LOG_DEBUG("SemanticAnalyzer: Visitando bloco do if");
    visit_block(node->getBody());
    if (node->getElseStmt())
    {
        LOG_DEBUG("SemanticAnalyzer: Visitando bloco do else");
        visit_block(*node->getElseStmt());
    }
    context_stack.pop_back();
    LOG_DEBUG("SemanticAnalyzer: visit_If concluído");
}

/**
 * @brief Valida uma estrutura de repetição while.
 */
void SemanticAnalyzer::visit_While(While *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_While");
    std::string cond_type = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: Tipo da condição: " << cond_type);
    if (cond_type != "bool")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - condição do while não é bool");
        throw SemanticError("esperado bool, mas encontrado " + cond_type);
    }
    context_stack.push_back(node);
    LOG_DEBUG("SemanticAnalyzer: Visitando bloco do while");
    visit_block(node->getBody());
    context_stack.pop_back();
    LOG_DEBUG("SemanticAnalyzer: visit_While concluído");
}

/**
 * @brief Valida um bloco de execução paralela.
 */
void SemanticAnalyzer::visit_Par(Par *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Par");
    for (const auto &inst : node->getBody())
    {
        if (!dynamic_cast<Call *>(inst.get()))
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - statement em Par não é uma chamada de função");
            throw SemanticError("esperado apenas funções em um bloco de execução paralela");
        }
        LOG_DEBUG("SemanticAnalyzer: Chamada de função válida em Par");
    }
    LOG_DEBUG("SemanticAnalyzer: visit_Par concluído");
}

/**
 * @brief Valida um canal de cliente (CChannel).
 */
void SemanticAnalyzer::visit_CChannel(CChannel *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_CChannel para " << node->getName());
    std::string localhost_type = evaluate(node->getLocalhostNode());
    LOG_DEBUG("SemanticAnalyzer: Tipo de localhost: " << localhost_type);
    if (localhost_type != "string")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - localhost não é string");
        throw SemanticError("localhost em " + node->getName() + " precisa ser string");
    }
    std::string port_type = evaluate(node->getPortNode());
    LOG_DEBUG("SemanticAnalyzer: Tipo de port: " << port_type);
    if (port_type != "num")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - port não é num");
        throw SemanticError("port em " + node->getName() + " precisa ser num");
    }
    LOG_DEBUG("SemanticAnalyzer: visit_CChannel concluído");
}

/**
 * @brief Valida um canal de serviço (SChannel).
 */
void SemanticAnalyzer::visit_SChannel(SChannel *node)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_SChannel para " << node->getName());
    auto it = function_table.find(node->getFuncName());
    if (it == function_table.end())
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - função " << node->getFuncName() << " não declarada");
        throw SemanticError("função " + node->getFuncName() + " não declarada");
    }
    FuncDef *func = it->second;
    LOG_DEBUG("SemanticAnalyzer: Função associada: " << func->getName());
    if (func->getReturnType() != "string")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - retorno da função não é string");
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
    LOG_DEBUG("SemanticAnalyzer: Parâmetros requeridos: " << required_params << ", argumentos fornecidos: " << call_args);
    if (required_params > call_args)
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - número insuficiente de argumentos");
        throw SemanticError("Esperado " + std::to_string(required_params) + " argumentos, mas encontrado " +
                            std::to_string(call_args));
    }
    std::string description_type = evaluate(node->getDescription());
    LOG_DEBUG("SemanticAnalyzer: Tipo de description: " << description_type);
    if (description_type != "string")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - description não é string");
        throw SemanticError("description em " + node->getName() + " precisa ser string");
    }
    std::string localhost_type = evaluate(node->getLocalhostNode());
    LOG_DEBUG("SemanticAnalyzer: Tipo de localhost: " << localhost_type);
    if (localhost_type != "string")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - localhost não é string");
        throw SemanticError("localhost em " + node->getName() + " precisa ser string");
    }
    std::string port_type = evaluate(node->getPortNode());
    LOG_DEBUG("SemanticAnalyzer: Tipo de port: " << port_type);
    if (port_type != "num")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - port não é num");
        throw SemanticError("port em " + node->getName() + " precisa ser num");
    }
    LOG_DEBUG("SemanticAnalyzer: visit_SChannel concluído");
}

/**
 * @brief Avalia uma constante e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_Constant(const Constant *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Constant");
    std::string type = node->getType();
    LOG_DEBUG("SemanticAnalyzer: Tipo da constante: " << type);
    return type;
}

/**
 * @brief Avalia um identificador e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_ID(const ID *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_ID para " << node->getToken().getValue());
    std::string type = node->getType();
    LOG_DEBUG("SemanticAnalyzer: Tipo do identificador: " << type);
    return type;
}

/**
 * @brief Avalia um acesso a elemento e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_Access(const Access *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Access");
    if (node->getType() != "STRING")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - acesso por índice não é em uma string");
        throw SemanticError("Acesso por index é válido apenas em strings");
    }
    LOG_DEBUG("SemanticAnalyzer: Tipo do acesso: string");
    return "string";
}

/**
 * @brief Avalia uma operação lógica e retorna "bool".
 */
std::optional<std::string> SemanticAnalyzer::visit_Logical(const Logical *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Logical para operação " << node->getToken().getValue());
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    LOG_DEBUG("SemanticAnalyzer: Tipos dos operandos: " << left_type << " e " << right_type);
    if (left_type != "bool" || right_type != "bool")
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - operandos não são bool");
        throw SemanticError("(Erro de Tipo) Esperado bool, mas encontrado " + left_type +
                            " e " + right_type + " na operação " + node->getToken().getValue());
    }
    LOG_DEBUG("SemanticAnalyzer: Operação lógica válida, retornando bool");
    return "bool";
}

/**
 * @brief Avalia uma operação relacional e retorna "bool".
 */
std::optional<std::string> SemanticAnalyzer::visit_Relational(const Relational *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Relational para operação " << node->getToken().getValue());
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    LOG_DEBUG("SemanticAnalyzer: Tipos dos operandos: " << left_type << " e " << right_type);
    if (node->getToken().getValue() == "==" || node->getToken().getValue() == "!=")
    {
        if (left_type != right_type)
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - tipos diferentes em comparação");
            throw SemanticError("(Erro de Tipo) Esperado tipos iguais, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    else
    {
        if (left_type != "num" || right_type != "num")
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - operandos não são num");
            throw SemanticError("(Erro de Tipo) Esperado num, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    LOG_DEBUG("SemanticAnalyzer: Operação relacional válida, retornando bool");
    return "bool";
}

/**
 * @brief Avalia uma operação aritmética e retorna o tipo do operando.
 */
std::optional<std::string> SemanticAnalyzer::visit_Arithmetic(const Arithmetic *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Arithmetic para operação " << node->getToken().getValue());
    std::string left_type = evaluate(node->getLeft());
    std::string right_type = evaluate(node->getRight());
    LOG_DEBUG("SemanticAnalyzer: Tipos dos operandos: " << left_type << " e " << right_type);
    if (node->getToken().getValue() == "+")
    {
        if (left_type != right_type)
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - tipos diferentes na soma");
            throw SemanticError("(Erro de Tipo) Esperado tipos iguais, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    else
    {
        if (left_type != "num" || right_type != "num")
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - operandos não são num");
            throw SemanticError("(Erro de Tipo) Esperado num, mas encontrado " +
                                left_type + " e " + right_type + " na operação " + node->getToken().getValue());
        }
    }
    LOG_DEBUG("SemanticAnalyzer: Operação aritmética válida, retornando " << left_type);
    return left_type;
}

/**
 * @brief Avalia uma operação unária e retorna o tipo do operando.
 */
std::optional<std::string> SemanticAnalyzer::visit_Unary(const Unary *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Unary para operação " << node->getToken().getValue());
    std::string expr_type = evaluate(node->getExpr());
    LOG_DEBUG("SemanticAnalyzer: Tipo do operando: " << expr_type);
    if (node->getToken().getTag() == "-")
    {
        if (expr_type != "num")
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - operando não é num");
            throw SemanticError("(Erro de Tipo) Esperado num, mas encontrado " +
                                expr_type + " na operação " + node->getToken().getValue());
        }
    }
    else if (node->getToken().getTag() == "!")
    {
        if (expr_type != "bool")
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - operando não é bool");
            throw SemanticError("(Erro de Tipo) Esperado bool, mas encontrado " +
                                expr_type + " na operação " + node->getToken().getValue());
        }
    }
    LOG_DEBUG("SemanticAnalyzer: Operação unária válida, retornando " << expr_type);
    return expr_type;
}

/**
 * @brief Visita um bloco de código (Body).
 */
void SemanticAnalyzer::visit_block(const Body &block)
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_block");
    for (const auto &stmt : block)
    {
        LOG_DEBUG("SemanticAnalyzer: Visitando statement no bloco");
        visit(stmt.get());
    }
    LOG_DEBUG("SemanticAnalyzer: visit_block concluído");
}

/**
 * @brief Avalia uma chamada de função e retorna o tipo de retorno.
 */
std::optional<std::string> SemanticAnalyzer::visit_Call(const Call *node) const
{
    std::string func_name = node->getOper().empty() ? node->getToken().getValue() : node->getOper();
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Call para função " << func_name);
    for (const auto &arg : node->getArgs())
    {
        std::string arg_type = evaluate(arg.get());
        LOG_DEBUG("SemanticAnalyzer: Tipo do argumento: " << arg_type);
    }
    auto it = function_table.find(func_name);
    if (it == function_table.end())
    {
        if (std::find(default_func_names.begin(), default_func_names.end(), func_name) == default_func_names.end())
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - função " << func_name << " não declarada");
            throw SemanticError("função " + func_name + " não declarada");
        }
        else
        {
            std::string type = DEFAULT_FUNCTION_NAMES.at(func_name);
            LOG_DEBUG("SemanticAnalyzer: Função padrão " << func_name << " encontrada, retornando " << type);
            return type;
        }
    }
    FuncDef *function = it->second;
    LOG_DEBUG("SemanticAnalyzer: Função definida encontrada: " << function->getName());
    int required_params = 0;
    for (const auto &param : function->getParams())
    {
        if (param.second.second == nullptr)
        {
            required_params++;
        }
    }
    int call_args = node->getArgs().size();
    LOG_DEBUG("SemanticAnalyzer: Parâmetros requeridos: " << required_params << ", argumentos fornecidos: " << call_args);
    if (required_params > call_args)
    {
        LOG_DEBUG("SemanticAnalyzer: Erro - número insuficiente de argumentos");
        throw SemanticError("Esperado " + std::to_string(required_params) + " argumentos, mas encontrado " +
                            std::to_string(call_args));
    }
    std::string return_type = function->getReturnType();
    LOG_DEBUG("SemanticAnalyzer: Tipo de retorno da função: " << return_type);
    return return_type;
}

/**
 * @brief Avalia um array e retorna seu tipo.
 */
std::optional<std::string> SemanticAnalyzer::visit_Array(const Array *node) const
{
    LOG_DEBUG("SemanticAnalyzer: Iniciando visit_Array");
    std::string element_type = "";
    for (const auto &element : node->getElements())
    {
        std::string current_type = evaluate(element.get());
        LOG_DEBUG("SemanticAnalyzer: Tipo do elemento: " << current_type);
        if (element_type.empty())
        {
            element_type = current_type;
        }
        else if (current_type != element_type)
        {
            LOG_DEBUG("SemanticAnalyzer: Erro - tipos diferentes no array");
            throw SemanticError("Todos os elementos do array devem ser do mesmo tipo");
        }
    }
    LOG_DEBUG("SemanticAnalyzer: Array válido, retornando ARRAY");
    return "ARRAY";
}