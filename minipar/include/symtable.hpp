#ifndef SYMTABLE_HPP
#define SYMTABLE_HPP

#pragma once
#include <unordered_map>
#include <string>
#include <any>
#include <memory>

/**
 * @brief Representa um símbolo na tabela de símbolos.
 *
 * Um símbolo contém informações sobre o nome da variável e seu tipo.
 */
struct Symbol
{
    std::string var;  ///< Nome da variável.
    std::string type; ///< Tipo da variável.

    /**
     * @brief Construtor da estrutura Symbol.
     * @param var Nome da variável.
     * @param type Tipo da variável.
     */
    Symbol(const std::string &var, const std::string &type)
        : var(var), type(type) {}

};

/**
 * @brief Representa uma tabela de símbolos para controle de variáveis e funções.
 *
 * Essa estrutura permite armazenar e recuperar símbolos em diferentes escopos.
 */
class SymTable
{
public:
    std::unordered_map<std::string, Symbol> table; ///< Mapeamento de identificadores para símbolos.
    std::shared_ptr<SymTable> prev;                ///< Ponteiro para a tabela do escopo anterior.

    /**
     * @brief Construtor da tabela de símbolos.
     * @param prev Ponteiro para a tabela do escopo anterior (padrão: nullptr).
     */
    SymTable(std::shared_ptr<SymTable> prev = nullptr) : prev(prev) {}

    /**
     * @brief Insere um novo símbolo na tabela.
     * @param key Nome do símbolo a ser inserido.
     * @param symbol Objeto Symbol contendo informações da variável.
     * @return true se a inserção foi bem-sucedida, false se a chave já existia.
     */
    bool insert(const std::string &key, const Symbol &symbol)
    {
        return table.emplace(key, symbol).second;
    }

    /**
     * @brief Busca um símbolo na tabela e nos escopos superiores.
     * @param key Nome do símbolo a ser buscado.
     * @return Ponteiro para o Symbol encontrado ou nullptr se não existir.
     */
    Symbol *find(const std::string &key)
    {
        std::shared_ptr<SymTable> current = std::shared_ptr<SymTable>(this);
        while (current != nullptr)
        {
            auto it = current->table.find(key);
            if (it != current->table.end())
            {
                return &it->second;
            }
            current = current->prev;
        }
        return nullptr;
    }
};

/**
 * @brief Representa uma tabela para armazenamento de valores de variáveis.
 *
 * Permite associar identificadores a valores e recuperar valores armazenados.
 */
class VarTable
{
public:
    std::unordered_map<std::string, std::any> table; ///< Mapeamento de identificadores para valores.
    std::shared_ptr<VarTable> prev;                  ///< Ponteiro para a tabela do escopo anterior.

    /**
     * @brief Construtor da tabela de variáveis.
     * @param prev Ponteiro para a tabela do escopo anterior (padrão: nullptr).
     */
    VarTable(std::shared_ptr<VarTable> prev = nullptr) : prev(prev) {}

    /**
     * @brief Busca uma variável na tabela e nos escopos superiores.
     * @param key Nome da variável a ser buscada.
     * @return Ponteiro para a tabela onde a variável foi encontrada ou nullptr se não existir.
     */
    std::shared_ptr<VarTable> find(const std::string &key)
    {
        std::shared_ptr<VarTable> current = std::shared_ptr<VarTable>(this);
        while (current != nullptr)
        {
            if (current->table.find(key) != current->table.end())
            {
                return current;
            }
            current = current->prev;
        }
        return nullptr;
    }
};

#endif // SYMTABLE_HPP
