#pragma once
#include <string>
#include <utility>

class Token {
public:
    enum class Kind { MATCH, ERR, END };

    Kind kind;
    std::string lexeme;    // texto reconocido
    std::string tokenName; // nombre del TokenDef que hizo match (ej. "ID", "SUMA", "IF")

    explicit Token(Kind k, std::string lex = "", std::string name = "")
        : kind(k), lexeme(std::move(lex)), tokenName(std::move(name)) {}
};
