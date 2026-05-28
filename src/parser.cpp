#include "parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>

using namespace std;

// ---------------------------------------------------------------------------
// Formato de archivos BR (OR-Library):
//
// <número de problemas P>
// Para cada problema:
//   <número de problema> <semilla>
//   <L> <W> <H>              <- dimensiones del contenedor
//   <n>                      <- número de tipos de caja
//   <tipo> <l> <ol> <w> <ow> <h> <oh> <cantidad>   <- una línea por tipo
//
// ol/ow/oh: 1 = orientación vertical permitida en ese eje, 0 = no permitida
// ---------------------------------------------------------------------------

static vector<Instance> parseFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open())
        throw runtime_error("No se pudo abrir el archivo: " + filename);

    vector<Instance> instances;
    int totalProblems;
    file >> totalProblems;

    for (int p = 0; p < totalProblems; p++) {
        Instance inst;

        // Número de problema y semilla
        file >> inst.problemNumber >> inst.seed;

        // Dimensiones del contenedor
        file >> inst.container.L >> inst.container.W >> inst.container.H;

        // Número de tipos de caja
        int n;
        file >> n;

        inst.boxes.resize(n);
        for (int i = 0; i < n; i++) {
            Box& b = inst.boxes[i];
            int ol, ow, oh;
            file >> b.id >> b.l >> ol >> b.w >> ow >> b.h >> oh >> b.quantity;
            b.ol = (ol == 1);
            b.ow = (ow == 1);
            b.oh = (oh == 1);
        }

        instances.push_back(inst);
    }

    return instances;
}

Instance parseInstance(const string& filename, int instanceIndex) {
    auto all = parseFile(filename);
    if (instanceIndex < 0 || instanceIndex >= (int)all.size())
        throw out_of_range(
            "Índice " + to_string(instanceIndex) +
            " fuera de rango (archivo tiene " +
            to_string(all.size()) + " instancias)");
    return all[instanceIndex];
}

vector<Instance> parseAllInstances(const string& filename) {
    return parseFile(filename);
}

double Instance::totalBoxVolume() const {
    double total = 0;
    for (const auto& b : boxes)
        total += (double)b.l * b.w * b.h * b.quantity;
    return total;
}

double Instance::containerVolume() const {
    return (double)container.L * container.W * container.H;
}

void printInstance(const Instance& inst) {
    cout << "=== Instancia #" << inst.problemNumber
              << " (semilla: " << inst.seed << ") ===" << endl;
    cout << "Contenedor: "
              << inst.container.L << " x "
              << inst.container.W << " x "
              << inst.container.H
              << "  (vol=" << inst.containerVolume() << ")" << endl;
    cout << "Tipos de caja: " << inst.boxes.size() << endl;

    for (const auto& b : inst.boxes) {
        cout << "  Tipo " << setw(3) << b.id
                  << ": " << b.l << "x" << b.w << "x" << b.h
                  << "  ori=(" << b.ol << b.ow << b.oh << ")"
                  << "  qty=" << b.quantity << endl;
    }

    double ratio = inst.totalBoxVolume() / inst.containerVolume() * 100.0;
    cout << "Volumen cajas / contenedor: "
              << fixed << setprecision(1) << ratio << "%" << endl;
}