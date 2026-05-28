#pragma once
#include "box.h"
#include <string>
#include <vector>

using namespace std;
// Dimensiones del contenedor para una instancia
struct ContainerDims {
    int L, W, H;
};

// Una instancia completa del problema
struct Instance {
    int problemNumber;   // número de problema dentro del archivo
    long long seed;      // semilla usada para generar la instancia
    ContainerDims container;
    vector<Box> boxes;

    // Volumen total de todas las cajas disponibles
    double totalBoxVolume() const;
    // Volumen del contenedor
    double containerVolume() const;
};

// Lee UNA instancia específica desde un archivo BR
// instanceIndex: 0-indexed (0 = primera instancia del archivo)
Instance parseInstance(const string& filename, int instanceIndex);

// Lee TODAS las instancias de un archivo BR (normalmente 100)
vector<Instance> parseAllInstances(const string& filename);

// Imprime un resumen de la instancia (útil para debug)
void printInstance(const Instance& inst);