#include "greedy.h"
#include "block.h"
#include <algorithm>

// Implementación del método volumeUtilization
double Solution::volumeUtilization() const {
    double total = 0;
    for (const auto& p : placedBoxes)
        total += (double)p.l * p.w * p.h;
    return total;
}

// La función greedySolve (sin redefinir Solution)
Solution greedySolve(Container& container, 
                     vector<Box>& boxes, 
                     vector<Block>& blocks,
                     KnapsackCache& cache) {
    Solution solution;
    vector<Box> remainingBoxes = boxes;
    
    int iterations = 0;
    const int maxIterations = 10000;
    
    while (iterations < maxIterations) {
        iterations++;
        int spaceIdx = container.selectFreeSpace();
        if (spaceIdx == -1) break;
        
        Cuboid selectedSpace = container.freeSpaces[spaceIdx];
        
        double bestScore = -1e9;
        int bestBlockIdx = -1;
        Block bestBlock;
        
        for (size_t i = 0; i < blocks.size(); ++i) {
            const Block& blk = blocks[i];
            if (!blk.fitsIn(selectedSpace)) continue;
            
            vector<int> usage(boxes.size()+1, 0);
            bool feasible = true;
            for (const auto& pb : blk.boxes)
                usage[pb.boxId]++;
            for (const auto& rb : remainingBoxes) {
                if (usage[rb.id] > rb.quantity) {
                    feasible = false;
                    break;
                }
            }
            if (!feasible) continue;
            
            double score = evaluateBlock(blk, selectedSpace, remainingBoxes, cache);
            if (score > bestScore) {
                bestScore = score;
                bestBlockIdx = i;
                bestBlock = blk;
            }
        }
        
        if (bestBlockIdx == -1) break;
        
        auto absoluteBoxes = bestBlock.toAbsolute(selectedSpace.x, selectedSpace.y, selectedSpace.z);
        container.placeBlock(selectedSpace, bestBlock.l, bestBlock.w, bestBlock.h, absoluteBoxes);
        
        for (const auto& pb : bestBlock.boxes) {
            for (auto& rb : remainingBoxes) {
                if (rb.id == pb.boxId) {
                    rb.quantity--;
                    break;
                }
            }
        }
        
        for (const auto& pb : absoluteBoxes)
            solution.placedBoxes.push_back(pb);
    }
    return solution;
}