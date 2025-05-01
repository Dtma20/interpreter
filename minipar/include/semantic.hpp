#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "../include/ast.hpp"
#include "../include/error.hpp"

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    void visit(Node* node);
    std::string evaluate(Node* node) const;
private:
    // stacks to track variable/function scopes and AST context
    std::vector<std::unordered_map<std::string, std::string>> scope_stack;
    std::vector<Node*> context_stack;
    std::unordered_map<std::string, FuncDef*> function_table;
    std::vector<std::string> default_funcs;

    // visit helpers
    void generic_visit(Node* node);
    void visit_Assign(Assign* node);
    void visit_Return(Return* node);
    void visit_Break(Break* node);
    void visit_Continue(Continue* node);
    void visit_FuncDef(FuncDef* node);
    void visit_If(If* node);
    void visit_While(While* node);
    void visit_Par(Par* node);
    void visit_CChannel(CChannel* node);
    void visit_SChannel(SChannel* node);
    void visit_ArrayDecl(ArrayDecl* node);
    void visit_block(const Body& block);

    // expression type visitors
    std::optional<std::string> visit_Constant(const Constant* node) const;
    std::optional<std::string> visit_ID(const ID* node) const;
    std::optional<std::string> visit_Access(const Access* node) const;
    std::optional<std::string> visit_Logical(const Logical* node) const;
    std::optional<std::string> visit_Relational(const Relational* node) const;
    std::optional<std::string> visit_Arithmetic(const Arithmetic* node) const;
    std::optional<std::string> visit_Unary(const Unary* node) const;
    std::optional<std::string> visit_Call(const Call* node) const;
    std::optional<std::string> visit_Array(const Array* node) const;

    // utilities
    std::string normalize(const std::string& raw) const;
};