#include <iostream>
#include <iomanip>
#include <chrono>
#include "parser.h"
#include "greedy.h"
#include "beam_search.h"
#include "block.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Uso: clp_solver <archivo_BR> [indice_instancia]" << endl;
        cout << "Ejemplo: clp_solver ../data/BR1.txt 0" << endl;
        return 1;
    }

    string filename = argv[1];
    int index = (argc >= 3) ? stoi(argv[2]) : 0;

    try {
        // Parsear instancia
        Instance inst = parseInstance(filename, index);
        
        cout << "\n=== INSTANCIA ===" << endl;
        cout << "Contenedor: " << inst.container.L << " x " 
             << inst.container.W << " x " << inst.container.H << endl;
        cout << "Tipos de caja: " << inst.boxes.size() << endl;
        cout << "Volumen teórico utilizable: " 
             << fixed << setprecision(1) 
             << (inst.totalBoxVolume() / inst.containerVolume() * 100.0) << "%" << endl;
        
        // Generar bloques (K2 del paper)
        double minFillRate = (index < 8) ? 1.0 : 0.98;
        int maxBlocks = 20000;
        
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
        // ALGORITMO GREEDY
        // ===================================================================
        
        cout << "\n=== GREEDY ===" << endl;
        
        Container containerGreedy(inst.container.L, inst.container.W, inst.container.H);
        
        auto startGreedy = chrono::high_resolution_clock::now();
        Solution greedySol = greedySolve(containerGreedy, inst.boxes, blocks);
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
        // ALGORITMO BEAM SEARCH
        // ===================================================================
        
        cout << "\n=== BEAM SEARCH ===" << endl;
        
        Container containerBeam(inst.container.L, inst.container.W, inst.container.H);
        
        // Probar con diferentes anchos de beam
        vector<int> beamWidths = {3, 5, 10, 15};
        Solution bestBeamSol = greedySol; // Backup
        double bestBeamUtil = greedyUtil;
        
        for (int w : beamWidths) {
            auto startBeam = chrono::high_resolution_clock::now();
            Solution beamSol = beamSearch(containerBeam, inst.boxes, blocks, w, 30);
            auto endBeam = chrono::high_resolution_clock::now();
            double timeBeam = chrono::duration<double>(endBeam - startBeam).count();
            
            double beamUtil = (beamSol.volumeUtilization() / inst.containerVolume()) * 100.0;
            cout << "\n  Beam Width=" << w << ":" << endl;
            cout << "    Utilización: " << fixed << setprecision(2) << beamUtil << "%";
            
            if (beamUtil > bestBeamUtil) {
                cout << " ★ MEJOR";
                bestBeamSol = beamSol;
                bestBeamUtil = beamUtil;
            }
            cout << endl;
            cout << "    Cajas: " << beamSol.placedBoxes.size() 
                 << " | Tiempo: " << fixed << setprecision(3) << timeBeam << "s" << endl;
        }
        
        // ===================================================================
        // COMPARACIÓN FINAL
        // ===================================================================
        
        cout << "\n=== COMPARACIÓN ===" << endl;
        cout << "Greedy:       " << fixed << setprecision(2) << greedyUtil << "%" << endl;
        cout << "Beam Search:  " << fixed << setprecision(2) << bestBeamUtil << "%" << endl;
        cout << "Teórico:      " << fixed << setprecision(2) 
             << (inst.totalBoxVolume() / inst.containerVolume() * 100.0) << "%" << endl;
        
        double improvement = ((bestBeamUtil - greedyUtil) / greedyUtil) * 100.0;
        cout << "\nMejoría Beam Search: " << fixed << setprecision(2) << improvement << "%" << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}