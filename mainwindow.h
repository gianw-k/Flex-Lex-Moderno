#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <vector>
#include "piece.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    enum Pagina {
        P_INICIO = 0,
        P_PIEZA,
        P_ELEMENTO,
        P_REPETICION,
        P_KEYWORD,
        P_TOKENS,
        P_AYUDA
    };

    QStackedWidget* stack;

    // widgets de la pantalla "seleccionar tipo de pieza"
    QLabel* lblHeaderPieza;
    QLabel* lblPreview;
    QPushButton* btnElementoUnico;
    QPushButton* btnRepeticion;
    QPushButton* btnOr;
    QPushButton* btnPalabraClave;

    // widgets de la pantalla de tokens / prueba
    QListWidget* listaDefiniciones;
    QTableWidget* tablaTokens;
    QTextEdit* txtCodigoFuente;

    // estado del token que se esta construyendo
    std::vector<Piece> currentPieces;
    Element currentElement;
    bool hasCurrentElement = false;
    bool buildingRepetition = false;
    bool repetitionZeroOrMore = true;

    std::vector<TokenDef> tokenDefs;

    QWidget* crearPaginaInicio();
    QWidget* crearPaginaPieza();
    QWidget* crearPaginaElemento();
    QWidget* crearPaginaRepeticion();
    QWidget* crearPaginaKeyword();
    QWidget* crearPaginaTokens();
    QWidget* crearPaginaAyuda();

    void refrescarPantallaPieza();
    void confirmarElementoPendiente();
    void agregarAlternativa(Alternative::Kind kind,
                             const std::string& lit = "",
                             const std::string& nombre = "");
    void iniciarRepeticion(bool ceroOMas);
    void cerrarRepeticion();
    void finalizarToken();
    void plantillaIdentificador();
    void plantillaFlotante();
    void refrescarListaTokens();
    void escanear();

    void irA(Pagina p);
    void volver();

    bool nombreDisponible(const std::string& nombre);
};
