#include "generador.h"

#include <bitset>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>

namespace {

// PASO 1: definiciones -> NFA (Thompson)

struct Mov {
    std::bitset<128> chars;
    int destino;
};

struct EstadoNfa {
    std::vector<int> eps;
    std::vector<Mov> movs;
};

struct Nfa {
    std::vector<EstadoNfa> estados;

    int nuevo() {
        estados.emplace_back();
        return static_cast<int>(estados.size()) - 1;
    }
    void eps(int desde, int hacia) { estados[desde].eps.push_back(hacia); }
    void mov(int desde, const std::bitset<128>& chars, int hacia) {
        estados[desde].movs.push_back({chars, hacia});
    }
};

std::bitset<128> claseDe(Alternative::Kind k) {
    std::bitset<128> b;
    int desde, hasta;
    if (k == Alternative::UPPERCASE)      { desde = 'A'; hasta = 'Z'; }
    else if (k == Alternative::LOWERCASE) { desde = 'a'; hasta = 'z'; }
    else                                  { desde = '0'; hasta = '9'; }
    for (int c = desde; c <= hasta; c++) b.set(c);
    return b;
}

std::bitset<128> unSoloChar(char c) {
    std::bitset<128> b;
    b.set(static_cast<unsigned char>(c));
    return b;
}

void construirElemento(Nfa& n, int desde, int hacia, const Element& el) {
    for (const Alternative& a : el.alternatives) {
        if (a.kind == Alternative::SYMBOL) {
            int cur = desde;
            const std::string& lit = a.symbolLiteral;
            for (size_t i = 0; i < lit.size(); i++) {
                if (i + 1 == lit.size()) {
                    n.mov(cur, unSoloChar(lit[i]), hacia);
                } else {
                    int siguiente = n.nuevo();
                    n.mov(cur, unSoloChar(lit[i]), siguiente);
                    cur = siguiente;
                }
            }
        } else {
            n.mov(desde, claseDe(a.kind), hacia);
        }
    }
}

int construirPieza(Nfa& n, int desde, const Piece& p) {
    if (p.kind == Piece::SINGLE) {
        int fin = n.nuevo();
        construirElemento(n, desde, fin, p.element);
        return fin;
    }

    int ini = n.nuevo();
    n.eps(desde, ini);
    int med = n.nuevo();
    construirElemento(n, ini, med, p.element);
    n.eps(med, ini);
    int fin = n.nuevo();
    n.eps(med, fin);
    if (p.zeroOrMore) n.eps(ini, fin);  // unica diferencia entre '*' y '+'
    return fin;
}

int construirDef(Nfa& n, int desde, const TokenDef& d) {
    int cur = desde;
    if (d.isKeyword) {
        for (char c : d.keywordLiteral) {
            int siguiente = n.nuevo();
            n.mov(cur, unSoloChar(c), siguiente);
            cur = siguiente;
        }
        return cur;
    }
    for (const Piece& p : d.pieces) cur = construirPieza(n, cur, p);
    return cur;
}

// Un unico NFA con todas las reglas colgando del mismo estado inicial,
// igual que FLEX.
void construirNfa(const std::vector<TokenDef>& defs,
                  Nfa& n, int& inicio, std::vector<int>& finDeRegla) {
    inicio = n.nuevo();
    finDeRegla.assign(defs.size(), -1);
    for (size_t i = 0; i < defs.size(); i++) {
        int s = n.nuevo();
        n.eps(inicio, s);
        finDeRegla[i] = construirDef(n, s, defs[i]);
    }
}

// PASO 2: NFA -> DFA (subconjuntos)

std::set<int> cerraduraEps(const Nfa& n, const std::set<int>& conjunto) {
    std::vector<int> pila(conjunto.begin(), conjunto.end());
    std::set<int> out = conjunto;
    while (!pila.empty()) {
        int e = pila.back();
        pila.pop_back();
        for (int t : n.estados[e].eps) {
            if (out.insert(t).second) pila.push_back(t);
        }
    }
    return out;
}

std::set<int> mover(const Nfa& n, const std::set<int>& conjunto, int c) {
    std::set<int> out;
    for (int e : conjunto) {
        for (const Mov& m : n.estados[e].movs) {
            if (m.chars.test(c)) out.insert(m.destino);
        }
    }
    return out;
}

std::string escaparC(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
}

} // namespace

Dfa construirDfa(const std::vector<TokenDef>& defs) {
    Nfa n;
    int inicio = 0;
    std::vector<int> finDeRegla;
    construirNfa(defs, n, inicio, finDeRegla);

    std::map<int, int> aceptaNfa;
    for (size_t i = 0; i < finDeRegla.size(); i++) {
        auto it = aceptaNfa.find(finDeRegla[i]);
        if (it == aceptaNfa.end() || it->second > static_cast<int>(i))
            aceptaNfa[finDeRegla[i]] = static_cast<int>(i);
    }

    Dfa r;
    r.estadosNfa = static_cast<int>(n.estados.size());

    std::set<int> s0 = cerraduraEps(n, std::set<int>{inicio});

    // Si el final de una regla ya es alcanzable solo con transiciones vacias,
    // esa regla reconoce la cadena vacia.
    for (size_t i = 0; i < defs.size(); i++) {
        if (s0.count(finDeRegla[i])) r.avisos.push_back(defs[i].name);
    }

    std::map<std::set<int>, int> ids;
    std::vector<std::set<int>> conjuntos;
    ids[s0] = 0;
    conjuntos.push_back(s0);

    for (size_t i = 0; i < conjuntos.size(); i++) {
        // copia: el vector se reubica al agregar subconjuntos nuevos
        const std::set<int> actual = conjuntos[i];

        r.tabla.emplace_back(128, -1);

        // Si acepta por varias reglas gana la de menor indice.
        // Eso es lo que hace que "if" le gane a "ID".
        int mejor = -1;
        for (int e : actual) {
            auto it = aceptaNfa.find(e);
            if (it == aceptaNfa.end()) continue;
            if (mejor < 0 || it->second < mejor) mejor = it->second;
        }
        r.acepta.push_back(mejor);

        for (int c = 0; c < 128; c++) {
            std::set<int> mv = mover(n, actual, c);
            if (mv.empty()) continue;
            std::set<int> cl = cerraduraEps(n, mv);

            auto it = ids.find(cl);
            int id;
            if (it == ids.end()) {
                id = static_cast<int>(conjuntos.size());
                ids[cl] = id;
                conjuntos.push_back(cl);
            } else {
                id = it->second;
            }
            r.tabla[i][c] = id;
        }
    }

    return r;
}

std::vector<Token> escanearConDfa(const std::vector<TokenDef>& defs,
                                  const Dfa& dfa,
                                  const std::string& src) {
    std::vector<Token> salida;
    size_t i = 0;
    while (i < src.size()) {
        char c0 = src[i];
        if (c0 == ' ' || c0 == '\t' || c0 == '\n' || c0 == '\r') { i++; continue; }

        int estado = 0;
        int ultimoAcepta = -1;
        size_t j = i, ultimoFin = i;

        while (j < src.size()) {
            unsigned char c = static_cast<unsigned char>(src[j]);
            if (c >= 128) break;
            int siguiente = dfa.tabla[estado][c];
            if (siguiente < 0) break;
            estado = siguiente;
            j++;
            if (dfa.acepta[estado] >= 0) { ultimoAcepta = estado; ultimoFin = j; }
        }

        if (ultimoAcepta < 0) {
            salida.push_back(Token(Token::Kind::ERR, std::string(1, src[i])));
            i++;
        } else {
            salida.push_back(Token(Token::Kind::MATCH,
                                   src.substr(i, ultimoFin - i),
                                   defs[dfa.acepta[ultimoAcepta]].name));
            i = ultimoFin;
        }
    }
    salida.push_back(Token(Token::Kind::END));
    return salida;
}

// PASO 3: DFA -> codigo C++

std::string generarCpp(const std::vector<TokenDef>& defs, const Dfa& dfa) {
    const int n = static_cast<int>(dfa.tabla.size());
    std::ostringstream o;

    o << "// ============================================================\n";
    o << "// lexer.cpp - GENERADO AUTOMATICAMENTE, no editar a mano.\n";
    o << "//\n";
    o << "// Reglas (el orden es la prioridad):\n";
    for (size_t i = 0; i < defs.size(); i++)
        o << "//   " << (i + 1) << ". " << defs[i].name
          << "  ->  " << defs[i].describe() << "\n";
    o << "//\n";
    o << "// NFA de " << dfa.estadosNfa << " estados -> DFA de " << n << " estados.\n";
    for (const std::string& a : dfa.avisos)
        o << "// AVISO: la regla " << a << " reconoce la cadena vacia.\n";
    o << "// ============================================================\n\n";

    o << "#include <iostream>\n";
    o << "#include <string>\n";
    o << "#include <vector>\n\n";

    // parte variable: cambia con cada gramatica
    o << "static const int NESTADOS = " << n << ";\n\n";
    o << "// TABLA[estado][caracter] = estado destino, o -1 si no hay transicion.\n";
    o << "static const int TABLA[NESTADOS][128] = {\n";
    for (int i = 0; i < n; i++) {
        o << "    { // estado " << i;
        if (dfa.acepta[i] >= 0) o << "  (acepta " << defs[dfa.acepta[i]].name << ")";
        o << "\n";
        for (int k = 0; k < 128; k += 16) {
            o << "        ";
            for (int j = k; j < k + 16; j++) {
                o << std::setw(3) << dfa.tabla[i][j];
                if (j + 1 < 128) o << ",";
            }
            o << "\n";
        }
        o << "    }" << (i + 1 < n ? "," : "") << "\n";
    }
    o << "};\n\n";

    o << "// Nombre del token que acepta cada estado, o nullptr si no es final.\n";
    o << "static const char* ACEPTA[NESTADOS] = {\n";
    for (int i = 0; i < n; i++) {
        o << "    ";
        if (dfa.acepta[i] >= 0) o << "\"" << escaparC(defs[dfa.acepta[i]].name) << "\"";
        else                    o << "nullptr";
        o << (i + 1 < n ? "," : "") << "   // estado " << i << "\n";
    }
    o << "};\n\n";

    // parte fija: el runtime es siempre el mismo
    o << R"CPP(struct Token {
    std::string lexema;
    std::string nombre;
};

static bool esEspacio(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Recorre el DFA guardando el ultimo estado de aceptacion visto y retrocede
// hasta el: eso es el "maximal munch" de FLEX.
std::vector<Token> escanear(const std::string& src) {
    std::vector<Token> salida;
    size_t i = 0;
    while (i < src.size()) {
        if (esEspacio(src[i])) { i++; continue; }

        int estado = 0;
        int ultimoAcepta = -1;
        size_t j = i, ultimoFin = i;

        while (j < src.size()) {
            unsigned char c = static_cast<unsigned char>(src[j]);
            if (c >= 128) break;
            int siguiente = TABLA[estado][c];
            if (siguiente < 0) break;
            estado = siguiente;
            j++;
            if (ACEPTA[estado] != nullptr) { ultimoAcepta = estado; ultimoFin = j; }
        }

        if (ultimoAcepta < 0) {
            salida.push_back({ std::string(1, src[i]), "ERROR" });
            i++;
        } else {
            salida.push_back({ src.substr(i, ultimoFin - i), ACEPTA[ultimoAcepta] });
            i = ultimoFin;
        }
    }
    return salida;
}

int main() {
    std::string linea, todo;
    while (std::getline(std::cin, linea)) todo += linea + "\n";

    for (const Token& t : escanear(todo))
        std::cout << t.nombre << "\t" << t.lexema << "\n";
    return 0;
}
)CPP";

    return o.str();
}
