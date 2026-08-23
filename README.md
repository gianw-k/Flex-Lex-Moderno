# Generador de Analizador Lexico (GUI)

Proyecto de Compiladores: crea tipos de token con botones (elemento, repeticion,
OR, palabra clave) y luego prueba el analizador escaneando texto real, o genera
el analizador como codigo C++ independiente.

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

## Navegacion y salida del programa

- La flecha **`<-`** siempre CANCELA o retrocede, nunca confirma nada:
  - Si estas armando el contenido de una repeticion, `<-` la cancela sin
    guardar lo que llevabas ahi.
  - Si estas en las pantallas de elemento, repeticion, palabra clave, lista
    de tokens o ayuda, `<-` te regresa a la pantalla principal de piezas.
  - Si estas en la pantalla principal de piezas (fuera de una repeticion),
    `<-` intenta volver a la pantalla de inicio.
- **"Cerrar repeticion y continuar"** (fondo azul, a la derecha de la fila
  secundaria) es la unica forma de CONFIRMAR una repeticion y seguir
  agregando mas piezas al mismo token (por ejemplo, para armar un punto
  flotante a mano: repeticion de digitos, cerrarla, simbolo `.`, otra
  repeticion de digitos).
- **"<- Deshacer ultima pieza"** (a la izquierda de esa misma fila) aparece
  en cuanto el token en construccion tiene al menos una pieza confirmada
  O una pieza pendiente sin confirmar (ver nota de bug abajo), y quita la
  ultima una por una. Cuando ya no queda nada, el boton desaparece y la
  fila vuelve a estar vacia. Esta fila usa un stretch en el medio para que
  ninguno de los dos botones se estire a ocupar todo el ancho.
- No hay un boton dedicado para salir del programa dentro de esta pantalla:
  para eso esta el `<-` de arriba (si estas en el nivel superior, sin
  repeticion abierta) o el boton "Salir" de la pantalla de inicio.
- Al finalizar un token que es exactamente UN simbolo suelto (sin
  repeticion, sin combinarlo con nada mas), el cuadro para escribir el
  nombre viene pre-llenado con el nombre del simbolo en mayusculas (ej.
  `+` con nombre "suma" sugiere `SUMA`), igual que ya pasaba con las
  palabras clave. Ver `MainWindow::sugerirNombrePorDefecto()`.
- **"Salir del programa"** ya no existe como boton separado: la proteccion
  de salida vive en el `<-` que va hacia Inicio y en el "Salir" de Inicio.
  Si hay tokens ya guardados o una pieza a medio armar, aparece un dialogo
  de advertencia con dos opciones:
  - **"Salir y borrar todo"** (boton rojo): borra todos los tokens definidos
    y la pieza en progreso, y procede a salir. El analizador que se estaba
    armando nunca llega a crearse.
  - **"Continuar creando"**: cancela la salida, te quedas donde estabas sin
    perder nada.
  Esta logica vive en `MainWindow::confirmarSalirSiHayProgreso()`.

## Generar el analizador como codigo C++ independiente

En la pantalla "Ver tokens" hay un boton **"Generar codigo C++"**. Al presionarlo,
te deja elegir donde guardar un archivo `.cpp` que:

- No depende de Qt para nada (solo usa `<string>`, `<vector>`, `<iostream>`, `<cctype>`).
- Contiene copiadas las clases `Alternative`, `Element`, `Piece`, `TokenDef`, `Token`
  y `Scanner` (el mismo motor que usa el GUI).
- Tiene una funcion `construirDefiniciones()` con exactamente los tokens que
  armaste en el GUI, ya escritos como codigo C++ (no como configuracion externa).
- Tiene un `main()` que lee una linea de la entrada estandar y la escanea.

Se compila con cualquier compilador, sin instalar nada mas:
```bash
g++ -std=c++17 analizador_generado.cpp -o analizador
./analizador
```

Esto es lo que entregas aparte del proyecto del GUI: es tu "generador de
analizadores lexicos" funcionando de verdad, igual que FLEX genera un `.c`
a partir de las reglas que le das.

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
- El GUI no trae plantillas predefinidas (como Identificador o Punto
  flotante): todo token, sin excepcion, lo arma el usuario con los botones.
  Esto fue una decision deliberada para que el analizador que se genera sea
  siempre el que el usuario realmente construyo, no uno parcialmente
  precocinado por el programa.

## Bugs corregidos

- **Deshacer no veia la pieza pendiente.** Un elemento elegido con
  "Elemento unico" no se guarda de inmediato en `currentPieces` -- se queda
  pendiente en `currentElement`/`hasCurrentElement` por si el usuario le
  agrega un OR despues, y solo se confirma cuando se agrega la siguiente
  pieza o se finaliza el token. `deshacerUltimaPieza()` solo miraba
  `currentPieces`, asi que si la ultima pieza agregada todavia estaba
  pendiente, el boton ni aparecia, o al presionarlo se borraba la pieza
  ANTERIOR en vez de la ultima. Ahora `deshacerUltimaPieza()` primero
  descarta el elemento pendiente (si existe) antes de tocar `currentPieces`,
  y `btnDeshacerPieza` se muestra si hay pieza pendiente O piezas
  confirmadas.
- **Las repeticiones de un solo simbolo no llevaban parentesis en la vista
  previa.** `Element::describe()` solo pone parentesis cuando hay mas de
  una alternativa (por el OR), asi que una repeticion de una sola alternativa
  se veia como `0-9*` en vez de `(0-9)*`. `Piece::describe()` ahora siempre
  envuelve en parentesis el elemento que se repite (sin duplicar los
  parentesis si el elemento ya los trae por tener varias alternativas).
