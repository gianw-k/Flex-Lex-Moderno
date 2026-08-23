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
#include "generador.h"

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
        P_CODIGO,
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
    QPushButton* btnCerrarRepeticion;
    QPushButton* btnDeshacerPieza;

    // widgets de la pantalla de tokens / prueba
    QListWidget* listaDefiniciones;
    QTableWidget* tablaTokens;
    QTextEdit* txtCodigoFuente;

    // widgets de la pantalla de codigo generado
    QTextEdit* txtCodigoGenerado;
    QLabel* lblInfoDfa;
    std::string codigoGenerado;

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
    QWidget* crearPaginaCodigo();
    QWidget* crearPaginaAyuda();

    void refrescarPantallaPieza();
    void confirmarElementoPendiente();
    void agregarAlternativa(Alternative::Kind kind,
                             const std::string& lit = "",
                             const std::string& nombre = "");
    void iniciarRepeticion(bool ceroOMas);
    void cerrarRepeticion();
    void deshacerUltimaPieza();
    void finalizarToken();
    QString sugerirNombrePorDefecto() const;
    void refrescarListaTokens();
    void escanear();
    void generarCodigo();
    void guardarCodigo();

    void irA(Pagina p);
    void volver();
    bool confirmarSalirSiHayProgreso();

    bool nombreDisponible(const std::string& nombre);
};
