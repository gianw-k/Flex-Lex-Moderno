# Generador de Analizador Lexico (GUI)

Proyecto de Compiladores: crea tipos de token con botones (elemento, repeticion,
OR, palabra clave, y plantillas de Identificador/Punto flotante) y luego prueba
el analizador escaneando texto real.

## Como compilar

### Opcion 1: Qt Creator (recomendada)
1. Instala Qt Creator (con el "Qt Online Installer", eligiendo el kit de Qt Widgets
   con MinGW si estas en Windows sin Visual Studio, o el compilador que ya tengas).
2. Abre Qt Creator -> File -> Open File or Project -> selecciona `CMakeLists.txt`.
3. Qt Creator configura el proyecto solo. Presiona el boton verde de Run (Ctrl+R).

### Opcion 2: linea de comandos (Linux/Mac, o Windows con MinGW/MSVC + CMake)
```bash
mkdir build && cd build
cmake ..
cmake --build .
./LexerGUI      # en Windows: LexerGUI.exe
```

## Archivos

- `piece.h` / `piece.cpp` - modelo de datos: Alternative, Element, Piece, TokenDef.
  Aqui vive la representacion de "que es un token" armado desde el GUI.
- `token.h` - clase Token (resultado de escanear), simplificada como valor
  (sin `new`/`delete` como en el Scanner original de clase).
- `generador.h` / `generador.cpp` - el motor: convierte los TokenDef en un NFA
  (Thompson), lo determiniza (subconjuntos) y emite el `lexer.cpp`. El mismo
  DFA se usa para la vista previa dentro de la app.
- `mainwindow.h` / `mainwindow.cpp` - toda la interfaz Qt: las 8 pantallas
  (inicio, seleccion de pieza, elemento, repeticion, palabra clave, lista de
  tokens + prueba, codigo generado, ayuda) y la logica de navegacion.
- `main.cpp` - punto de entrada.
- `test_generador.cpp` - prueba de linea de comandos del motor, sin necesidad
  de abrir el GUI. Se compila aparte:
  `g++ -std=c++17 -o test test_generador.cpp generador.cpp piece.cpp`

## Como usar el codigo generado

Definir los tokens con los botones, ir a "Ver tokens", pulsar **Generar codigo
C++** y despues **Guardar como lexer.cpp**. Entonces:

```bash
g++ -std=c++17 lexer.cpp -o lexer
echo 'if iff x1 == 3.14' | ./lexer
```

Imprime una linea por token, con el nombre y el lexema.

## De la expresion regular al C++

| Paso | Donde |
|---|---|
| 1. Estructura -> NFA (Thompson) | `generador.cpp`: `construirElemento`, `construirPieza`, `construirNfa` |
| 2. NFA -> DFA (subconjuntos) | `generador.cpp`: `construirDfa` |
| 3. DFA -> C++ | `generador.cpp`: `generarCpp` |

## Que decisiones de diseno tomamos (para tu exposicion)

- No hay parser de expresiones regulares: el GUI arma directamente una lista
  plana de piezas (nunca un arbol), porque no se permite anidar una repeticion
  dentro de otra. Eso evita construir un compilador de regex completo como el
  de FLEX.
- Cada pieza es una `Element` (una o mas alternativas unidas por OR) opcionalmente
  envuelta en una repeticion 0-o-mas / 1-o-mas.
- Se construye UN solo automata con todas las reglas, no uno por token. Las
  palabras clave no se tratan aparte: entran al mismo DFA que los patrones.
- La prioridad es el orden de la lista: si un estado acepta por varias reglas
  gana la de mas arriba. Por eso "if" le gana a ID, igual que en FLEX.
- El escaneo usa "maximal munch": el motor recuerda el ultimo estado de
  aceptacion y retrocede hasta el, asi "3.14" sale como un FLOAT y no como
  tres tokens.
- Limitacion consciente: no se pueden alternar dos secuencias completas ya
  formadas (solo piezas individuales dentro de un elemento), y no hay soporte
  para cadenas de texto ("...") porque necesitarian una clase de caracter
  "negada" que el modelo actual no tiene.
