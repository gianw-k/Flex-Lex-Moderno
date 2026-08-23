#pragma once
#include <string>
#include <vector>
#include "piece.h"
#include "token.h"

struct Dfa {
    // tabla[estado][codigo ascii] = estado destino, o -1 si no hay transicion
    std::vector<std::vector<int>> tabla;

    // acepta[estado] = indice del TokenDef que acepta ahi, o -1
    std::vector<int> acepta;

    int estadosNfa = 0;

    // reglas que reconocen la cadena vacia; no detienen la generacion
    std::vector<std::string> avisos;
};

// Pasos 1 y 2: definiciones -> NFA (Thompson) -> DFA (subconjuntos)
Dfa construirDfa(const std::vector<TokenDef>& defs);

// Paso 3: DFA -> texto del lexer.cpp
std::string generarCpp(const std::vector<TokenDef>& defs, const Dfa& dfa);

// Recorre el DFA sobre un texto. Es el mismo algoritmo que se emite en el
// lexer.cpp, para que la vista previa de la app y el codigo generado no
// puedan dar resultados distintos.
std::vector<Token> escanearConDfa(const std::vector<TokenDef>& defs,
                                  const Dfa& dfa,
                                  const std::string& src);
