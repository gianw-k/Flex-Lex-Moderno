cmake_minimum_required(VERSION 3.16)
project(LexerGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Busca Qt6 primero; si no está, usa Qt5 (para que compile en cualquier
# computadora sin importar cuál versión de Qt tenga instalada)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(LexerGUI
    main.cpp
    mainwindow.cpp
    mainwindow.h
    piece.cpp
    piece.h
    scanner.cpp
    scanner.h
    token.h
)

target_link_libraries(LexerGUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
