#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>
#include "parser.h"
#include "greedy.h"
#include "beam_search.h"
#include "block.h"

using namespace std;

// Búsqueda local: elimina un bloque (conjunto de cajas) y vuelve a empaquetar con greedy
Solution localSearch(const Solution& initial,
                     const Container& containerDims,
                     const vector<Box>& originalBoxes,
                     vector<Block>& blocks,          // ← quitar const
                     int maxIterations,
                     double timeLimitSeconds) {
    Solution best = initial;
    double bestUtil = best.volumeUtilization();
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    auto startTime = chrono::steady_clock::now();

    for (int iter = 0; iter < maxIterations; ++iter) {
        auto elapsed = chrono::duration<double>(chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeLimitSeconds) break;

        if (best.placedBoxes.empty()) break;

        // Elegir un bloque al azar (grupo de cajas que podrían formar un bloque)
        // Simplificación: elegir una caja al azar y considerar como "bloque" solo esa caja.
        int idx = uniform_int_distribution<int>(0, (int)best.placedBoxes.size() - 1)(rng);
        PlacedBox removed = best.placedBoxes[idx];

        // Reconstruir las cajas restantes (partir de las originales y eliminar las colocadas,
        // excepto la que quitamos)
        vector<Box> remainingBoxes = originalBoxes;
        for (const auto& pb : best.placedBoxes) {
            if (&pb == &removed) continue; // omitir la que eliminamos
            for (auto& rb : remainingBoxes) {
                if (rb.id == pb.boxId && rb.quantity > 0) {
                    rb.quantity--;
                    break;
                }
            }
        }

        // Crear un contenedor vacío y ejecutar greedy con las cajas restantes
        Container tempContainer(containerDims.L, containerDims.W, containerDims.H);
        KnapsackCache cache;  // nuevo cache para esta ejecución
        Solution tempSol = greedySolve(tempContainer, remainingBoxes, blocks, cache);

        // Volver a añadir la caja que quitamos (al final, para no complicar)
        // Esto es una simplificación; en realidad deberíamos intentar recolocarla mejor.
        // Para no complicar, simplemente comparamos la solución obtenida.
        double newUtil = tempSol.volumeUtilization();
        if (newUtil > bestUtil) {
            best = tempSol;
            bestUtil = newUtil;
        }
    }
    return best;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Uso: clp_solver <archivo_BR> [indice_instancia]" << endl;
        cout << "Ejemplo: clp_solver ../data/BR1.txt 0" << endl;
        return 1;
    }

    string filename = argv[1];
    int index = (argc >= 3) ? stoi(argv[2]) : 0;

    try {
       Instance inst = parseInstance(filename, index);
        
cout << "\n=== INSTANCIA ===" << endl;
cout << "Contenedor: " << inst.container.L << " x " 
     << inst.container.W << " x " << inst.container.H << endl;
cout << "Tipos de caja: " << inst.boxes.size() << endl;
cout << "Volumen teórico utilizable: " 
     << fixed << setprecision(1) 
     << (inst.totalBoxVolume() / inst.containerVolume() * 100.0) << "%" << endl;

// =====================================================
// Parámetros adaptativos según el grupo BR
// =====================================================
string filename = argv[1];
int setNumber = 0;
size_t pos = filename.find("BR");
if (pos != string::npos) {
    setNumber = stoi(filename.substr(pos+2));
}
bool stronglyHeterogeneous = (setNumber >= 8);

// Valores por defecto para BR0-BR7 (débilmente heterogéneos)
int maxBlocks;
double minFillRate;      // solo bloques simples
int maxBeamWidth;         // ancho máximo del haz
int totalTimeLimit;       // segundos por instancia

if (stronglyHeterogeneous) {
    maxBlocks = 8000;          // menos bloques para acelerar
    minFillRate = 0.98;        // bloques más densos (casi llenos)
    totalTimeLimit = 120;      // más tiempo (ajústalo según tu hardware)
    maxBeamWidth = 15;         // ancho de haz pequeño
}else{
    maxBlocks = 20000;         // más bloques para aprovechar la homogeneidad
    minFillRate = 1.0;        // solo bloques simples (100% llenos)
    totalTimeLimit = 30;      // tiempo moderado
    maxBeamWidth = 20;        // FIX: w>20 con 20000 bloques agota el tiempo sin completar una iteracion
}

// Opcional: permitir pasar el tiempo por línea de comandos
if (argc >= 4) {
    totalTimeLimit = stoi(argv[3]);
}

// Generar bloques
auto startGen = chrono::high_resolution_clock::now();
vector<Block> blocks = generateBlocks(inst.boxes, 
                                      inst.container.L, 
                                      inst.container.W, 
                                      inst.container.H,
                                      maxBlocks,
                                      minFillRate);
auto endGen = chrono::high_resolution_clock::now();
double timeGen = chrono::duration<double>(endGen - startGen).count();

cout << "\nBloques generados: " << blocks.size() 
     << " (tiempo: " << fixed << setprecision(3) << timeGen << "s)" << endl;
        
        // ===================================================================
        // ALGORITMO GREEDY (con cache dummy)
        // ===================================================================
        cout << "\n=== GREEDY ===" << endl;
        Container containerGreedy(inst.container.L, inst.container.W, inst.container.H);
        KnapsackCache dummyCache;
        auto startGreedy = chrono::high_resolution_clock::now();
        Solution greedySol = greedySolve(containerGreedy, inst.boxes, blocks, dummyCache);
        auto endGreedy = chrono::high_resolution_clock::now();
        double timeGreedy = chrono::duration<double>(endGreedy - startGreedy).count();
        
        double greedyUtil = (greedySol.volumeUtilization() / inst.containerVolume()) * 100.0;
        cout << "Cajas colocadas: " << greedySol.placedBoxes.size() << endl;
        cout << "Utilización: " << fixed << setprecision(2) << greedyUtil << "%" << endl;
        cout << "Volumen: " << fixed << setprecision(0) 
             << greedySol.volumeUtilization() << " / " 
             << inst.containerVolume() << endl;
        cout << "Tiempo: " << fixed << setprecision(3) << timeGreedy << "s" << endl;
        
        // ===================================================================
        // BEAM SEARCH CON DOBLE ESFUERZO
        // ===================================================================
        cout << "\n=== BEAM SEARCH (Double Effort) ===" << endl;
        
        Solution bestBeamSol = greedySol;
        double bestBeamUtil = greedyUtil;
        int w = 1;
        auto startTotal = chrono::steady_clock::now();
        int iteration = 0;
        
        while (true) {
            auto elapsed = chrono::duration<double>(chrono::steady_clock::now() - startTotal).count();
            if (elapsed >= totalTimeLimit) break;
            
            int remainingTime = totalTimeLimit - (int)elapsed;
            if (remainingTime <= 0) break;
            
            Container containerBeam(inst.container.L, inst.container.W, inst.container.H);
            auto startBeam = chrono::high_resolution_clock::now();
            int currentW = min(w, maxBeamWidth);
            Solution beamSol = beamSearch(containerBeam, inst.boxes, blocks, currentW, remainingTime, stronglyHeterogeneous);
            auto endBeam = chrono::high_resolution_clock::now();
            double timeBeam = chrono::duration<double>(endBeam - startBeam).count();
            
            double beamUtil = (beamSol.volumeUtilization() / inst.containerVolume()) * 100.0;
            cout << "  w=" << w << " → " << fixed << setprecision(2) << beamUtil 
                 << "%  (tiempo: " << timeBeam << "s)" << endl;
            
            if (beamUtil > bestBeamUtil) {
                bestBeamUtil = beamUtil;
                bestBeamSol = beamSol;
                cout << "    ★ NUEVA MEJOR" << endl;
            }
            
            // Duplicar esfuerzo según el paper: w = ceil(sqrt(2) * w)
            w = max(2, (int)ceil(sqrt(2.0) * w));
            if (w > maxBeamWidth * 2) break;
            iteration++;
            if (iteration > 20) break; // seguridad
        }
        
        // ===================================================================
        // BÚSQUEDA LOCAL POST-BEAM (mejora propia)
        // ===================================================================
        cout << "\n=== BÚSQUEDA LOCAL POST-BEAM ===" << endl;
        auto startLocal = chrono::steady_clock::now();
        double localTime = min(5.0, totalTimeLimit - 
            chrono::duration<double>(chrono::steady_clock::now() - startTotal).count());
        if (localTime > 0) {
            Container containerDummy(inst.container.L, inst.container.W, inst.container.H);
            Solution finalSol = localSearch(bestBeamSol, containerDummy, inst.boxes, blocks, 100, localTime);
            double finalUtil = (finalSol.volumeUtilization() / inst.containerVolume()) * 100.0;
            auto endLocal = chrono::steady_clock::now();
            double timeLocal = chrono::duration<double>(endLocal - startLocal).count();
            cout << "Utilización final: " << fixed << setprecision(2) << finalUtil << "%" << endl;
            cout << "Tiempo local: " << fixed << setprecision(3) << timeLocal << "s" << endl;
            if (finalUtil > bestBeamUtil) {
                bestBeamUtil = finalUtil;
                bestBeamSol = finalSol;
                cout << "  ★ MEJORADA POR BÚSQUEDA LOCAL" << endl;
            }
        }
        
        // ===================================================================
        // COMPARACIÓN FINAL
        // ===================================================================
        cout << "\n=== COMPARACIÓN FINAL ===" << endl;
        cout << "Greedy:            " << fixed << setprecision(2) << greedyUtil << "%" << endl;
        cout << "Beam Search:       " << fixed << setprecision(2) << bestBeamUtil << "%" << endl;
        cout << "Teórico máximo:    " << fixed << setprecision(2) 
             << (inst.totalBoxVolume() / inst.containerVolume() * 100.0) << "%" << endl;
        cout << "Conjunto BR: " << setNumber << " - Heterogéneo: " << stronglyHeterogeneous << endl;
        cout << "maxBlocks: " << maxBlocks << ", minFillRate: " << minFillRate 
        << ", timeLimit: " << totalTimeLimit << ", maxBeamWidth: " << maxBeamWidth << endl;
        
        double improvement = (bestBeamUtil - greedyUtil) / greedyUtil * 100.0;
        cout << "\nMejoría Beam Search sobre Greedy: " << fixed << setprecision(2) << improvement << "%" << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}