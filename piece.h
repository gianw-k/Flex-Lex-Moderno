#pragma once
#include <string>
#include <vector>

// Una alternativa dentro de un elemento. Varias alternativas unidas
// representan un OR: ej. mayusculas OR minusculas.
struct Alternative {
    enum Kind { UPPERCASE, LOWERCASE, DIGIT, SYMBOL };

    Kind kind = UPPERCASE;
    std::string symbolLiteral; // solo se usa si kind == SYMBOL, ej. "+", "=="
    std::string symbolName;    // nombre amigable, ej. "suma" (solo para SYMBOL)

    std::string describe() const;
};

// Un elemento es una o más alternativas unidas por OR.
struct Element {
    std::vector<Alternative> alternatives;

    std::string describe() const;
};

// Una pieza del token: un elemento simple, o un elemento repetido
// (0 o más / 1 o más). No se permite anidar una repetición dentro de otra:
// element aquí nunca contiene a su vez una Piece.
struct Piece {
    enum Kind { SINGLE, REPETITION };

    Kind kind = SINGLE;
    Element element;
    bool zeroOrMore = true; // solo aplica si kind == REPETITION

    std::string describe() const;
};

// Definición completa de un tipo de token: o bien una palabra clave literal,
// o bien una secuencia (concatenación) de piezas.
struct TokenDef {
    std::string name;

    bool isKeyword = false;
    std::string keywordLiteral; // solo si isKeyword

    std::vector<Piece> pieces;  // solo si !isKeyword, se concatenan en orden

    std::string describe() const; // representación tipo regex, para vista previa
};
