#include "piece.h"

std::string Alternative::describe() const {
    switch (kind) {
        case UPPERCASE: return "A-Z";
        case LOWERCASE: return "a-z";
        case DIGIT:     return "0-9";
        case SYMBOL:    return "'" + symbolLiteral + "'";
    }
    return "?";
}

std::string Element::describe() const {
    if (alternatives.empty()) return "?";
    if (alternatives.size() == 1) return alternatives[0].describe();
    std::string s = "(";
    for (size_t i = 0; i < alternatives.size(); i++) {
        if (i > 0) s += "|";
        s += alternatives[i].describe();
    }
    s += ")";
    return s;
}

std::string Piece::describe() const {
    if (kind == SINGLE) return element.describe();
    std::string s = element.describe();
    if (s.empty() || s[0] != '(') s = "(" + s + ")";
    s += zeroOrMore ? "*" : "+";
    return s;
}

std::string TokenDef::describe() const {
    if (isKeyword) return "\"" + keywordLiteral + "\"";
    std::string s;
    for (const auto& p : pieces) s += p.describe();
    return s;
}
