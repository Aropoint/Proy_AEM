#pragma once
#include <vector>
#include <tuple>

using namespace std;
// Representa un tipo de caja con sus dimensiones y restricciones de orientación
struct Box {
    int id;            // tipo de caja (1-indexed, como en los archivos BR)
    int l, w, h;       // dimensiones originales (length, width, height)
    bool ol, ow, oh;   // orientación vertical permitida por eje (0/1 en archivos BR)
    int quantity;      // cantidad de cajas de este tipo disponibles
};

// Una caja ya colocada dentro del contenedor
struct PlacedBox {
    int boxId;
    int x, y, z;       // esquina inferior-izquierda-frontal
    int l, w, h;       // dimensiones en la orientación elegida
};

// Retorna todas las orientaciones válidas de una caja como (l, w, h)
// Respeta las restricciones ol, ow, oh del archivo BR
vector<tuple<int,int,int>> getOrientations(const Box& box);