#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include "token.hpp"

class ILexer
{
public:
    virtual ~ILexer() = default;
    virtual std::vector<std::pair<Token, int>> scan() = 0;
};

class Lexer : public ILexer
{
public:
    Lexer(const std::string &data);
    std::vector<std::pair<Token, int>> scan() override;

private:
    std::string data;
    int line;
    std::unordered_map<std::string, std::string> token_table;
    void initializeTokenTable();
};

#endif // LEXER_HPP