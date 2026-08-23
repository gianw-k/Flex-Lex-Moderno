#include "mainwindow.h"
#include "scanner.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QFont>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// ---------------------------------------------------------------
// Construccion de la ventana
// ---------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Generador de Analizador Lexico");
    resize(720, 560);

    stack = new QStackedWidget;
    setCentralWidget(stack);

    stack->addWidget(crearPaginaInicio());     // P_INICIO
    stack->addWidget(crearPaginaPieza());      // P_PIEZA
    stack->addWidget(crearPaginaElemento());   // P_ELEMENTO
    stack->addWidget(crearPaginaRepeticion());  // P_REPETICION
    stack->addWidget(crearPaginaKeyword());    // P_KEYWORD
    stack->addWidget(crearPaginaTokens());     // P_TOKENS
    stack->addWidget(crearPaginaAyuda());      // P_AYUDA

    stack->setCurrentIndex(P_INICIO);
}

// ---------------------------------------------------------------
// Navegacion
// ---------------------------------------------------------------

void MainWindow::irA(Pagina p) {
    stack->setCurrentIndex(p);
    if (p == P_PIEZA) refrescarPantallaPieza();
    if (p == P_TOKENS) refrescarListaTokens();
}

void MainWindow::volver() {
    switch (stack->currentIndex()) {
        case P_PIEZA:
            if (buildingRepetition) {
                // <- cancela la repeticion en progreso, no la confirma.
                // Para confirmarla y seguir agregando piezas, se usa el
                // boton "Cerrar repeticion y continuar".
                hasCurrentElement = false;
                currentElement = Element();
                buildingRepetition = false;
                refrescarPantallaPieza();
            } else {
                if (confirmarSalirSiHayProgreso()) {
                    irA(P_INICIO);
                }
                // si no se confirma, nos quedamos en P_PIEZA tal cual
            }
            break;
        case P_ELEMENTO:
        case P_REPETICION:
        case P_KEYWORD:
        case P_TOKENS:
        case P_AYUDA:
            irA(P_PIEZA);
            break;
        default:
            irA(P_INICIO);
    }
}

bool MainWindow::nombreDisponible(const std::string& nombre) {
    for (const auto& d : tokenDefs) {
        if (d.name == nombre) return false;
    }
    return true;
}

// Se llama antes de cualquier salida "grande" (volver a Inicio, o cerrar el
// programa). Si no hay nada que perder, deja pasar directo. Si hay tokens ya
// guardados o una pieza a medio armar, pregunta y, si el usuario confirma
// salir, borra todo (el analizador que se estaba armando nunca llega a
// existir). Devuelve true si se debe proceder con la salida.
bool MainWindow::confirmarSalirSiHayProgreso() {
    bool hayProgreso = !tokenDefs.empty() || !currentPieces.empty() || hasCurrentElement;
    if (!hayProgreso) return true;

    QMessageBox caja(this);
    caja.setIcon(QMessageBox::Warning);
    caja.setWindowTitle("Analizador sin terminar");
    caja.setText("Todavia no terminaste de crear el analizador lexico.\n\n"
                  "Si sales ahora se van a borrar todos los tokens que ya "
                  "definiste, y el analizador nunca llega a crearse.");
    QPushButton* btnSalir = caja.addButton("Salir y borrar todo", QMessageBox::DestructiveRole);
    btnSalir->setStyleSheet("background-color:#b33939; color:white; font-weight:bold;");
    QPushButton* btnContinuar = caja.addButton("Continuar creando", QMessageBox::RejectRole);
    caja.setDefaultButton(btnContinuar);
    caja.exec();

    if (caja.clickedButton() == btnSalir) {
        tokenDefs.clear();
        currentPieces.clear();
        hasCurrentElement = false;
        currentElement = Element();
        buildingRepetition = false;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------
// Pantalla: inicio
// ---------------------------------------------------------------

QWidget* MainWindow::crearPaginaInicio() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->addStretch();

    QLabel* titulo = new QLabel("Generador de Analizador Lexico");
    titulo->setAlignment(Qt::AlignCenter);
    QFont f = titulo->font();
    f.setPointSize(18);
    f.setBold(true);
    titulo->setFont(f);
    layout->addWidget(titulo);

    layout->addSpacing(24);

    QPushButton* btnComenzar = new QPushButton("Comenzar");
    QPushButton* btnSalir = new QPushButton("Salir");
    btnComenzar->setFixedHeight(42);
    btnSalir->setFixedHeight(42);

    connect(btnComenzar, &QPushButton::clicked, this, [this]() { irA(P_PIEZA); });
    connect(btnSalir, &QPushButton::clicked, this, [this]() {
        if (confirmarSalirSiHayProgreso()) QApplication::quit();
    });

    QVBoxLayout* botones = new QVBoxLayout;
    botones->addWidget(btnComenzar);
    botones->addWidget(btnSalir);
    QHBoxLayout* centrado = new QHBoxLayout;
    centrado->addStretch();
    QWidget* caja = new QWidget;
    caja->setLayout(botones);
    caja->setFixedWidth(260);
    centrado->addWidget(caja);
    centrado->addStretch();
    layout->addLayout(centrado);

    layout->addStretch();
    return page;
}

// ---------------------------------------------------------------
// Pantalla: seleccionar tipo de pieza (pantalla principal)
// ---------------------------------------------------------------

QWidget* MainWindow::crearPaginaPieza() {
    QWidget* page = new QWidget;
    QVBoxLayout* outer = new QVBoxLayout(page);

    // barra superior
    QHBoxLayout* topBar = new QHBoxLayout;
    QPushButton* btnAtras = new QPushButton("<-");
    btnAtras->setFixedWidth(40);
    connect(btnAtras, &QPushButton::clicked, this, &MainWindow::volver);

    lblHeaderPieza = new QLabel("Ingrese un tipo de token");
    lblHeaderPieza->setAlignment(Qt::AlignCenter);
    lblHeaderPieza->setWordWrap(true);
    QFont f = lblHeaderPieza->font();
    f.setPointSize(13);
    f.setBold(true);
    lblHeaderPieza->setFont(f);

    QPushButton* btnVerTokens = new QPushButton("Ver tokens");
    connect(btnVerTokens, &QPushButton::clicked, this, [this]() { irA(P_TOKENS); });

    QPushButton* btnAyuda = new QPushButton("Ayuda");
    connect(btnAyuda, &QPushButton::clicked, this, [this]() { irA(P_AYUDA); });

    topBar->addWidget(btnAtras);
    topBar->addWidget(lblHeaderPieza, 1);
    topBar->addWidget(btnVerTokens);
    topBar->addWidget(btnAyuda);
    outer->addLayout(topBar);

    // vista previa
    lblPreview = new QLabel("Patron: (vacio)");
    lblPreview->setAlignment(Qt::AlignCenter);
    lblPreview->setWordWrap(true);
    lblPreview->setStyleSheet("color: #555; font-family: monospace; padding: 10px;");
    outer->addWidget(lblPreview);

    outer->addStretch();

    // mitad inferior: opciones
    QGridLayout* grid = new QGridLayout;
    grid->setSpacing(10);

    btnElementoUnico = new QPushButton("Elemento unico");
    connect(btnElementoUnico, &QPushButton::clicked, this, [this]() {
        if (!buildingRepetition) confirmarElementoPendiente();
        irA(P_ELEMENTO);
    });

    btnRepeticion = new QPushButton("Repeticion");
    connect(btnRepeticion, &QPushButton::clicked, this, [this]() {
        confirmarElementoPendiente();
        irA(P_REPETICION);
    });

    btnOr = new QPushButton("OR (union)");
    connect(btnOr, &QPushButton::clicked, this, [this]() {
        if (!hasCurrentElement) {
            QMessageBox::warning(this, "OR", "Primero agrega un elemento antes de usar OR.");
            return;
        }
        irA(P_ELEMENTO);
    });

    btnPalabraClave = new QPushButton("Palabra clave");
    connect(btnPalabraClave, &QPushButton::clicked, this, [this]() { irA(P_KEYWORD); });

    btnCerrarRepeticion = new QPushButton("Cerrar repeticion y continuar");
    btnCerrarRepeticion->setStyleSheet("background-color: #2d5f8a;");
    connect(btnCerrarRepeticion, &QPushButton::clicked, this, &MainWindow::cerrarRepeticion);

    btnDeshacerPieza = new QPushButton("<- Deshacer ultima pieza");
    connect(btnDeshacerPieza, &QPushButton::clicked, this, &MainWindow::deshacerUltimaPieza);

    // btnDeshacerPieza vive a la izquierda y solo aparece si ya hay piezas
    // agregadas; btnCerrarRepeticion vive a la derecha y solo aparece en
    // modo repeticion. El stretch del medio evita que cualquiera de los
    // dos se estire y ocupe todo el ancho.
    QHBoxLayout* filaAccionSecundaria = new QHBoxLayout;
    filaAccionSecundaria->addWidget(btnDeshacerPieza);
    filaAccionSecundaria->addStretch();
    filaAccionSecundaria->addWidget(btnCerrarRepeticion);

    QPushButton* btnFinalizar = new QPushButton("Finalizar token");
    btnFinalizar->setStyleSheet("font-weight: bold;");
    connect(btnFinalizar, &QPushButton::clicked, this, &MainWindow::finalizarToken);

    grid->addWidget(btnElementoUnico, 0, 0);
    grid->addWidget(btnRepeticion, 0, 1);
    grid->addWidget(btnOr, 1, 0);
    grid->addWidget(btnPalabraClave, 1, 1);
    grid->addLayout(filaAccionSecundaria, 2, 0, 1, 2);
    grid->addWidget(btnFinalizar, 3, 0, 1, 2);

    outer->addLayout(grid);
    return page;
}

void MainWindow::refrescarPantallaPieza() {
    if (buildingRepetition) {
        lblHeaderPieza->setText(QString("Construyendo el contenido de la repeticion (%1) - agrega elementos "
                                         "y presiona 'Cerrar repeticion' para seguir, o <- para cancelarla")
                                     .arg(repetitionZeroOrMore ? "0 o mas" : "1 o mas"));
        btnRepeticion->setVisible(false);
        btnPalabraClave->setVisible(false);
        btnElementoUnico->setVisible(!hasCurrentElement);
        btnCerrarRepeticion->setVisible(true);
    } else {
        lblHeaderPieza->setText("Ingrese un tipo de token");
        btnRepeticion->setVisible(true);
        btnElementoUnico->setVisible(true);
        btnPalabraClave->setVisible(currentPieces.empty() && !hasCurrentElement);
        btnCerrarRepeticion->setVisible(false);
    }
    btnDeshacerPieza->setVisible(hasCurrentElement || !currentPieces.empty());
    btnOr->setEnabled(hasCurrentElement);

    QString preview;
    for (auto& p : currentPieces) preview += QString::fromStdString(p.describe());
    if (hasCurrentElement) {
        preview += QString::fromStdString(currentElement.describe());
        if (buildingRepetition) preview += "...";
    }
    lblPreview->setText(preview.isEmpty() ? "Patron: (vacio)" : "Patron: " + preview);
}

void MainWindow::confirmarElementoPendiente() {
    if (hasCurrentElement) {
        Piece p;
        p.kind = Piece::SINGLE;
        p.element = currentElement;
        currentPieces.push_back(p);
        hasCurrentElement = false;
        currentElement = Element();
    }
}

void MainWindow::agregarAlternativa(Alternative::Kind kind, const std::string& lit, const std::string& nombre) {
    if (!hasCurrentElement) {
        currentElement = Element();
        hasCurrentElement = true;
    }
    Alternative a;
    a.kind = kind;
    a.symbolLiteral = lit;
    a.symbolName = nombre;
    currentElement.alternatives.push_back(a);
}

// ---------------------------------------------------------------
// Pantalla: elegir elemento (mayusculas / minusculas / digitos / simbolo)
// ---------------------------------------------------------------

QWidget* MainWindow::crearPaginaElemento() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* topBar = new QHBoxLayout;
    QPushButton* btnAtras = new QPushButton("<-");
    connect(btnAtras, &QPushButton::clicked, this, &MainWindow::volver);
    topBar->addWidget(btnAtras);
    topBar->addWidget(new QLabel("Elige el elemento"), 1);
    layout->addLayout(topBar);

    QGridLayout* gridBasico = new QGridLayout;
    QPushButton* btnMayus = new QPushButton("Mayusculas (A-Z)");
    QPushButton* btnMinus = new QPushButton("Minusculas (a-z)");
    QPushButton* btnDigitos = new QPushButton("Digitos (0-9)");
    connect(btnMayus, &QPushButton::clicked, this, [this]() { agregarAlternativa(Alternative::UPPERCASE); volver(); });
    connect(btnMinus, &QPushButton::clicked, this, [this]() { agregarAlternativa(Alternative::LOWERCASE); volver(); });
    connect(btnDigitos, &QPushButton::clicked, this, [this]() { agregarAlternativa(Alternative::DIGIT); volver(); });
    gridBasico->addWidget(btnMayus, 0, 0);
    gridBasico->addWidget(btnMinus, 0, 1);
    gridBasico->addWidget(btnDigitos, 0, 2);
    layout->addLayout(gridBasico);

    layout->addWidget(new QLabel("Simbolos:"));
    QGridLayout* gridSimbolos = new QGridLayout;
    gridSimbolos->setSpacing(6);

    struct Preset { const char* nombre; const char* literal; };
    static const std::vector<Preset> presets = {
        {"suma", "+"}, {"resta", "-"}, {"multiplicacion", "*"}, {"division", "/"},
        {"potencia", "**"}, {"asignacion", "="}, {"igual a", "=="}, {"distinto de", "!="},
        {"menor que", "<"}, {"mayor que", ">"}, {"menor o igual", "<="}, {"mayor o igual", ">="},
        {"parentesis abre", "("}, {"parentesis cierra", ")"}, {"coma", ","}, {"punto y coma", ";"}
    };
    int r = 0, c = 0;
    for (const auto& sp : presets) {
        QPushButton* b = new QPushButton(QString("%1  (%2)").arg(sp.nombre, sp.literal));
        std::string lit = sp.literal;
        std::string nom = sp.nombre;
        connect(b, &QPushButton::clicked, this, [this, lit, nom]() {
            agregarAlternativa(Alternative::SYMBOL, lit, nom);
            volver();
        });
        gridSimbolos->addWidget(b, r, c);
        c++;
        if (c >= 4) { c = 0; r++; }
    }
    layout->addLayout(gridSimbolos);

    QHBoxLayout* customRow = new QHBoxLayout;
    QLineEdit* txtCustom = new QLineEdit;
    txtCustom->setPlaceholderText("Otro simbolo (ej. . _ \")");
    QPushButton* btnCustom = new QPushButton("Agregar simbolo");
    connect(btnCustom, &QPushButton::clicked, this, [this, txtCustom]() {
        QString val = txtCustom->text();
        if (val.isEmpty()) {
            QMessageBox::warning(this, "Simbolo", "Escribe un simbolo primero.");
            return;
        }
        std::string lit = val.toStdString();
        agregarAlternativa(Alternative::SYMBOL, lit, lit);
        txtCustom->clear();
        volver();
    });
    customRow->addWidget(txtCustom);
    customRow->addWidget(btnCustom);
    layout->addLayout(customRow);

    layout->addStretch();
    return page;
}

// ---------------------------------------------------------------
// Pantalla: elegir tipo de repeticion
// ---------------------------------------------------------------

QWidget* MainWindow::crearPaginaRepeticion() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* topBar = new QHBoxLayout;
    QPushButton* btnAtras = new QPushButton("<-");
    connect(btnAtras, &QPushButton::clicked, this, &MainWindow::volver);
    topBar->addWidget(btnAtras);
    topBar->addWidget(new QLabel("Elige el tipo de repeticion"), 1);
    layout->addLayout(topBar);

    layout->addSpacing(20);

    QPushButton* btnCero = new QPushButton("0 o mas   ( * )");
    QPushButton* btnUno = new QPushButton("1 o mas   ( + )");
    btnCero->setFixedHeight(44);
    btnUno->setFixedHeight(44);
    connect(btnCero, &QPushButton::clicked, this, [this]() { iniciarRepeticion(true); });
    connect(btnUno, &QPushButton::clicked, this, [this]() { iniciarRepeticion(false); });

    layout->addWidget(btnCero);
    layout->addWidget(btnUno);
    layout->addStretch();
    return page;
}

void MainWindow::iniciarRepeticion(bool ceroOMas) {
    repetitionZeroOrMore = ceroOMas;
    buildingRepetition = true;
    hasCurrentElement = false;
    currentElement = Element();
    irA(P_PIEZA);
}

void MainWindow::cerrarRepeticion() {
    if (!hasCurrentElement) {
        QMessageBox::warning(this, "Repeticion", "Agrega al menos un elemento antes de cerrar la repeticion.");
        return;
    }
    Piece p;
    p.kind = Piece::REPETITION;
    p.element = currentElement;
    p.zeroOrMore = repetitionZeroOrMore;
    currentPieces.push_back(p);
    hasCurrentElement = false;
    currentElement = Element();
    buildingRepetition = false;
    irA(P_PIEZA);
}

void MainWindow::deshacerUltimaPieza() {
    if (hasCurrentElement) {
        // Habia un elemento (posiblemente con OR) pendiente de confirmar
        // todavia -- es, para el usuario, "la ultima pieza agregada" aunque
        // no este en currentPieces todavia. Se descarta completo.
        hasCurrentElement = false;
        currentElement = Element();
    } else if (!currentPieces.empty()) {
        currentPieces.pop_back();
    }
    refrescarPantallaPieza();
}

// ---------------------------------------------------------------
// Pantalla: palabra clave
// ---------------------------------------------------------------

QWidget* MainWindow::crearPaginaKeyword() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* topBar = new QHBoxLayout;
    QPushButton* btnAtras = new QPushButton("<-");
    connect(btnAtras, &QPushButton::clicked, this, &MainWindow::volver);
    topBar->addWidget(btnAtras);
    topBar->addWidget(new QLabel("Escribe la palabra clave"), 1);
    layout->addLayout(topBar);

    QLineEdit* txt = new QLineEdit;
    txt->setPlaceholderText("ej. while, if, sqrt...");
    layout->addWidget(txt);

    QPushButton* btnAgregar = new QPushButton("Agregar y nombrar");
    connect(btnAgregar, &QPushButton::clicked, this, [this, txt]() {
        QString palabra = txt->text().trimmed();
        if (palabra.isEmpty()) {
            QMessageBox::warning(this, "Palabra clave", "Escribe una palabra primero.");
            return;
        }
        bool ok;
        QString nombre = QInputDialog::getText(this, "Nombre del token",
                                                "Nombre para este token:", QLineEdit::Normal,
                                                palabra.toUpper(), &ok);
        if (!ok || nombre.trimmed().isEmpty()) return;
        if (!nombreDisponible(nombre.toStdString())) {
            QMessageBox::warning(this, "Nombre repetido", "Ya existe un token con ese nombre.");
            return;
        }
        TokenDef def;
        def.name = nombre.toStdString();
        def.isKeyword = true;
        def.keywordLiteral = palabra.toStdString();
        tokenDefs.push_back(def);
        txt->clear();
        irA(P_PIEZA);
    });
    layout->addWidget(btnAgregar);
    layout->addStretch();
    return page;
}

// ---------------------------------------------------------------
// Finalizar token
// ---------------------------------------------------------------

// Si el token que se esta por finalizar es exactamente UN simbolo solo
// (sin repeticion, sin OR con otra cosa), sugiere su nombre amigable como
// nombre por defecto, igual que ya se hace con las palabras clave.
QString MainWindow::sugerirNombrePorDefecto() const {
    if (currentPieces.size() == 1) {
        const Piece& p = currentPieces[0];
        if (p.kind == Piece::SINGLE && p.element.alternatives.size() == 1) {
            const Alternative& a = p.element.alternatives[0];
            if (a.kind == Alternative::SYMBOL && !a.symbolName.empty()) {
                QString nombre = QString::fromStdString(a.symbolName).toUpper();
                nombre.replace(' ', '_');
                return nombre;
            }
        }
    }
    return "";
}

void MainWindow::finalizarToken() {
    if (buildingRepetition) {
        // Si habia una repeticion sin cerrar, se cierra automaticamente
        // con lo que se alcanzo a elegir (si no se eligio nada, se descarta).
        if (hasCurrentElement) {
            Piece p;
            p.kind = Piece::REPETITION;
            p.element = currentElement;
            p.zeroOrMore = repetitionZeroOrMore;
            currentPieces.push_back(p);
        }
        hasCurrentElement = false;
        currentElement = Element();
        buildingRepetition = false;
    } else {
        confirmarElementoPendiente();
    }
    if (currentPieces.empty()) {
        QMessageBox::warning(this, "Token vacio", "Agrega al menos una pieza antes de finalizar.");
        return;
    }
    bool ok;
    QString nombre = QInputDialog::getText(this, "Nombre del token", "Nombre para este token:",
                                            QLineEdit::Normal, sugerirNombrePorDefecto(), &ok);
    if (!ok || nombre.trimmed().isEmpty()) return;
    if (!nombreDisponible(nombre.toStdString())) {
        QMessageBox::warning(this, "Nombre repetido", "Ya existe un token con ese nombre.");
        return;
    }
    TokenDef def;
    def.name = nombre.toStdString();
    def.isKeyword = false;
    def.pieces = currentPieces;
    tokenDefs.push_back(def);

    currentPieces.clear();
    hasCurrentElement = false;
    currentElement = Element();
    QMessageBox::information(this, "Token guardado", "Token '" + nombre + "' guardado.");
    refrescarPantallaPieza();
}

// ---------------------------------------------------------------
// Pantalla: lista de tokens + probar el analizador
// ---------------------------------------------------------------

QWidget* MainWindow::crearPaginaTokens() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* topBar = new QHBoxLayout;
    QPushButton* btnAtras = new QPushButton("<-");
    connect(btnAtras, &QPushButton::clicked, this, &MainWindow::volver);
    topBar->addWidget(btnAtras);
    topBar->addWidget(new QLabel("Tokens definidos"), 1);
    layout->addLayout(topBar);

    listaDefiniciones = new QListWidget;
    listaDefiniciones->setMaximumHeight(140);
    layout->addWidget(listaDefiniciones);

    layout->addWidget(new QLabel("Codigo de prueba:"));
    txtCodigoFuente = new QTextEdit;
    txtCodigoFuente->setPlaceholderText("Escribe o pega aqui el texto a escanear...");
    txtCodigoFuente->setMaximumHeight(80);
    layout->addWidget(txtCodigoFuente);

    QHBoxLayout* filaBotones = new QHBoxLayout;
    QPushButton* btnEscanear = new QPushButton("Escanear (probar aqui)");
    connect(btnEscanear, &QPushButton::clicked, this, &MainWindow::escanear);
    QPushButton* btnGenerar = new QPushButton("Generar codigo C++");
    btnGenerar->setStyleSheet("font-weight: bold;");
    connect(btnGenerar, &QPushButton::clicked, this, &MainWindow::generarCodigo);
    filaBotones->addWidget(btnEscanear);
    filaBotones->addWidget(btnGenerar);
    layout->addLayout(filaBotones);

    tablaTokens = new QTableWidget(0, 3);
    tablaTokens->setHorizontalHeaderLabels({"Lexema", "Tipo", "Estado"});
    tablaTokens->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(tablaTokens);

    return page;
}

void MainWindow::refrescarListaTokens() {
    listaDefiniciones->clear();
    for (const auto& d : tokenDefs) {
        QString linea = QString::fromStdString(d.name) + "  =  " + QString::fromStdString(d.describe());
        listaDefiniciones->addItem(linea);
    }
    if (tokenDefs.empty()) {
        listaDefiniciones->addItem("(sin tokens definidos todavia)");
    }
}

void MainWindow::escanear() {
    std::string codigo = txtCodigoFuente->toPlainText().toStdString();
    Scanner sc(codigo, tokenDefs);
    tablaTokens->setRowCount(0);

    Token t = sc.nextToken();
    while (t.kind != Token::Kind::END) {
        int row = tablaTokens->rowCount();
        tablaTokens->insertRow(row);
        tablaTokens->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(t.lexeme)));
        if (t.kind == Token::Kind::ERR) {
            tablaTokens->setItem(row, 1, new QTableWidgetItem("-"));
            tablaTokens->setItem(row, 2, new QTableWidgetItem("ERROR: caracter no reconocido"));
        } else {
            tablaTokens->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(t.tokenName)));
            tablaTokens->setItem(row, 2, new QTableWidgetItem("OK"));
        }
        t = sc.nextToken();
    }
}

// ---------------------------------------------------------------
// Pantalla: ayuda
// ---------------------------------------------------------------

QWidget* MainWindow::crearPaginaAyuda() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);

    QHBoxLayout* topBar = new QHBoxLayout;
    QPushButton* btnAtras = new QPushButton("<-");
    connect(btnAtras, &QPushButton::clicked, this, &MainWindow::volver);
    topBar->addWidget(btnAtras);
    topBar->addWidget(new QLabel("Como usar el analizador"), 1);
    layout->addLayout(topBar);

    QLabel* texto = new QLabel(
        "1. Elemento unico agrega una pieza basica: mayusculas, minusculas, "
        "digitos o un simbolo.\n\n"
        "2. OR combina la pieza mas reciente con otra opcion (ej. mayusculas o minusculas).\n\n"
        "3. Repeticion envuelve un elemento para que se repita 0-o-mas o 1-o-mas veces. "
        "Dentro de una repeticion solo puedes usar Elemento unico y OR (no se puede "
        "anidar otra repeticion ni una palabra clave).\n\n"
        "4. Dentro de una repeticion, 'Cerrar repeticion y continuar' guarda lo que "
        "armaste y te deja seguir agregando mas piezas al token (por ejemplo, para "
        "hacer un punto flotante: repeticion de digitos, cerrar, simbolo '.', "
        "otra repeticion de digitos). El boton <- en cambio CANCELA la repeticion "
        "sin guardar nada.\n\n"
        "5. Palabra clave agrega texto literal exacto (solo si el token todavia esta vacio).\n\n"
        "6. Finalizar token guarda la secuencia armada con el nombre que elijas. Si "
        "dejaste una repeticion sin cerrar, se cierra sola con lo que alcanzaste a elegir.\n\n"
        "7. En 'Ver tokens' puedes escribir codigo de prueba y presionar Escanear "
        "para ver los tokens reconocidos, o generar el codigo C++ del analizador.");
    texto->setWordWrap(true);
    layout->addWidget(texto);
    layout->addStretch();
    return page;
}

// ---------------------------------------------------------------
// Generacion de codigo C++ independiente (sin Qt)
// ---------------------------------------------------------------

static QString escaparCpp(const std::string& s) {
    QString r;
    for (unsigned char c : s) {
        switch (c) {
            case '\\': r += "\\\\"; break;
            case '"':  r += "\\\""; break;
            case '\n': r += "\\n"; break;
            case '\t': r += "\\t"; break;
            default:   r += QChar(c);
        }
    }
    return r;
}

static QString cppParaAlternativa(const Alternative& a) {
    QString kindStr;
    switch (a.kind) {
        case Alternative::UPPERCASE: kindStr = "Alternative::UPPERCASE"; break;
        case Alternative::LOWERCASE: kindStr = "Alternative::LOWERCASE"; break;
        case Alternative::DIGIT:     kindStr = "Alternative::DIGIT"; break;
        case Alternative::SYMBOL:    kindStr = "Alternative::SYMBOL"; break;
    }
    return QString("Alternative{%1, \"%2\", \"%3\"}")
        .arg(kindStr, escaparCpp(a.symbolLiteral), escaparCpp(a.symbolName));
}

QString MainWindow::generarBloqueTokenDef(const TokenDef& def) const {
    QString out;
    out += "    {\n        TokenDef def;\n";
    out += QString("        def.name = \"%1\";\n").arg(escaparCpp(def.name));
    if (def.isKeyword) {
        out += "        def.isKeyword = true;\n";
        out += QString("        def.keywordLiteral = \"%1\";\n").arg(escaparCpp(def.keywordLiteral));
    } else {
        for (const auto& piece : def.pieces) {
            out += "        {\n            Piece p;\n";
            out += QString("            p.kind = Piece::%1;\n")
                       .arg(piece.kind == Piece::SINGLE ? "SINGLE" : "REPETITION");
            if (piece.kind == Piece::REPETITION) {
                out += QString("            p.zeroOrMore = %1;\n")
                           .arg(piece.zeroOrMore ? "true" : "false");
            }
            for (const auto& alt : piece.element.alternatives) {
                out += QString("            p.element.alternatives.push_back(%1);\n")
                           .arg(cppParaAlternativa(alt));
            }
            out += "            def.pieces.push_back(p);\n        }\n";
        }
    }
    out += "        defs.push_back(def);\n    }\n";
    return out;
}

QString MainWindow::generarCodigoCpp() const {
    QString cuerpo;
    for (const auto& def : tokenDefs) {
        cuerpo += generarBloqueTokenDef(def);
    }

    QString plantilla = R"CPP(// Analizador lexico generado automaticamente.
// Generado: %1
// No depende de Qt: compila con cualquier compilador C++17, por ejemplo:
//   g++ -std=c++17 analizador_generado.cpp -o analizador
//   ./analizador

#include <string>
#include <vector>
#include <iostream>
#include <cctype>
#include <utility>

struct Alternative {
    enum Kind { UPPERCASE, LOWERCASE, DIGIT, SYMBOL };
    Kind kind = UPPERCASE;
    std::string symbolLiteral;
    std::string symbolName;
};

struct Element {
    std::vector<Alternative> alternatives;
};

struct Piece {
    enum Kind { SINGLE, REPETITION };
    Kind kind = SINGLE;
    Element element;
    bool zeroOrMore = true;
};

struct TokenDef {
    std::string name;
    bool isKeyword = false;
    std::string keywordLiteral;
    std::vector<Piece> pieces;
};

class Token {
public:
    enum class Kind { MATCH, ERR, END };
    Kind kind;
    std::string lexeme;
    std::string tokenName;
    explicit Token(Kind k, std::string lex = "", std::string name = "")
        : kind(k), lexeme(std::move(lex)), tokenName(std::move(name)) {}
};

class Scanner {
public:
    Scanner(const std::string& in, const std::vector<TokenDef>& d)
        : input(in), defs(d), current(0) {}

    Token nextToken() {
        while (current < input.size() && esBlanco(input[current])) current++;
        if (current >= input.size()) return Token(Token::Kind::END);

        size_t first = current;
        int bestLen = -1;
        const TokenDef* bestDef = nullptr;

        for (const auto& def : defs) {
            if (def.isKeyword) continue;
            int len = matchTokenDef(def, first);
            if (len > bestLen) { bestLen = len; bestDef = &def; }
        }

        if (bestLen <= 0) {
            current++;
            return Token(Token::Kind::ERR, std::string(1, input[first]));
        }

        std::string lexeme = input.substr(first, (size_t)bestLen);
        current = first + (size_t)bestLen;

        for (const auto& def : defs) {
            if (def.isKeyword && def.keywordLiteral == lexeme)
                return Token(Token::Kind::MATCH, lexeme, def.name);
        }
        return Token(Token::Kind::MATCH, lexeme, bestDef->name);
    }

private:
    std::string input;
    const std::vector<TokenDef>& defs;
    size_t current;

    static bool esBlanco(char c) { return c==' '||c=='\n'||c=='\r'||c=='\t'; }

    int matchElement(const Element& elem, size_t pos) const {
        if (pos >= input.size()) return -1;
        unsigned char c = (unsigned char)input[pos];
        for (const auto& alt : elem.alternatives) {
            switch (alt.kind) {
                case Alternative::UPPERCASE: if (std::isupper(c)) return 1; break;
                case Alternative::LOWERCASE: if (std::islower(c)) return 1; break;
                case Alternative::DIGIT:     if (std::isdigit(c)) return 1; break;
                case Alternative::SYMBOL: {
                    size_t len = alt.symbolLiteral.size();
                    if (len > 0 && input.compare(pos, len, alt.symbolLiteral) == 0) return (int)len;
                    break;
                }
            }
        }
        return -1;
    }

    int matchPiece(const Piece& piece, size_t pos) const {
        if (piece.kind == Piece::SINGLE) return matchElement(piece.element, pos);
        size_t cur = pos; int count = 0;
        while (true) {
            int len = matchElement(piece.element, cur);
            if (len < 0) break;
            cur += (size_t)len; count++;
        }
        if (!piece.zeroOrMore && count == 0) return -1;
        return (int)(cur - pos);
    }

    int matchTokenDef(const TokenDef& def, size_t pos) const {
        if (def.isKeyword) {
            size_t len = def.keywordLiteral.size();
            if (len > 0 && input.compare(pos, len, def.keywordLiteral) == 0) return (int)len;
            return -1;
        }
        size_t cur = pos;
        for (const auto& piece : def.pieces) {
            int len = matchPiece(piece, cur);
            if (len < 0) return -1;
            cur += (size_t)len;
        }
        if (cur == pos) return -1;
        return (int)(cur - pos);
    }
};

// ---------- Definiciones de token (generadas desde el GUI) ----------
static std::vector<TokenDef> construirDefiniciones() {
    std::vector<TokenDef> defs;
%2
    return defs;
}

int main() {
    std::vector<TokenDef> defs = construirDefiniciones();

    std::cout << "Escribe una cadena para escanear:\n";
    std::string entrada;
    std::getline(std::cin, entrada);

    Scanner sc(entrada, defs);
    Token t = sc.nextToken();
    while (t.kind != Token::Kind::END) {
        if (t.kind == Token::Kind::ERR)
            std::cout << "ERROR\tcaracter='" << t.lexeme << "'\n";
        else
            std::cout << t.tokenName << "\tlexema='" << t.lexeme << "'\n";
        t = sc.nextToken();
    }
    return 0;
}
)CPP";

    return plantilla.arg(QDateTime::currentDateTime().toString(Qt::ISODate), cuerpo);
}

void MainWindow::generarCodigo() {
    if (tokenDefs.empty()) {
        QMessageBox::warning(this, "Sin tokens", "Define al menos un token antes de generar el codigo.");
        return;
    }
    QString ruta = QFileDialog::getSaveFileName(this, "Guardar analizador generado",
                                                 "analizador_generado.cpp", "Codigo C++ (*.cpp)");
    if (ruta.isEmpty()) return;

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "No se pudo guardar el archivo.");
        return;
    }
    QTextStream out(&archivo);
    out << generarCodigoCpp();
    archivo.close();

    QMessageBox::information(this, "Codigo generado",
        "Se genero el archivo:\n" + ruta +
        "\n\nCompila con:\ng++ -std=c++17 \"" + ruta + "\" -o analizador");
}
