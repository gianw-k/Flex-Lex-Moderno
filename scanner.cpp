#include "scanner.h"
#include <cctype>

static bool isWhiteSpace(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

Scanner::Scanner(const std::string& in, const std::vector<TokenDef>& d)
    : input(in), defs(d), current(0) {}

int Scanner::matchElement(const Element& elem, size_t pos) const {
    if (pos >= input.size()) return -1;
    unsigned char c = static_cast<unsigned char>(input[pos]);
    for (const auto& alt : elem.alternatives) {
        switch (alt.kind) {
            case Alternative::UPPERCASE:
                if (std::isupper(c)) return 1;
                break;
            case Alternative::LOWERCASE:
                if (std::islower(c)) return 1;
                break;
            case Alternative::DIGIT:
                if (std::isdigit(c)) return 1;
                break;
            case Alternative::SYMBOL: {
                size_t len = alt.symbolLiteral.size();
                if (len > 0 && input.compare(pos, len, alt.symbolLiteral) == 0)
                    return static_cast<int>(len);
                break;
            }
        }
    }
    return -1;
}

int Scanner::matchPiece(const Piece& piece, size_t pos) const {
    if (piece.kind == Piece::SINGLE) {
        return matchElement(piece.element, pos);
    }
    // REPETITION: consume la mayor cantidad posible (maximal munch local)
    size_t cur = pos;
    int count = 0;
    while (true) {
        int len = matchElement(piece.element, cur);
        if (len < 0) break;
        cur += static_cast<size_t>(len);
        count++;
    }
    if (!piece.zeroOrMore && count == 0) return -1;
    return static_cast<int>(cur - pos);
}

int Scanner::matchTokenDef(const TokenDef& def, size_t pos) const {
    if (def.isKeyword) {
        size_t len = def.keywordLiteral.size();
        if (len > 0 && input.compare(pos, len, def.keywordLiteral) == 0)
            return static_cast<int>(len);
        return -1;
    }
    size_t cur = pos;
    for (const auto& piece : def.pieces) {
        int len = matchPiece(piece, cur);
        if (len < 0) return -1;
        cur += static_cast<size_t>(len);
    }
    if (cur == pos) return -1; // token vacio
    return static_cast<int>(cur - pos);
}

Token Scanner::nextToken() {
    while (current < input.size() && isWhiteSpace(input[current])) current++;
    if (current >= input.size()) return Token(Token::Kind::END);

    size_t first = current;
    int bestLen = -1;
    const TokenDef* bestDef = nullptr;

    // 1) Maximal munch: probamos todas las definiciones que no son
    //    palabra clave y nos quedamos con la coincidencia mas larga.
    for (const auto& def : defs) {
        if (def.isKeyword) continue;
        int len = matchTokenDef(def, first);
        if (len > bestLen) {
            bestLen = len;
            bestDef = &def;
        }
    }

    if (bestLen <= 0) {
        current++;
        return Token(Token::Kind::ERR, std::string(1, input[first]));
    }

    std::string lexeme = input.substr(first, static_cast<size_t>(bestLen));
    current = first + static_cast<size_t>(bestLen);

    // 2) Si el lexema coincide EXACTO con una palabra clave, esta gana
    //    sobre cualquier patron (igual que "if" gana sobre ID en FLEX).
    for (const auto& def : defs) {
        if (def.isKeyword && def.keywordLiteral == lexeme) {
            return Token(Token::Kind::MATCH, lexeme, def.name);
        }
    }

    return Token(Token::Kind::MATCH, lexeme, bestDef->name);
}
