#pragma once
#include <array>
#include <string_view>

/**
 * @brief Especificação de uma função builtin: nome e tipo de retorno normalizado
 * ("string", "num", "bool", "void").
 */
struct BuiltinSpec
{
    std::string_view name;
    std::string_view return_type;
};

/**
 * @brief Fonte única de builtins reconhecidos pela linguagem (T22).
 *
 * Consumida por semântica (`semantic_types.cpp`), parser (`token.cpp`) e
 * verificada contra o dispatch real do interpretador (`interpreter_func.cpp`).
 * Adicionar/remover um builtin: editar somente esta tabela.
 */
inline constexpr std::array<BuiltinSpec, 12> BUILTIN_TABLE = {{
    {"print", "string"},
    {"len", "num"},
    {"to_num", "num"},
    {"to_string", "string"},
    {"isnum", "bool"},
    {"isalpha", "bool"},
    {"exp", "num"},
    {"randf", "num"},
    {"randi", "num"},
    {"input", "string"},
    {"send", "string"},
    {"close", "void"},
}};
