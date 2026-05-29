#include "greedy.h"
#include "block.h"
#include <algorithm>
#include <unordered_map>

double Solution::volumeUtilization() const {
    double total = 0;
    for (const auto& p : placedBoxes)
        total += (double)p.l * p.w * p.h;
    return total;
}

Solution greedySolve(Container& container,
                     vector<Box>& boxes,
                     vector<Block>& blocks,
                     KnapsackCache& cache) {
    Solution solution;
    vector<Box> remainingBoxes = boxes;

    // FIX: mapa de disponibilidad O(1) en vez de búsqueda lineal O(n) por iteración
    unordered_map<int, int> available; // boxId → quantity
    for (const auto& b : remainingBoxes)
        available[b.id] = b.quantity;

    while (true) {
        int spaceIdx = container.selectFreeSpace();
        if (spaceIdx == -1) break;

        Cuboid selectedSpace = container.freeSpaces[spaceIdx];

        double bestScore = -1e9;
        int bestBlockIdx = -1;
        const Block* bestBlock = nullptr;

        for (int i = 0; i < (int)blocks.size(); i++) {
            const Block& blk = blocks[i];
            if (!blk.fitsIn(selectedSpace)) continue;

            // Verificar factibilidad con el mapa O(1) por caja del bloque
            bool feasible = true;
            // Contar cuántas de cada tipo necesita este bloque
            unordered_map<int, int> needed;
            for (const auto& pb : blk.boxes)
                needed[pb.boxId]++;
            for (const auto& [id, qty] : needed) {
                auto it = available.find(id);
                if (it == available.end() || it->second < qty) {
                    feasible = false;
                    break;
                }
            }
            if (!feasible) continue;

            double score = evaluateBlock(blk, selectedSpace, remainingBoxes, cache);
            if (score > bestScore) {
                bestScore = score;
                bestBlockIdx = i;
                bestBlock = &blk;
            }
        }

        if (bestBlockIdx == -1) break;

        auto absoluteBoxes = bestBlock->toAbsolute(
            selectedSpace.x, selectedSpace.y, selectedSpace.z);
        container.placeBlock(selectedSpace, bestBlock->l, bestBlock->w,
                             bestBlock->h, absoluteBoxes);

        // Actualizar disponibilidad
        for (const auto& pb : bestBlock->boxes) {
            available[pb.boxId]--;
        }

        // Sincronizar remainingBoxes (necesario para evaluateBlock)
        for (const auto& pb : bestBlock->boxes) {
            for (auto& rb : remainingBoxes) {
                if (rb.id == pb.boxId) { rb.quantity--; break; }
            }
        }

        for (const auto& pb : absoluteBoxes)
            solution.placedBoxes.push_back(pb);
    }
    return solution;
}