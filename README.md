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
- `scanner.h` / `scanner.cpp` - el motor: recibe la lista de TokenDef y reconoce
  tokens con maximal munch, igual que hace FLEX.
- `mainwindow.h` / `mainwindow.cpp` - toda la interfaz Qt: las 7 pantallas
  (inicio, seleccion de pieza, elemento, repeticion, palabra clave, lista de
  tokens + prueba, ayuda) y la logica de navegacion.
- `main.cpp` - punto de entrada.
- `test_scanner.cpp` - prueba de linea de comandos del motor, sin necesidad de
  abrir el GUI (util para depurar la logica del automata por separado).
  Se compila aparte: `g++ -std=c++17 -o test test_scanner.cpp piece.cpp scanner.cpp`

## Que decisiones de diseno tomamos (para tu exposicion)

- No hay parser de expresiones regulares: el GUI arma directamente una lista
  plana de piezas (nunca un arbol), porque no se permite anidar una repeticion
  dentro de otra. Eso evita construir un compilador de regex completo como el
  de FLEX.
- Cada pieza es una `Element` (una o mas alternativas unidas por OR) opcionalmente
  envuelta en una repeticion 0-o-mas / 1-o-mas.
- El escaneo usa "maximal munch": en cada posicion se prueban TODAS las
  definiciones y se toma la mas larga; si el lexema resultante coincide exacto
  con una palabra clave, esta gana (igual que "if" le gana a ID en FLEX).
- Limitacion consciente: no se pueden alternar dos secuencias completas ya
  formadas (solo piezas individuales dentro de un elemento), y no hay soporte
  para cadenas de texto ("...") porque necesitarian una clase de caracter
  "negada" que el modelo actual no tiene.
