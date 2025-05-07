#include "../include/semantic.hpp"
#include "../include/debug.hpp"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>

static const std::unordered_map<std::string, std::string> builtin_return = {
    {"print",     "string"},
    {"len",       "num"},
    {"to_num",    "num"},
    {"to_string", "string"},
    {"isnum",     "bool"},
    {"isalpha",   "bool"},
    {"exp",       "num"},
    {"randf",     "num"},
    {"randi",     "num"},
    {"input",     "string"},
    {"send",     "string"},
    {"close",     "void"},
};

/**
 * @brief Construtor do SemanticAnalyzer.
 *
 * Inicializa o analisador semântico com um escopo global vazio e uma lista
 * de funções padrão da linguagem Minipar.
 */
SemanticAnalyzer::SemanticAnalyzer()
{
    LOG_DEBUG("SemanticAnalyzer: Construtor chamado");
    scope_stack.emplace_back();
    LOG_DEBUG("SemanticAnalyzer: default_funcs inicializadas");
}

/**
 * @brief Normaliza uma string de tipo para sua forma canônica.
 *
 * Converte strings de tipo como "NUM", "num", "STRING", "string", "BOOL" e "bool"
 * para as formas canônicas "num", "string" e "bool", respectivamente. Outros tipos
 * são retornados sem alteração.
 *
 * @param raw A string de tipo a ser normalizada.
 * @return A string de tipo normalizada.
 */
std::string SemanticAnalyzer::normalize(const std::string &raw) const
{
    LOG_DEBUG("SemanticAnalyzer: Normalizando tipo " << raw);
    if (raw == "NUM" || raw == "num")
        return "num";
    if (raw == "STRING" || raw == "string")
        return "string";
    if (raw == "BOOL" || raw == "bool")
        return "bool";
    return raw;
}

/**
 * @brief Avalia o tipo de um nó AST.
 *
 * Determina o tipo de um nó da Árvore de Sintaxe Abstrata (AST) delegando a análise
 * ao método visitante apropriado com base no tipo dinâmico do nó. Se o nó for nulo,
 * retorna uma string vazia.
 *
 * @param node Ponteiro para o nó AST a ser avaliado.
 * @return O tipo normalizado do nó como uma string.
 */
std::string SemanticAnalyzer::evaluate(Node *node) const
{
    if (!node)
    {
        LOG_DEBUG("SemanticAnalyzer: evaluate recebido nó nulo");
        return "";
    }
    LOG_DEBUG("SemanticAnalyzer: Evaluating node type " << typeid(*node).name());
    std::string raw;
    if (auto c = dynamic_cast<Constant *>(node))
        raw = visit_Constant(c).value_or("");
    else if (auto id = dynamic_cast<ID *>(node))
        raw = visit_ID(id).value_or("");
    else if (auto a = dynamic_cast<Access *>(node))
        raw = visit_Access(a).value_or("");
    else if (auto l = dynamic_cast<Logical *>(node))
        raw = visit_Logical(l).value_or("");
    else if (auto r = dynamic_cast<Relational *>(node))
        raw = visit_Relational(r).value_or("");
    else if (auto ar = dynamic_cast<Arithmetic *>(node))
        raw = visit_Arithmetic(ar).value_or("");
    else if (auto u = dynamic_cast<Unary *>(node))
        raw = visit_Unary(u).value_or("");
    else if (auto call = dynamic_cast<Call *>(node))
        raw = visit_Call(call).value_or("");
    else if (auto arr = dynamic_cast<Array *>(node))
        raw = visit_Array(arr).value_or("");
    if (raw.empty())
        throw SemanticError(
            std::string("Tipo não suportado para nó: ") +
            typeid(*node).name()
        );
    std::string norm = normalize(raw);
    LOG_DEBUG("SemanticAnalyzer: Type normalized to " << norm);
    return norm;
}

/**
 * @brief Realiza a análise semântica de um nó.
 *
 * Executa uma visita em profundidade no nó fornecido e seus filhos, analisando a
 * semântica do programa de acordo com as regras da linguagem.
 *
 * @param node O nó a ser analisado.
 */
void SemanticAnalyzer::visit(Node *node)
{
    if (!node)
        return;
    LOG_DEBUG("SemanticAnalyzer: visit node type " << typeid(*node).name());

    if (auto mod = dynamic_cast<Module *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Module");
        context_stack.push_back(mod);
        generic_visit(mod);
        context_stack.pop_back();
        LOG_DEBUG("SemanticAnalyzer: exit Module");
    }
    else if (auto seq = dynamic_cast<Seq *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Seq");
        if (seq->isBlock())
        {
            LOG_DEBUG("SemanticAnalyzer: Seq is block, creating new scope");
            scope_stack.emplace_back();
        }
        for (const auto &stmt : seq->getBody())
            visit(stmt.get());
        if (seq->isBlock())
        {
            LOG_DEBUG("SemanticAnalyzer: Exiting Seq block, popping scope");
            scope_stack.pop_back();
        }
        LOG_DEBUG("SemanticAnalyzer: exit Seq");
    }
    else if (auto asn = dynamic_cast<Assign *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Assign");
        visit_Assign(asn);
    }
    else if (auto fn = dynamic_cast<FuncDef *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit FuncDef " << fn->getName());
        visit_FuncDef(fn);
    }
    else if (auto ret = dynamic_cast<Return *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Return");
        visit_Return(ret);
    }
    else if (auto br = dynamic_cast<Break *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Break");
        visit_Break(br);
    }
    else if (auto cont = dynamic_cast<Continue *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Continue");
        visit_Continue(cont);
    }
    else if (auto iff = dynamic_cast<If *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit If");
        visit_If(iff);
    }
    else if (auto wh = dynamic_cast<While *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit While");
        visit_While(wh);
    }
    else if (auto p = dynamic_cast<Par *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit Par");
        visit_Par(p);
    }
    else if (auto cch = dynamic_cast<CChannel *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit CChannel " << cch->getName());
        visit_CChannel(cch);
    }
    else if (auto sch = dynamic_cast<SChannel *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit SChannel " << sch->getName());
        visit_SChannel(sch);
    }
    else if (auto arrDecl = dynamic_cast<ArrayDecl *>(node))
    {
        LOG_DEBUG("SemanticAnalyzer: visit ArrayDecl " << arrDecl->getName());
        visit_ArrayDecl(arrDecl);
    }
    else
    {
        LOG_DEBUG("SemanticAnalyzer: generic_visit on " << typeid(*node).name());
        generic_visit(node);
    }
}

/**
 * @brief Realiza uma visita genérica aos atributos de um nó.
 *
 * Itera sobre os atributos do nó fornecido e invoca o método visit em cada um,
 * permitindo uma análise semântica recursiva dos componentes do nó.
 *
 * @param node O nó cujos atributos serão visitados.
 */
void SemanticAnalyzer::generic_visit(Node *node)
{
    LOG_DEBUG("SemanticAnalyzer: generic_visit attributes of " << typeid(*node).name());
    for (auto *child : node->getAttributes())
        visit(child);
}

/**
 * @brief Analisa uma declaração de array.
 *
 * Marca o array como tendo um tipo desconhecido no escopo atual e avalia as
 * dimensões do array.
 *
 * @param node O nó ArrayDecl a ser analisado.
 */
void SemanticAnalyzer::visit_ArrayDecl(ArrayDecl *node) {
    for (auto &dim : node->getDimensions()) {
        if (evaluate(dim.get()) != "num")
            throw SemanticError("Dimensão de array deve ser num");
    }
    std::string t = "unknown";
    for (size_t k = 0; k < node->getDimensions().size(); ++k) {
        t = "array<" + t + ">";
    }
    scope_stack.back()[node->getName()] = t;
}

/**
 * @brief Verifica a consistência de tipo em atribuições.
 *
 * Analisa atribuições, declarando novas variáveis ou inferindo tipos de arrays
 * quando necessário. Lança erros se tipos forem incompatíveis ou variáveis não
 * declaradas.
 *
 * @param node O nó Assign a ser analisado.
 */
void SemanticAnalyzer::visit_Assign(Assign *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Assign start");

    if (auto id = dynamic_cast<ID *>(node->getLeft()))
    {
        const std::string name = id->getToken().getValue();
        const std::string declared = id->getType();
        LOG_DEBUG("SemanticAnalyzer: Assign to ID " << name << " declared type " << declared);

        std::string rightType = evaluate(node->getRight());
        std::string leftType;

        if (!declared.empty() && declared != "ID")
        {
            if (declared == "array")
            {
                if (rightType.rfind("array<", 0) != 0)
                    throw SemanticError("Esperado tipo array, mas recebeu " + rightType);
                leftType = rightType;
            }
            else
            {
                leftType = normalize(declared);
                if (leftType != rightType)
                    throw SemanticError("Tipo " + leftType + " esperado, mas recebeu " + rightType);
            }

            if (scope_stack.back().count(name))
                throw SemanticError("Variável " + name + " já declarada");

            scope_stack.back()[name] = leftType;
            LOG_DEBUG("SemanticAnalyzer: Declared new var " << name << " of type " << leftType);
            LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
            return;
        }

        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it)
        {
            auto vit = it->find(name);
            if (vit != it->end())
            {
                leftType = vit->second;
                LOG_DEBUG("SemanticAnalyzer: Found existing var " << name << " of type " << leftType);

                if (leftType.find("array") != std::string::npos)
                {
                    vit->second = rightType;
                    LOG_DEBUG("SemanticAnalyzer: Inferred array type for " << name << ": " << rightType);
                    return;
                }

                if (leftType != rightType)
                    throw SemanticError("Tipo " + leftType + " esperado, mas recebeu " + rightType);

                LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
                return;
            }
        }

        throw SemanticError("Variável " + name + " não declarada");
    }
    else if (auto acc = dynamic_cast<Access *>(node->getLeft()))
    {
        LOG_DEBUG("SemanticAnalyzer: Assign to Access");
    
        std::string baseType = evaluate(acc->getBase());
        LOG_DEBUG("SemanticAnalyzer: base type " << baseType);
    
        std::string elemType;
        if (baseType == "string") {
            elemType = "string";
        }
        else if (baseType == "array") {
            baseType = "array<unknown>";
            elemType = "unknown";
        }
        else if (baseType.rfind("array<", 0) == 0) {
            elemType = baseType.substr(6, baseType.size() - 7);
        }
        else {
            throw SemanticError("Tipo " + baseType + " não indexável");
        }
        
        std::string valType = evaluate(node->getRight());
        LOG_DEBUG("SemanticAnalyzer: element type " << elemType
                  << " rhs type " << valType);
    

        if (elemType.find("unknown") != std::string::npos)
        {
            Node *root = acc;
            while (auto a = dynamic_cast<Access *>(root)) {
                root = a->getBase();
            }
            auto baseID = dynamic_cast<ID *>(root);
            if (!baseID)
                throw SemanticError("Não foi possível inferir tipo de array não‑ID");
    
            std::string arrName = baseID->getToken().getValue();
            std::string originalType;
            for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
                if (it->count(arrName)) {
                    originalType = it->at(arrName);
                    break;
                }
            }

            std::string innerValType = valType;
            if (valType.rfind("array<", 0) == 0)
                innerValType = valType.substr(6, valType.size() - 7);

            std::string newType = originalType;
            size_t pos;
            while ((pos = newType.find("unknown")) != std::string::npos) {
                newType.replace(pos, 7, innerValType);
            }

            for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
                if (it->count(arrName)) {
                    (*it)[arrName] = newType;
                    LOG_DEBUG("SemanticAnalyzer: Inferred array type for "
                              << arrName << ": " << newType);
                    return;
                }
            }
        }
    
        if (valType != elemType)
            throw SemanticError("Tipo de índice espera " +
                                elemType + ", recebeu " + valType);
    
        LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
        return;
    }
    
    throw SemanticError("Lado esquerdo inválido em atribuição");
}


/**
 * @brief Analisa uma definição de função.
 *
 * Verifica se a função está em um escopo válido e registra seus parâmetros no
 * novo escopo criado.
 *
 * @param node O nó FuncDef a ser analisado.
 */
void SemanticAnalyzer::visit_FuncDef(FuncDef *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_FuncDef " << node->getName());

    // 1) Impede declaração de função em escopos locais proibidos
    for (auto ctx : context_stack) {
        if (dynamic_cast<If *>(ctx) ||
            dynamic_cast<While *>(ctx) ||
            dynamic_cast<Par *>(ctx))
        {
            throw SemanticError("Não pode declarar função em escopo local");
        }
    }

    // 2) Registra a função na tabela de símbolos
    std::string fname = node->getName();
    if (function_table.count(fname)) {
        throw SemanticError("Função " + fname + " já declarada");
    }
    function_table[fname] = node;

    // 3) Cria um novo escopo para parâmetros e corpo
    scope_stack.emplace_back();
    LOG_DEBUG("SemanticAnalyzer: New function scope for " << fname);

    // 4) Declara parâmetros no escopo
    for (auto &param : node->getParams()) {
        auto normalized_type = normalize(param.second.first);
        if (param.second.first.find("array") != std::string::npos && normalized_type == "unknown") {
            throw SemanticError("Erro semântico: Tipo " + param.second.first + " esperado, mas recebeu array<unknown>");
        }
        scope_stack.back()[param.first] = normalized_type;
        LOG_DEBUG("SemanticAnalyzer: Param " << param.first << " of type " << param.second.first);
    }

    // 5) Empilha contexto de função para habilitar visit_Return
    context_stack.push_back(node);

    // 6) Analisa todo o corpo da função (inclui Returns)
    visit_block(node->getBody());

    // 7) Desempilha contexto e escopo
    context_stack.pop_back();
    scope_stack.pop_back();

    LOG_DEBUG("SemanticAnalyzer: exit FuncDef " << fname);
}

/**
 * @brief Analisa uma instrução de retorno.
 *
 * Verifica se o retorno está dentro de uma função e se o tipo retornado é compatível.
 *
 * @param node O nó Return a ser analisado.
 */
void SemanticAnalyzer::visit_Return(Return *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Return");

    FuncDef *f = nullptr;
    for (auto ctx : context_stack)
        if ((f = dynamic_cast<FuncDef *>(ctx)))
            break;
    if (!f)
        throw SemanticError("return fora de função");

    std::string retType = evaluate(node->getExpr());
    LOG_DEBUG("SemanticAnalyzer: return expr type " << retType);

    std::string declaredRaw = f->getReturnType();
    LOG_DEBUG("SemanticAnalyzer: declared return type " << declaredRaw);

    if (declaredRaw == "array")
    {
        if (retType.rfind("array<", 0) != 0)
            throw SemanticError(
                "Retorno em " + f->getName() +
                " deve ser um array, mas retornou " + retType
            );
    }
    
    else
    {
        std::string expectedType = normalize(declaredRaw);
        LOG_DEBUG("SemanticAnalyzer: expected normalized " << expectedType);
        if (retType != expectedType)
            throw SemanticError(
                "Retorno em " + f->getName() +
                " deve ser " + expectedType +
                ", mas retornou " + retType
            );
    }

    inferredReturnTypes[f->getName()] = retType;

    LOG_DEBUG("SemanticAnalyzer: visit_Return end");
}


/**
 * @brief Analisa uma instrução if.
 *
 * Verifica se a condição é booleana e analisa os blocos de código.
 *
 * @param node O nó If a ser analisado.
 */
void SemanticAnalyzer::visit_If(If *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_If");
    std::string cond = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: if condition type " << cond);
    if (cond != "bool")
        throw SemanticError("Esperado bool em if, mas recebeu " + cond);
    scope_stack.emplace_back();
    visit_block(node->getBody());
    scope_stack.pop_back();
    if (node->getElseStmt())
    {
        scope_stack.emplace_back();
        visit_block(*node->getElseStmt());
        scope_stack.pop_back();
    }
    LOG_DEBUG("SemanticAnalyzer: visit_If end");
}

/**
 * @brief Analisa uma instrução while.
 *
 * Verifica se a condição é booleana e analisa o bloco de código.
 *
 * @param node O nó While a ser analisado.
 */
void SemanticAnalyzer::visit_While(While *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_While");
    context_stack.push_back(node);
    std::string cond = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: while condition type " << cond);
    if (cond != "bool")
        throw SemanticError("Esperado bool em while, mas recebeu " + cond);
    scope_stack.emplace_back();
    visit_block(node->getBody());
    scope_stack.pop_back();
    context_stack.pop_back();
    LOG_DEBUG("SemanticAnalyzer: visit_While end");
}

/**
 * @brief Analisa uma instrução break.
 *
 * Verifica se o break está dentro de um loop.
 *
 * @param node O nó Break a ser analisado.
 */
void SemanticAnalyzer::visit_Break(Break *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Break");
    bool inLoop = false;
    for (auto ctx : context_stack)
        if (dynamic_cast<While *>(ctx))
        {
            inLoop = true;
            break;
        }
    if (!inLoop)
        throw SemanticError("break fora de loop");
    LOG_DEBUG("SemanticAnalyzer: visit_Break end");
}

/**
 * @brief Analisa uma instrução continue.
 *
 * Verifica se o continue está dentro de um loop.
 *
 * @param node O nó Continue a ser analisado.
 */
void SemanticAnalyzer::visit_Continue(Continue *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Continue");
    bool inLoop = false;
    for (auto ctx : context_stack)
        if (dynamic_cast<While *>(ctx))
        {
            inLoop = true;
            break;
        }
    if (!inLoop)
        throw SemanticError("continue fora de loop");
    LOG_DEBUG("SemanticAnalyzer: visit_Continue end");
}

/**
 * @brief Analisa uma execução paralela.
 *
 * Verifica se todos os statements são chamadas de função válidas.
 *
 * @param node O nó Par a ser analisado.
 */
void SemanticAnalyzer::visit_Par(Par *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_Par");
    for (auto &st : node->getBody())
    {
        if (!dynamic_cast<Call *>(st.get()))
            throw SemanticError("Apenas chamadas válidas em par");
    }
    LOG_DEBUG("SemanticAnalyzer: visit_Par end");
}

/**
 * @brief Analisa uma declaração de canal de comunicação.
 *
 * Verifica se a variável não existe no escopo atual, e registra o nome com tipo
 * CChannel. Além disso, verifica se o localhost e port são string e num,
 * respectivamente, e se o valor numérico de port está no intervalo [0,65535].
 *
 * @param node O nó CChannel a ser analisado.
 */
void SemanticAnalyzer::visit_CChannel(CChannel *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_CChannel " << node->getName());
    
    auto &currentScope = scope_stack.back();
    std::string name = node->getName();
    if (currentScope.find(name) != currentScope.end()) {
        throw SemanticError("Identificador duplicado: " + name);
    }
    currentScope[name] = "CChannel";

    if (evaluate(node->getLocalhostNode()) != "string")
        throw SemanticError("localhost deve ser string em CChannel");
    if (evaluate(node->getPortNode()) != "num")
        throw SemanticError("port deve ser num em CChannel");

    if (auto lit = dynamic_cast<Constant*>(node->getLocalhostNode())) {
        std::string hostVal = lit->getToken().getValue();
        if (hostVal.empty()) {
            throw SemanticError("localhost não pode ser string vazia em CChannel");
        }
    }

    if (auto lit = dynamic_cast<Constant*>(node->getPortNode())) {
        double portVal = std::stod(lit->getToken().getValue());
        if (portVal < 0 || portVal > 65535) {
            throw SemanticError("port fora do intervalo válido [0,65535] em CChannel");
        }
    }

    LOG_DEBUG("SemanticAnalyzer: visit_CChannel end");
}


/**
 * @brief Analisa uma declaração de canal de serviço.
 *
 * Verifica a existência da função e os tipos dos parâmetros.
 *
 * @param node O nó SChannel a ser analisado.
 */
void SemanticAnalyzer::visit_SChannel(SChannel *node)
{
    LOG_DEBUG("SemanticAnalyzer: visit_SChannel " << node->getName());
    auto it = function_table.find(node->getFuncName());
    if (it == function_table.end())
        throw SemanticError("Função não declarada em SChannel");
    if (evaluate(node->getDescription()) != "string")
        throw SemanticError("description deve ser string");
    if (evaluate(node->getLocalhostNode()) != "string")
        throw SemanticError("localhost deve ser string");
    if (evaluate(node->getPortNode()) != "num")
        throw SemanticError("port deve ser num");
    LOG_DEBUG("SemanticAnalyzer: visit_SChannel end");
}

/**
 * @brief Visita um bloco de instruções.
 *
 * Itera sobre uma sequência de nós e chama visit() em cada um.
 *
 * @param block O corpo do bloco a ser visitado.
 */
void SemanticAnalyzer::visit_block(const Body &block)
{
    LOG_DEBUG("SemanticAnalyzer: visit_block start");
    for (auto &st : block)
        visit(st.get());
    LOG_DEBUG("SemanticAnalyzer: visit_block end");
}

/**
 * @brief Analisa uma constante.
 *
 * @param node O nó Constant a ser analisado.
 * @return O tipo da constante.
 */
std::optional<std::string> SemanticAnalyzer::visit_Constant(const Constant *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Constant value " << node->getType());
    return normalize(node->getType());
}

/**
 * @brief Analisa um identificador.
 *
 * @param node O nó ID a ser analisado.
 * @return O tipo do identificador.
 */
std::optional<std::string> SemanticAnalyzer::visit_ID(const ID *node) const
{
    std::string name = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_ID " << name);
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it)
    {
        if (it->count(name))
        {
            LOG_DEBUG("SemanticAnalyzer: ID type " << it->at(name));
            return it->at(name);
        }
    }
    throw SemanticError("ID não declarado: " + name);
}

/**
 * @brief Analisa um acesso a um elemento.
 *
 * @param node O nó Access a ser analisado.
 * @return O tipo do elemento acessado.
 */
std::optional<std::string> SemanticAnalyzer::visit_Access(const Access *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Access");

    std::string base = evaluate(node->getBase());
    LOG_DEBUG("SemanticAnalyzer: base type '" << base << "'");

    if (base == "string")
        return "string";

    if (base == "array") {
        LOG_DEBUG("SemanticAnalyzer: treating raw 'array' as 'array<num>'");
        return "num"; // Assume default element type as 'num'
    }

    if (base.rfind("array<", 0) == 0) {
        return base.substr(6, base.size() - 7); // Extrai tipo interno
    }

    throw SemanticError("Tipo não indexável: " + base);
}


/**
 * @brief Analisa uma operação lógica.
 *
 * @param node O nó Logical a ser analisado.
 * @return O tipo do resultado da operação lógica.
 */
std::optional<std::string> SemanticAnalyzer::visit_Logical(const Logical *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Logical " << node->getToken().getValue());
    if (evaluate(node->getLeft()) != "bool" || evaluate(node->getRight()) != "bool")
        throw SemanticError("Operandos lógicos devem ser bool");
    return "bool";
}

/**
 * @brief Analisa uma operação relacional.
 *
 * @param node O nó Relational a ser analisado.
 * @return O tipo do resultado da operação relacional.
 */
std::optional<std::string> SemanticAnalyzer::visit_Relational(const Relational *node) const
{
    auto op = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_Relational op " << op);
    auto lt = evaluate(node->getLeft()), rt = evaluate(node->getRight());
    if ((op == "==" || op == "!=") && lt != rt)
        throw SemanticError("Comparação exige tipos iguais");
    if (op != "==" && op != "!=" && (lt != "num" || rt != "num"))
        throw SemanticError("Operadores relacionais numéricos exigem num");
    return "bool";
}

/**
 * @brief Analisa uma operação aritmética.
 *
 * @param node O nó Arithmetic a ser analisado.
 * @return O tipo do resultado da operação aritmética.
 */
std::optional<std::string> SemanticAnalyzer::visit_Arithmetic(const Arithmetic *node) const
{
    auto op = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_Arithmetic op " << op);
    std::string lt = evaluate(node->getLeft());
    std::string rt = evaluate(node->getRight());
    LOG_DEBUG("SemanticAnalyzer: operand types " << lt << " and " << rt);

    auto normalizeUnknown = [&](const std::string &t)
    {
        return t == "unknown" ? std::string("num") : t;
    };

    if (op == "+")
    {
        std::string l0 = normalizeUnknown(lt);
        std::string r0 = normalizeUnknown(rt);
        if (l0 != r0)
        {
            throw SemanticError("(Erro de Tipo) Operação '+' exige operandos do mesmo tipo, mas encontrou " + l0 + " e " + r0);
        }
        LOG_DEBUG("SemanticAnalyzer: '+' result type " << l0);
        return l0;
    }
    std::string l0 = normalizeUnknown(lt);
    std::string r0 = normalizeUnknown(rt);
    if (l0 != "num" || r0 != "num")
    {
        throw SemanticError("(Erro de Tipo) Operadores aritméticos exigem num, mas encontrou " + lt + " e " + rt);
    }
    LOG_DEBUG("SemanticAnalyzer: arithmetic result type num");
    return "num";
}

/**
 * @brief Analisa uma operação unária.
 *
 * @param node O nó Unary a ser analisado.
 * @return O tipo do resultado da operação unária.
 */
std::optional<std::string> SemanticAnalyzer::visit_Unary(const Unary *node) const
{
    auto t = node->getToken().getTag();
    LOG_DEBUG("SemanticAnalyzer: visit_Unary op " << t);
    auto et = evaluate(node->getExpr());
    if (t == "-" && et != "num")
        throw SemanticError("Unário - exige num");
    if (t == "!" && et != "bool")
        throw SemanticError("Unário ! exige bool");
    return et;
}

/**
 * @brief Analisa uma chamada de função.
 *
 * @param node O nó Call a ser analisado.
 * @return O tipo do valor retornado pela função.
 */
std::optional<std::string> SemanticAnalyzer::visit_Call(const Call *node) const
{
    std::string fname = node->getOper().empty() ? node->getToken().getValue() : node->getOper();
    LOG_DEBUG("SemanticAnalyzer: visit_Call " << fname);

    if (inferredReturnTypes.count(fname)) {
        LOG_DEBUG("SemanticAnalyzer: Returning inferred type " << inferredReturnTypes.at(fname));
        return inferredReturnTypes.at(fname);
    }

    if (builtin_return.count(fname))
        return builtin_return.at(fname);

    // Checa funções definidas pelo usuário
    if (!function_table.count(fname))
        throw SemanticError("Função não declarada: " + fname);
    auto f = function_table.at(fname);

    auto &params = f->getParams();
    // Verifica número de argumentos
    if (node->getArgs().size() > params.size())
        throw SemanticError("Número excessivo de args em " + fname);

    size_t minParams = 0;
    for (auto &p : params)
        if (!p.second.second)
            ++minParams;
    if (node->getArgs().size() < minParams)
        throw SemanticError("Número insuficiente de args em " + fname);

    // Checagem de tipo para cada argumento
    for (size_t i = 0; i < node->getArgs().size(); ++i) {
        std::string declaredRaw = params[i].second.first; // tipo cru da assinatura
        std::string actual     = evaluate(node->getArgs()[i].get());

        if (declaredRaw == "array") {
            // aceita qualquer array<...>
            if (actual.rfind("array<", 0) != 0)
                throw SemanticError(
                    "Argumento " + std::to_string(i + 1) + " de " + fname +
                    " deve ser um array, mas recebeu " + actual
                );
        } else {
            // caso comum (num, bool, string, ou array<...> explícito)
            std::string expected = normalize(declaredRaw);
            if (actual != expected)
                throw SemanticError(
                    "Argumento " + std::to_string(i + 1) + " de " + fname +
                    " deve ser " + expected + ", mas recebeu " + actual
                );
        }
    }

    // Se já passamos na checagem, retornamos o tipo inferido ou esperado
    // Para funções usuário: preferimos tipo inferido (se existir)
    if (inferredReturnTypes.count(fname))
        return inferredReturnTypes.at(fname);

    // Caso contrário, retorna o tipo declarado na assinatura, normalizado
    return normalize(f->getReturnType());
}

/**
 * @brief Analisa um array.
 *
 * @param node O nó Array a ser analisado.
 * @return O tipo do array.
 */
std::optional<std::string> SemanticAnalyzer::visit_Array(const Array *node) const
{
    LOG_DEBUG("SemanticAnalyzer: visit_Array");
    std::string elemType;
    for (auto &el : node->getElements())
    {
        auto t = evaluate(el.get());
        LOG_DEBUG("SemanticAnalyzer: element type " << t);
        if (elemType.empty())
            elemType = t;
        else if (t != elemType)
            throw SemanticError("Elementos de array precisam ter mesmo tipo");
    }
    return "array<" + elemType + ">";
}