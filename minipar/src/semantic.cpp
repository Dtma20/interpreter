#include "../include/semantic.hpp"
#include "../include/debug.hpp"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>

static const std::unordered_map<std::string, std::string> builtin_return = {
    {"print", "string"}, {"input", "string"}, {"sleep", "num"},
    {"to_num", "num"}, {"to_string", "string"}, {"to_bool", "bool"},
    {"len", "num"}, {"isalpha", "bool"}, {"isnum", "bool"},
    {"exp", "num"}, {"randf", "num"}
};

SemanticAnalyzer::SemanticAnalyzer() {
    LOG_DEBUG("SemanticAnalyzer: Construtor chamado");
    scope_stack.emplace_back();
    default_funcs = {"print","input","sleep","to_num","to_string","to_bool",
                     "len","isalpha","isnum","exp","randf"};
    LOG_DEBUG("SemanticAnalyzer: default_funcs inicializadas");
}

std::string SemanticAnalyzer::normalize(const std::string& raw) const {
    LOG_DEBUG("SemanticAnalyzer: Normalizando tipo " << raw);
    if (raw == "NUM" || raw == "num") return "num";
    if (raw == "STRING" || raw == "string") return "string";
    if (raw == "BOOL" || raw == "bool") return "bool";
    return raw;
}

std::string SemanticAnalyzer::evaluate(Node* node) const {
    if (!node) {
        LOG_DEBUG("SemanticAnalyzer: evaluate recebido nó nulo");
        return "";
    }
    LOG_DEBUG("SemanticAnalyzer: Evaluating node type " << typeid(*node).name());
    std::string raw;
    if (auto c = dynamic_cast<Constant*>(node)) raw = visit_Constant(c).value_or("");
    else if (auto id = dynamic_cast<ID*>(node)) raw = visit_ID(id).value_or("");
    else if (auto a = dynamic_cast<Access*>(node)) raw = visit_Access(a).value_or("");
    else if (auto l = dynamic_cast<Logical*>(node)) raw = visit_Logical(l).value_or("");
    else if (auto r = dynamic_cast<Relational*>(node)) raw = visit_Relational(r).value_or("");
    else if (auto ar = dynamic_cast<Arithmetic*>(node)) raw = visit_Arithmetic(ar).value_or("");
    else if (auto u = dynamic_cast<Unary*>(node)) raw = visit_Unary(u).value_or("");
    else if (auto call = dynamic_cast<Call*>(node)) raw = visit_Call(call).value_or("");
    else if (auto arr = dynamic_cast<Array*>(node)) raw = visit_Array(arr).value_or("");
    std::string norm = normalize(raw);
    LOG_DEBUG("SemanticAnalyzer: Type normalized to " << norm);
    return norm;
}

void SemanticAnalyzer::visit(Node* node) {
    if (!node) return;
    LOG_DEBUG("SemanticAnalyzer: visit node type " << typeid(*node).name());

    if (auto mod = dynamic_cast<Module*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit Module");
        context_stack.push_back(mod);
        generic_visit(mod);
        context_stack.pop_back();
        LOG_DEBUG("SemanticAnalyzer: exit Module");
    }
    else if (auto seq = dynamic_cast<Seq*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit Seq");
        if (seq->isBlock()) {
            LOG_DEBUG("SemanticAnalyzer: Seq is block, creating new scope");
            scope_stack.emplace_back();
        }
        for (const auto &stmt : seq->getBody()) visit(stmt.get());
        if (seq->isBlock()) {
            LOG_DEBUG("SemanticAnalyzer: Exiting Seq block, popping scope");
            scope_stack.pop_back();
        }
        LOG_DEBUG("SemanticAnalyzer: exit Seq");
    }
    else if (auto asn = dynamic_cast<Assign*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit Assign"); visit_Assign(asn);
    }
    else if (auto fn = dynamic_cast<FuncDef*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit FuncDef " << fn->getName()); visit_FuncDef(fn);
    }
    else if (auto ret = dynamic_cast<Return*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit Return"); visit_Return(ret);
    }
    else if (auto br = dynamic_cast<Break*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit Break"); visit_Break(br);
    }
    else if (auto cont = dynamic_cast<Continue*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit Continue"); visit_Continue(cont);
    }
    else if (auto iff = dynamic_cast<If*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit If"); visit_If(iff);
    }
    else if (auto wh = dynamic_cast<While*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit While"); visit_While(wh);
    }
    else if (auto p = dynamic_cast<Par*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit Par"); visit_Par(p);
    }
    else if (auto cch = dynamic_cast<CChannel*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit CChannel " << cch->getName()); visit_CChannel(cch);
    }
    else if (auto sch = dynamic_cast<SChannel*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit SChannel " << sch->getName()); visit_SChannel(sch);
    }
    else if (auto arrDecl = dynamic_cast<ArrayDecl*>(node)) {
        LOG_DEBUG("SemanticAnalyzer: visit ArrayDecl " << arrDecl->getName()); visit_ArrayDecl(arrDecl);
    }
    else {
        LOG_DEBUG("SemanticAnalyzer: generic_visit on " << typeid(*node).name()); generic_visit(node);
    }
}

void SemanticAnalyzer::generic_visit(Node* node) {
    LOG_DEBUG("SemanticAnalyzer: generic_visit attributes of " << typeid(*node).name());
    for (auto* child : node->getAttributes()) visit(child);
}

void SemanticAnalyzer::visit_ArrayDecl(ArrayDecl* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_ArrayDecl " << node->getName());
    // Marca array sem tipo conhecido
    scope_stack.back()[node->getName()] = "array<unknown>";
    for (auto &dim : node->getDimensions()) evaluate(dim.get());
}

void SemanticAnalyzer::visit_Assign(Assign* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_Assign start");
    if (auto id = dynamic_cast<ID*>(node->getLeft())) {
        const std::string name = id->getToken().getValue();
        const std::string declared = id->getType();
        LOG_DEBUG("SemanticAnalyzer: Assign to ID " << name << " declared type " << declared);
        
        // Declaração explícita: x: type = ...
        if (!declared.empty() && declared != "ID") {
            std::string leftType = (declared == "array")
                ? evaluate(node->getRight())
                : normalize(declared);
            if (scope_stack.back().count(name))
                throw SemanticError("Variável " + name + " já declarada");
            scope_stack.back()[name] = leftType;
            LOG_DEBUG("SemanticAnalyzer: Declared new var " << name << " of type " << leftType);
            return;
        }
        
        // Atribuição: variável existente
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            auto vit = it->find(name);
            if (vit != it->end()) {
                std::string leftType = vit->second;
                LOG_DEBUG("SemanticAnalyzer: Found existing var " << name << " of type " << leftType);
                
                // Inferir array<unknown>
                if (leftType == "array<unknown>") {
                    std::string rightType = evaluate(node->getRight());
                    vit->second = rightType;
                    LOG_DEBUG("SemanticAnalyzer: Inferred array type for " << name << ": " << rightType);
                    return;
                }
                
                // Compatibilidade de tipo
                std::string rightType = evaluate(node->getRight());
                LOG_DEBUG("SemanticAnalyzer: rhs type " << rightType);
                if (leftType != rightType)
                    throw SemanticError("Tipo " + leftType + " esperado, mas recebeu " + rightType);
                LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
                return;
            }
        }
        throw SemanticError("Variável " + name + " não declarada");
    }
    
    // Acesso por índice
    else if (auto acc = dynamic_cast<Access*>(node->getLeft())) {
        LOG_DEBUG("SemanticAnalyzer: Assign to Access");
        std::string baseType = evaluate(acc->getBase());
        LOG_DEBUG("SemanticAnalyzer: base type " << baseType);
        std::string elemType;
        if (baseType == "string") elemType = "string";
        else if (baseType.rfind("array<", 0) == 0)
            elemType = baseType.substr(6, baseType.size() - 7);
        else
            throw SemanticError("Tipo " + baseType + " não indexável");
        std::string valType = evaluate(node->getRight());
        LOG_DEBUG("SemanticAnalyzer: element type " << elemType << " rhs type " << valType);
        if (elemType == "unknown") {
            // Inferir tipo de elementos
            std::string arrName = dynamic_cast<ID*>(acc->getBase())->getToken().getValue();
            std::string newType = "array<" + valType + ">";
            for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
                if (it->count(arrName)) {
                    (*it)[arrName] = newType;
                    LOG_DEBUG("SemanticAnalyzer: Inferred array type for " << arrName << ": " << newType);
                    return;
                }
            }
        }
        if (valType != elemType)
            throw SemanticError("Tipo de índice espera " + elemType + ", recebeu " + valType);
        LOG_DEBUG("SemanticAnalyzer: visit_Assign end");
        return;
    }
    else {
        throw SemanticError("Lado esquerdo inválido em atribuição");
    }
}

void SemanticAnalyzer::visit_FuncDef(FuncDef* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_FuncDef " << node->getName());
    for (auto ctx : context_stack) {
        if (dynamic_cast<If*>(ctx) || dynamic_cast<While*>(ctx) || dynamic_cast<Par*>(ctx))
            throw SemanticError("Não pode declarar função em escopo local");
    }
    std::string fname = node->getName();
    if (function_table.count(fname)) throw SemanticError("Função " + fname + " já declarada");
    function_table[fname] = node;
    scope_stack.emplace_back();
    LOG_DEBUG("SemanticAnalyzer: New function scope for " << fname);
    for (auto &param : node->getParams()) {
        scope_stack.back()[param.first] = normalize(param.second.first);
        LOG_DEBUG("SemanticAnalyzer: Param " << param.first << " of type " << param.second.first);
    }
    generic_visit(node);
    scope_stack.pop_back();
    LOG_DEBUG("SemanticAnalyzer: exit FuncDef " << fname);
}

void SemanticAnalyzer::visit_Return(Return* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_Return");
    FuncDef* f = nullptr;
    for (auto ctx : context_stack) if ((f = dynamic_cast<FuncDef*>(ctx))) break;
    if (!f) throw SemanticError("return fora de função");
    std::string retType = evaluate(node->getExpr());
    LOG_DEBUG("SemanticAnalyzer: return expr type " << retType);
    if (retType != normalize(f->getReturnType()))
        throw SemanticError("Retorno em " + f->getName() + " deve ser " + normalize(f->getReturnType()));
    LOG_DEBUG("SemanticAnalyzer: visit_Return end");
}

void SemanticAnalyzer::visit_If(If* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_If");
    std::string cond = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: if condition type " << cond);
    if (cond != "bool") throw SemanticError("Esperado bool em if, mas recebeu " + cond);
    scope_stack.emplace_back();
    visit_block(node->getBody());
    scope_stack.pop_back();
    if (node->getElseStmt()) {
        scope_stack.emplace_back();
        visit_block(*node->getElseStmt());
        scope_stack.pop_back();
    }
    LOG_DEBUG("SemanticAnalyzer: visit_If end");
}

void SemanticAnalyzer::visit_While(While* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_While");
    std::string cond = evaluate(node->getCondition());
    LOG_DEBUG("SemanticAnalyzer: while condition type " << cond);
    if (cond != "bool") throw SemanticError("Esperado bool em while, mas recebeu " + cond);
    scope_stack.emplace_back();
    visit_block(node->getBody());
    scope_stack.pop_back();
    LOG_DEBUG("SemanticAnalyzer: visit_While end");
}

void SemanticAnalyzer::visit_Break(Break*) {
    LOG_DEBUG("SemanticAnalyzer: visit_Break");
    bool inLoop = false;
    for (auto ctx : context_stack) if (dynamic_cast<While*>(ctx)) { inLoop = true; break; }
    if (!inLoop) throw SemanticError("break fora de loop");
    LOG_DEBUG("SemanticAnalyzer: visit_Break end");
}

void SemanticAnalyzer::visit_Continue(Continue*) {
    LOG_DEBUG("SemanticAnalyzer: visit_Continue");
    bool inLoop = false;
    for (auto ctx : context_stack) if (dynamic_cast<While*>(ctx)) { inLoop = true; break; }
    if (!inLoop) throw SemanticError("continue fora de loop");
    LOG_DEBUG("SemanticAnalyzer: visit_Continue end");
}

void SemanticAnalyzer::visit_Par(Par* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_Par");
    for (auto &st : node->getBody()) {
        if (!dynamic_cast<Call*>(st.get()))
            throw SemanticError("Apenas chamadas válidas em par");
    }
    LOG_DEBUG("SemanticAnalyzer: visit_Par end");
}

void SemanticAnalyzer::visit_CChannel(CChannel* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_CChannel " << node->getName());
    if (evaluate(node->getLocalhostNode()) != "string")
        throw SemanticError("localhost deve ser string em CChannel");
    if (evaluate(node->getPortNode()) != "num")
        throw SemanticError("port deve ser num em CChannel");
    LOG_DEBUG("SemanticAnalyzer: visit_CChannel end");
}

void SemanticAnalyzer::visit_SChannel(SChannel* node) {
    LOG_DEBUG("SemanticAnalyzer: visit_SChannel " << node->getName());
    auto it = function_table.find(node->getFuncName());
    if (it == function_table.end()) throw SemanticError("Função não declarada em SChannel");
    if (evaluate(node->getDescription()) != "string")
        throw SemanticError("description deve ser string");
    if (evaluate(node->getLocalhostNode()) != "string")
        throw SemanticError("localhost deve ser string");
    if (evaluate(node->getPortNode()) != "num")
        throw SemanticError("port deve ser num");
    LOG_DEBUG("SemanticAnalyzer: visit_SChannel end");
}

void SemanticAnalyzer::visit_block(const Body& block) {
    LOG_DEBUG("SemanticAnalyzer: visit_block start");
    for (auto &st : block) visit(st.get());
    LOG_DEBUG("SemanticAnalyzer: visit_block end");
}

std::optional<std::string> SemanticAnalyzer::visit_Constant(const Constant* node) const {
    LOG_DEBUG("SemanticAnalyzer: visit_Constant value " << node->getType());
    return normalize(node->getType());
}

std::optional<std::string> SemanticAnalyzer::visit_ID(const ID* node) const {
    std::string name = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_ID " << name);
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
        if (it->count(name)) {
            LOG_DEBUG("SemanticAnalyzer: ID type " << it->at(name));
            return it->at(name);
        }
    }
    throw SemanticError("ID não declarado: " + name);
}

std::optional<std::string> SemanticAnalyzer::visit_Access(const Access* node) const {
    LOG_DEBUG("SemanticAnalyzer: visit_Access");
    std::string base = evaluate(node->getBase());
    LOG_DEBUG("SemanticAnalyzer: base type " << base);
    if (base == "string") return "string";
    if (base.rfind("array<",0)==0) return base.substr(6, base.size()-7);
    throw SemanticError("Tipo não indexável: " + base);
}

std::optional<std::string> SemanticAnalyzer::visit_Logical(const Logical* node) const {
    LOG_DEBUG("SemanticAnalyzer: visit_Logical " << node->getToken().getValue());
    if (evaluate(node->getLeft()) != "bool" || evaluate(node->getRight()) != "bool")
        throw SemanticError("Operandos lógicos devem ser bool");
    return "bool";
}

std::optional<std::string> SemanticAnalyzer::visit_Relational(const Relational* node) const {
    auto op = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_Relational op " << op);
    auto lt = evaluate(node->getLeft()), rt = evaluate(node->getRight());
    if ((op=="=="||op=="!=") && lt!=rt)
        throw SemanticError("Comparação exige tipos iguais");
    if (op!="=="&&op!="!=" && (lt!="num"|| rt!="num"))
        throw SemanticError("Operadores relacionais numéricos exigem num");
    return "bool";
}

std::optional<std::string> SemanticAnalyzer::visit_Arithmetic(const Arithmetic* node) const {
    auto op = node->getToken().getValue();
    LOG_DEBUG("SemanticAnalyzer: visit_Arithmetic op " << op);
    std::string lt = evaluate(node->getLeft());
    std::string rt = evaluate(node->getRight());
    LOG_DEBUG("SemanticAnalyzer: operand types " << lt << " and " << rt);

    auto normalizeUnknown = [&](const std::string &t) {
        return t == "unknown" ? std::string("num") : t;
    };

    if (op == "+") {
        std::string l0 = normalizeUnknown(lt);
        std::string r0 = normalizeUnknown(rt);
        if (l0 != r0) {
            throw SemanticError("(Erro de Tipo) Operação '+' exige operandos do mesmo tipo, mas encontrou " + l0 + " e " + r0);
        }
        LOG_DEBUG("SemanticAnalyzer: '+' result type " << l0);
        return l0;
    }
    // -, *, / e outros exigem num
    std::string l0 = normalizeUnknown(lt);
    std::string r0 = normalizeUnknown(rt);
    if (l0 != "num" || r0 != "num") {
        throw SemanticError("(Erro de Tipo) Operadores aritméticos exigem num, mas encontrou " + lt + " e " + rt);
    }
    LOG_DEBUG("SemanticAnalyzer: arithmetic result type num");
    return "num";
}


std::optional<std::string> SemanticAnalyzer::visit_Unary(const Unary* node) const {
    auto t = node->getToken().getTag();
    LOG_DEBUG("SemanticAnalyzer: visit_Unary op " << t);
    auto et = evaluate(node->getExpr());
    if (t=="-" && et!="num") throw SemanticError("Unário - exige num");
    if (t=="!" && et!="bool") throw SemanticError("Unário ! exige bool");
    return et;
}

std::optional<std::string> SemanticAnalyzer::visit_Call(const Call* node) const {
    std::string fname = node->getOper().empty() ? node->getToken().getValue() : node->getOper();
    LOG_DEBUG("SemanticAnalyzer: visit_Call " << fname);
    for (auto &arg : node->getArgs()) {
        auto at = evaluate(arg.get());
        LOG_DEBUG("SemanticAnalyzer: arg type " << at);
    }
    if (builtin_return.count(fname)) return builtin_return.at(fname);
    if (!function_table.count(fname)) throw SemanticError("Função não declarada: " + fname);
    auto f = function_table.at(fname);
    size_t minParams = 0;
    for (auto &p: f->getParams()) if (!p.second.second) ++minParams;
    if (node->getArgs().size() < minParams)
        throw SemanticError("Número insuficiente de args em " + fname);
    return normalize(f->getReturnType());
}

std::optional<std::string> SemanticAnalyzer::visit_Array(const Array* node) const {
    LOG_DEBUG("SemanticAnalyzer: visit_Array");
    std::string elemType;
    for (auto &el : node->getElements()) {
        auto t = evaluate(el.get());
        LOG_DEBUG("SemanticAnalyzer: element type " << t);
        if (elemType.empty()) elemType = t;
        else if (t != elemType) throw SemanticError("Elementos de array precisam ter mesmo tipo");
    }
    return "array<" + elemType + ">";
}
