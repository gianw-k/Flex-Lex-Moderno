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
                cerrarRepeticion();
            } else {
                irA(P_INICIO);
            }
            break;
        case P_ELEMENTO:
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
    connect(btnSalir, &QPushButton::clicked, this, []() { QApplication::quit(); });

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

    QPushButton* btnPlantillaId = new QPushButton("Plantilla: Identificador");
    connect(btnPlantillaId, &QPushButton::clicked, this, &MainWindow::plantillaIdentificador);

    QPushButton* btnPlantillaFloat = new QPushButton("Plantilla: Punto flotante");
    connect(btnPlantillaFloat, &QPushButton::clicked, this, &MainWindow::plantillaFlotante);

    QPushButton* btnFinalizar = new QPushButton("Finalizar token");
    btnFinalizar->setStyleSheet("font-weight: bold;");
    connect(btnFinalizar, &QPushButton::clicked, this, &MainWindow::finalizarToken);

    grid->addWidget(btnElementoUnico, 0, 0);
    grid->addWidget(btnRepeticion, 0, 1);
    grid->addWidget(btnOr, 1, 0);
    grid->addWidget(btnPalabraClave, 1, 1);
    grid->addWidget(btnPlantillaId, 2, 0);
    grid->addWidget(btnPlantillaFloat, 2, 1);
    grid->addWidget(btnFinalizar, 3, 0, 1, 2);

    outer->addLayout(grid);
    return page;
}

void MainWindow::refrescarPantallaPieza() {
    if (buildingRepetition) {
        lblHeaderPieza->setText(QString("Construyendo el contenido de la repeticion (%1) - use <- para terminar")
                                     .arg(repetitionZeroOrMore ? "0 o mas" : "1 o mas"));
        btnRepeticion->setVisible(false);
        btnPalabraClave->setVisible(false);
        btnElementoUnico->setVisible(!hasCurrentElement);
    } else {
        lblHeaderPieza->setText("Ingrese un tipo de token");
        btnRepeticion->setVisible(true);
        btnElementoUnico->setVisible(true);
        btnPalabraClave->setVisible(currentPieces.empty() && !hasCurrentElement);
    }
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
// Finalizar token / plantillas
// ---------------------------------------------------------------

void MainWindow::finalizarToken() {
    confirmarElementoPendiente();
    if (buildingRepetition) {
        QMessageBox::warning(this, "Repeticion abierta", "Cierra la repeticion (<-) antes de finalizar.");
        return;
    }
    if (currentPieces.empty()) {
        QMessageBox::warning(this, "Token vacio", "Agrega al menos una pieza antes de finalizar.");
        return;
    }
    bool ok;
    QString nombre = QInputDialog::getText(this, "Nombre del token", "Nombre para este token:",
                                            QLineEdit::Normal, "", &ok);
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

void MainWindow::plantillaIdentificador() {
    currentPieces.clear();
    hasCurrentElement = false;
    buildingRepetition = false;

    Piece p1;
    p1.kind = Piece::SINGLE;
    Alternative a1; a1.kind = Alternative::UPPERCASE;
    Alternative a2; a2.kind = Alternative::LOWERCASE;
    p1.element.alternatives = {a1, a2};
    currentPieces.push_back(p1);

    Piece p2;
    p2.kind = Piece::REPETITION;
    p2.zeroOrMore = true;
    Alternative b1; b1.kind = Alternative::UPPERCASE;
    Alternative b2; b2.kind = Alternative::LOWERCASE;
    Alternative b3; b3.kind = Alternative::DIGIT;
    p2.element.alternatives = {b1, b2, b3};
    currentPieces.push_back(p2);

    bool ok;
    QString nombre = QInputDialog::getText(this, "Nombre del token", "Nombre:",
                                            QLineEdit::Normal, "ID", &ok);
    if (!ok || nombre.trimmed().isEmpty() || !nombreDisponible(nombre.toStdString())) {
        if (ok && !nombreDisponible(nombre.toStdString()))
            QMessageBox::warning(this, "Nombre repetido", "Ya existe un token con ese nombre.");
        currentPieces.clear();
        return;
    }
    TokenDef def;
    def.name = nombre.toStdString();
    def.pieces = currentPieces;
    tokenDefs.push_back(def);
    currentPieces.clear();
    refrescarPantallaPieza();
}

void MainWindow::plantillaFlotante() {
    currentPieces.clear();
    hasCurrentElement = false;
    buildingRepetition = false;

    Piece p1;
    p1.kind = Piece::REPETITION;
    p1.zeroOrMore = false;
    Alternative d1; d1.kind = Alternative::DIGIT;
    p1.element.alternatives = {d1};
    currentPieces.push_back(p1);

    Piece p2;
    p2.kind = Piece::SINGLE;
    Alternative punto; punto.kind = Alternative::SYMBOL; punto.symbolLiteral = "."; punto.symbolName = "punto";
    p2.element.alternatives = {punto};
    currentPieces.push_back(p2);

    Piece p3;
    p3.kind = Piece::REPETITION;
    p3.zeroOrMore = false;
    Alternative d2; d2.kind = Alternative::DIGIT;
    p3.element.alternatives = {d2};
    currentPieces.push_back(p3);

    bool ok;
    QString nombre = QInputDialog::getText(this, "Nombre del token", "Nombre:",
                                            QLineEdit::Normal, "FLOAT", &ok);
    if (!ok || nombre.trimmed().isEmpty() || !nombreDisponible(nombre.toStdString())) {
        if (ok && !nombreDisponible(nombre.toStdString()))
            QMessageBox::warning(this, "Nombre repetido", "Ya existe un token con ese nombre.");
        currentPieces.clear();
        return;
    }
    TokenDef def;
    def.name = nombre.toStdString();
    def.pieces = currentPieces;
    tokenDefs.push_back(def);
    currentPieces.clear();
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

    QPushButton* btnEscanear = new QPushButton("Escanear");
    connect(btnEscanear, &QPushButton::clicked, this, &MainWindow::escanear);
    layout->addWidget(btnEscanear);

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
        "No se puede meter una repeticion dentro de otra.\n\n"
        "4. Palabra clave agrega texto literal exacto (solo si el token todavia esta vacio).\n\n"
        "5. Las plantillas arman Identificador o Punto flotante automaticamente.\n\n"
        "6. Finalizar token guarda la secuencia armada con el nombre que elijas.\n\n"
        "7. En 'Ver tokens' puedes escribir codigo de prueba y presionar Escanear "
        "para ver los tokens reconocidos.");
    texto->setWordWrap(true);
    layout->addWidget(texto);
    layout->addStretch();
    return page;
}
