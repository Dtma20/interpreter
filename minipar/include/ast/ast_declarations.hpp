#pragma once

#include <memory>
#include <string>
#include <vector>
#include "ast_base.hpp"

/**
 * @brief Representa a declaração de um array com dimensões.
 */
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
