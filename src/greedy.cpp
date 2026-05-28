#include "box.h"
#include "container.h"
#include "block.h"
#include <vector>
#include <algorithm>
using namespace std;

// Estructura para la solución
struct Solution {
    vector<PlacedBox> placedBoxes;
    double volumeUtilization() const;
};

// Implementación de Solution::volumeUtilization
double Solution::volumeUtilization() const {
    double total = 0;
    for (const auto& p : placedBoxes)
        total += (double)p.l * p.w * p.h;
    return total;
}

// ============================================================================
// ALGORITMO GREEDY - Paper: Araya & Riff 2014
// ============================================================================
// El Greedy funciona así:
// 1. Mientras hay cajas por colocar Y hay espacio libre:
//    a) Selecciona el espacio libre r con menor distancia Manhattan (K3)
//    b) Encuentra el bloque b que maximiza f(b,r) = V(b) - Vloss(b,r) (K4)
//    c) Coloca el bloque b en r
//    d) Actualiza la lista de cajas restantes
// 2. Retorna la solución acumulada
// ============================================================================

Solution greedySolve(Container& container, 
                     vector<Box>& boxes, 
                     vector<Block>& blocks) {
    
    Solution solution;
    vector<Box> remainingBoxes = boxes;
    
    int iterations = 0;
    const int maxIterations = 10000;
    
    while (iterations < maxIterations) {
        iterations++;
        
        // Paso 1: Seleccionar espacio libre con menor distancia Manhattan
        int spaceIdx = container.selectFreeSpace();
        if (spaceIdx == -1) break; // No hay más espacios libres
        
        Cuboid selectedSpace = container.freeSpaces[spaceIdx];
        
        // Paso 2: Encontrar el bloque que se ajuste mejor en este espacio
        //         maximizando la función f(b,r) del paper
        double bestScore = -1e9;
        int bestBlockIdx = -1;
        Block bestBlock;
        
        // Generar bloques dinámicamente adaptados al espacio actual
        int limitedMaxBlocks = min(5000, (int)blocks.size() + 1000);
        vector<Block> limitedBlocks(blocks.begin(), blocks.begin() + min((int)blocks.size(), limitedMaxBlocks));
        
        for (size_t i = 0; i < limitedBlocks.size(); i++) {
            const Block& blk = limitedBlocks[i];
            
            // Verificar que el bloque cabe en el espacio
            if (!blk.fitsIn(selectedSpace))
                continue;
            
            // Verificar que todas las cajas del bloque están disponibles
            vector<int> usage(boxes.size() + 1, 0);
            bool feasible = true;
            for (const auto& pb : blk.boxes) {
                usage[pb.boxId]++;
            }
            
            for (size_t j = 0; j < remainingBoxes.size(); j++) {
                if (usage[remainingBoxes[j].id] > remainingBoxes[j].quantity) {
                    feasible = false;
                    break;
                }
            }
            if (!feasible)
                continue;
            
            // Calcular score usando función f(b,r) del paper
            double score = evaluateBlock(blk, selectedSpace, remainingBoxes);
            
            if (score > bestScore) {
                bestScore = score;
                bestBlockIdx = (int)i;
                bestBlock = blk;
            }
        }
        
        // Paso 3: Si no hay bloque válido, no podemos continuar
        if (bestBlockIdx == -1)
            break;
        
        // Paso 4: Colocar el bloque en el contenedor
        // Convertir coordenadas relativas a absolutas
        auto absoluteBoxes = bestBlock.toAbsolute(selectedSpace.x, selectedSpace.y, selectedSpace.z);
        
        // Actualizar container
        container.placeBlock(selectedSpace, bestBlock.l, bestBlock.w, bestBlock.h, absoluteBoxes);
        
        // Paso 5: Actualizar cajas restantes
        for (const auto& pb : bestBlock.boxes) {
            for (size_t j = 0; j < remainingBoxes.size(); j++) {
                if (remainingBoxes[j].id == pb.boxId) {
                    remainingBoxes[j].quantity--;
                    break;
                }
            }
        }
        
        // Agregar las cajas colocadas a la solución
        for (const auto& pb : absoluteBoxes) {
            solution.placedBoxes.push_back(pb);
        }
    }
    
    return solution;
}