// Prueba del generador sin abrir la GUI:
//   g++ -std=c++17 -o test test_generador.cpp generador.cpp piece.cpp && ./test

#include <iostream>
#include "generador.h"

static Alternative clase(Alternative::Kind k) { Alternative a; a.kind = k; return a; }

static Alternative simbolo(const std::string& lit) {
    Alternative a;
    a.kind = Alternative::SYMBOL;
    a.symbolLiteral = lit;
    return a;
}

static Piece pieza(std::vector<Alternative> alts, Piece::Kind k, bool cero = false) {
    Piece p;
    p.kind = k;
    p.element.alternatives = std::move(alts);
    p.zeroOrMore = cero;
    return p;
}

static TokenDef keyword(const std::string& nombre, const std::string& lit) {
    TokenDef d;
    d.name = nombre;
    d.isKeyword = true;
    d.keywordLiteral = lit;
    return d;
}

static TokenDef patron(const std::string& nombre, std::vector<Piece> piezas) {
    TokenDef d;
    d.name = nombre;
    d.pieces = std::move(piezas);
    return d;
}

int main() {
    const Alternative may = clase(Alternative::UPPERCASE);
    const Alternative min = clase(Alternative::LOWERCASE);
    const Alternative dig = clase(Alternative::DIGIT);

    std::vector<TokenDef> defs = {
        keyword("IF", "if"),
        keyword("WHILE", "while"),
        keyword("IGUAL", "=="),
        patron("ID",     { pieza({min, may}, Piece::SINGLE),
                           pieza({min, may, dig}, Piece::REPETITION, true) }),
        patron("FLOAT",  { pieza({dig}, Piece::REPETITION, false),
                           pieza({simbolo(".")}, Piece::SINGLE),
                           pieza({dig}, Piece::REPETITION, false) }),
        patron("NUM",    { pieza({dig}, Piece::REPETITION, false) }),
        patron("ASIGNA", { pieza({simbolo("=")}, Piece::SINGLE) }),
        patron("SUMA",   { pieza({simbolo("+")}, Piece::SINGLE) }),
    };

    Dfa dfa = construirDfa(defs);

    std::cout << "Reglas:\n";
    for (size_t i = 0; i < defs.size(); i++)
        std::cout << "  " << (i + 1) << ". " << defs[i].name
                  << "  =  " << defs[i].describe() << "\n";
    std::cout << "\nNFA " << dfa.estadosNfa << " estados -> DFA "
              << dfa.tabla.size() << " estados\n";
    for (const std::string& a : dfa.avisos)
        std::cout << "AVISO: la regla " << a << " reconoce la cadena vacia\n";

    const std::string prueba = "if iff x1 == 3.14 y = 20 + ?";
    std::cout << "\nEntrada: " << prueba << "\n";
    for (const Token& t : escanearConDfa(defs, dfa, prueba)) {
        if (t.kind == Token::Kind::END) break;
        std::cout << "  " << (t.kind == Token::Kind::ERR ? "ERROR" : t.tokenName)
                  << "\t" << t.lexeme << "\n";
    }

    std::cout << "\n--- lexer.cpp generado (primeras lineas) ---\n";
    std::string codigo = generarCpp(defs, dfa);
    std::cout << codigo.substr(0, codigo.find("#include")) << "\n";
    std::cout << "(total " << codigo.size() << " caracteres)\n";
    return 0;
}
