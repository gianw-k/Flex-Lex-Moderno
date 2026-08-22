#pragma once
#include <string>
#include <vector>
#include "piece.h"
#include "token.h"

// Scanner generico: en vez de tener reglas fijas en el codigo (como sqrt,
// max, min...), recibe la lista de TokenDef que el usuario armo en el GUI
// y reconoce tokens en base a eso, usando "maximal munch" (coincidencia
// mas larga) igual que FLEX.
class Scanner {
public:
    Scanner(const std::string& input, const std::vector<TokenDef>& defs);

    Token nextToken();

private:
    std::string input;
    const std::vector<TokenDef>& defs;
    size_t current;

    // Devuelve cuantos caracteres coincide el elemento en esa posicion, o -1
    int matchElement(const Element& elem, size_t pos) const;

    // Devuelve cuantos caracteres coincide la pieza (single o repeticion), o -1
    int matchPiece(const Piece& piece, size_t pos) const;

    // Devuelve cuantos caracteres coincide toda la definicion, o -1
    int matchTokenDef(const TokenDef& def, size_t pos) const;
};
