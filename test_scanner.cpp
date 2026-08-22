#include <iostream>
#include "piece.h"
#include "scanner.h"

TokenDef makeId() {
    TokenDef def; def.name = "ID";
    Piece p1; p1.kind = Piece::SINGLE;
    Alternative a1; a1.kind = Alternative::UPPERCASE;
    Alternative a2; a2.kind = Alternative::LOWERCASE;
    p1.element.alternatives = {a1, a2};
    def.pieces.push_back(p1);

    Piece p2; p2.kind = Piece::REPETITION; p2.zeroOrMore = true;
    Alternative b1; b1.kind = Alternative::UPPERCASE;
    Alternative b2; b2.kind = Alternative::LOWERCASE;
    Alternative b3; b3.kind = Alternative::DIGIT;
    p2.element.alternatives = {b1, b2, b3};
    def.pieces.push_back(p2);
    return def;
}

TokenDef makeFloat() {
    TokenDef def; def.name = "FLOAT";
    Piece p1; p1.kind = Piece::REPETITION; p1.zeroOrMore = false;
    Alternative d1; d1.kind = Alternative::DIGIT;
    p1.element.alternatives = {d1};
    def.pieces.push_back(p1);

    Piece p2; p2.kind = Piece::SINGLE;
    Alternative punto; punto.kind = Alternative::SYMBOL; punto.symbolLiteral = ".";
    p2.element.alternatives = {punto};
    def.pieces.push_back(p2);

    Piece p3; p3.kind = Piece::REPETITION; p3.zeroOrMore = false;
    Alternative d2; d2.kind = Alternative::DIGIT;
    p3.element.alternatives = {d2};
    def.pieces.push_back(p3);
    return def;
}

TokenDef makeNum() {
    TokenDef def; def.name = "NUM";
    Piece p1; p1.kind = Piece::REPETITION; p1.zeroOrMore = false;
    Alternative d1; d1.kind = Alternative::DIGIT;
    p1.element.alternatives = {d1};
    def.pieces.push_back(p1);
    return def;
}

TokenDef makeSymbol(const std::string& name, const std::string& lit) {
    TokenDef def; def.name = name;
    Piece p; p.kind = Piece::SINGLE;
    Alternative a; a.kind = Alternative::SYMBOL; a.symbolLiteral = lit;
    p.element.alternatives = {a};
    def.pieces.push_back(p);
    return def;
}

TokenDef makeKeyword(const std::string& name, const std::string& lit) {
    TokenDef def; def.name = name; def.isKeyword = true; def.keywordLiteral = lit;
    return def;
}

int main() {
    std::vector<TokenDef> defs = {
        makeId(), makeFloat(), makeNum(),
        makeSymbol("SUMA", "+"), makeSymbol("ASIG", "="), makeSymbol("POW", "**"), makeSymbol("MUL", "*"),
        makeKeyword("IF", "if"), makeKeyword("WHILE", "while")
    };

    std::cout << "Patrones:\n";
    for (auto& d : defs) std::cout << "  " << d.name << " = " << d.describe() << "\n";

    std::string codigo = "x1 = 3.14 + y ** 2 if while2 z@";
    std::cout << "\nEscaneando: \"" << codigo << "\"\n\n";

    Scanner sc(codigo, defs);
    Token t = sc.nextToken();
    while (t.kind != Token::Kind::END) {
        if (t.kind == Token::Kind::ERR) {
            std::cout << "  ERROR   caracter='" << t.lexeme << "'\n";
        } else {
            std::cout << "  " << t.tokenName << "\tlexema='" << t.lexeme << "'\n";
        }
        t = sc.nextToken();
    }
    std::cout << "  END\n";
    return 0;
}
