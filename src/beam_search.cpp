#include "box.h"
#include "container.h"
#include "block.h"
#include "greedy.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <ctime>
#include <cmath>
using namespace std;

struct State {
    Container container;
    vector<Box> remainingBoxes;
    double volumeUsed;
    int blocksPlaced;
    
    // Compara estados por volumen utilizado (orden descendente para priority_queue)
    bool operator<(const State& other) const {
        return volumeUsed < other.volumeUsed; // min-heap -> invertimos para max-heap
    }
};

// ============================================================================
// BEAM SEARCH - Paper: Araya & Riff 2014
// ============================================================================
// Algoritmo:
// 1. Inicializa un conjunto de K estados (inicialmente solo el estado vacío)
// 2. Para cada iteración:
//    a) Para cada estado en el beam, expande los mejores bloques disponibles
//    b) Selecciona los K mejores nuevos estados
// 3. Retorna el estado con máximo volumen utilizado
// ============================================================================

// Ancho adaptivo: puede aumentar el beam width si hay mucha diversidad
int adaptiveBeamWidth(vector<State>& states, int wMin, int wMax) {
    if ((int)states.size() < wMin) return wMin;
    
    // Calcular varianza de volumeUsed para medir diversidad
    double mean = 0;
    for (const auto& s : states)
        mean += s.volumeUsed;
    mean /= states.size();
    
    double variance = 0;
    for (const auto& s : states)
        variance += (s.volumeUsed - mean) * (s.volumeUsed - mean);
    variance /= states.size();
    
    double stdDev = sqrt(variance);
    
    // Mayor varianza = más diversidad = aumentar ancho
    // Si stdDev > 5% del promedio, aumentar ancho
    if (stdDev > 0.05 * mean)
        return min(wMax, wMin * 2);
    return wMin;
}

Solution beamSearch(Container& initialContainer,
                    vector<Box>& boxes,
                    vector<Block>& blocks,
                    int beamWidth,
                    int timeLimitSeconds) {
    
    auto startTime = time(nullptr);
    
    // Estado inicial: contenedor vacío
    State initialState{initialContainer, boxes, 0.0, 0};
    
    // Cola de prioridad: mantiene los K mejores estados
    priority_queue<State> currentLevel;
    currentLevel.push(initialState);
    
    State bestState = initialState;
    int iteration = 0;
    const int maxIterations = 100;
    
    while (!currentLevel.empty() && iteration < maxIterations) {
        iteration++;
        
        // Extraer los estados actuales del nivel
        vector<State> statesThisLevel;
        while (!currentLevel.empty()) {
            statesThisLevel.push_back(currentLevel.top());
            currentLevel.pop();
        }
        
        // Generar nuevos estados expandiendo cada uno
        priority_queue<State> nextLevel;
        
        for (const auto& parentState : statesThisLevel) {
            // Seleccionar espacio libre
            Container tempContainer = parentState.container;
            int spaceIdx = tempContainer.selectFreeSpace();
            if (spaceIdx == -1) {
                // No hay más espacio, este estado es terminal
                if (parentState.volumeUsed > bestState.volumeUsed)
                    bestState = parentState;
                nextLevel.push(parentState);
                continue;
            }
            
            Cuboid selectedSpace = tempContainer.freeSpaces[spaceIdx];
            
            // Encontrar los K mejores bloques que caben
            vector<pair<double, int>> blockScores; // (score, index)
            
            for (size_t i = 0; i < blocks.size(); i++) {
                const Block& blk = blocks[i];
                
                if (!blk.fitsIn(selectedSpace))
                    continue;
                
                // Verificar disponibilidad de cajas
                vector<int> usage(boxes.size() + 1, 0);
                bool feasible = true;
                for (const auto& pb : blk.boxes) {
                    usage[pb.boxId]++;
                }
                
                for (size_t j = 0; j < parentState.remainingBoxes.size(); j++) {
                    if (usage[parentState.remainingBoxes[j].id] > parentState.remainingBoxes[j].quantity) {
                        feasible = false;
                        break;
                    }
                }
                if (!feasible)
                    continue;
                
                double score = evaluateBlock(blk, selectedSpace, parentState.remainingBoxes);
                blockScores.push_back({score, (int)i});
            }
            
            // Ordenar bloques por score (descendente)
            sort(blockScores.begin(), blockScores.end(), 
                 [](const auto& a, const auto& b) { return a.first > b.first; });
            
            // Expandir los Top-K bloques
            int expansionWidth = min(beamWidth, (int)blockScores.size());
            if (expansionWidth == 0) {
                // No hay bloques válidos
                if (parentState.volumeUsed > bestState.volumeUsed)
                    bestState = parentState;
                nextLevel.push(parentState);
                continue;
            }
            
            for (int e = 0; e < expansionWidth; e++) {
                int blockIdx = blockScores[e].second;
                const Block& blk = blocks[blockIdx];
                
                // Crear nuevo estado
                State newState = parentState;
                
                // Colocar bloque
                auto absoluteBoxes = blk.toAbsolute(selectedSpace.x, selectedSpace.y, selectedSpace.z);
                newState.container.placeBlock(selectedSpace, blk.l, blk.w, blk.h, absoluteBoxes);
                newState.volumeUsed += (double)blk.usedVolume;
                newState.blocksPlaced++;
                
                // Actualizar cajas restantes
                for (const auto& pb : blk.boxes) {
                    for (size_t j = 0; j < newState.remainingBoxes.size(); j++) {
                        if (newState.remainingBoxes[j].id == pb.boxId) {
                            newState.remainingBoxes[j].quantity--;
                            break;
                        }
                    }
                }
                
                // Registrar si es mejor
                if (newState.volumeUsed > bestState.volumeUsed)
                    bestState = newState;
                
                nextLevel.push(newState);
            }
        }
        
        // Seleccionar los K mejores estados para la siguiente iteración
        vector<State> allNext;
        while (!nextLevel.empty()) {
            allNext.push_back(nextLevel.top());
            nextLevel.pop();
        }
        
        int keepWidth = adaptiveBeamWidth(allNext, beamWidth, beamWidth * 2);
        keepWidth = min(keepWidth, (int)allNext.size());
        
        for (int i = 0; i < keepWidth; i++) {
            currentLevel.push(allNext[i]);
        }
        
        // Verificar límite de tiempo
        time_t currentTime = time(nullptr);
        if (difftime(currentTime, startTime) > timeLimitSeconds)
            break;
    }
    
    // Construir solución final
    Solution solution;
    for (const auto& pb : bestState.container.placed) {
        solution.placedBoxes.push_back(pb);
    }
    
    return solution;
}