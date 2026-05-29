#include "box.h"
#include "container.h"
#include "beam_search.h"
#include "block.h"
#include "greedy.h"
#include <vector>
#include <algorithm>
#include <ctime>
#include <cmath>
using namespace std;

// Implementación del operador < (puede estar aquí o inline en el .h)
bool State::operator<(const State& other) const {
    return greedyScore < other.greedyScore;
}

int adaptiveBeamWidth(const vector<State>& states, int wMin, int wMax) {
    if ((int)states.size() <= wMin) return wMin;
    double mean = 0;
    for (const auto& s : states) mean += s.greedyScore;
    mean /= states.size();
    double var = 0;
    for (const auto& s : states) var += (s.greedyScore - mean) * (s.greedyScore - mean);
    var /= states.size();
    double cv = (mean > 0) ? sqrt(var) / mean : 0;
    if (cv > 0.02) return min(wMax, (int)(wMin * 1.5));
    return wMin;
}

void removeSimilarStates(vector<State>& states) {
    vector<bool> keep(states.size(), true);
    for (size_t i = 0; i < states.size(); ++i) {
        if (!keep[i]) continue;
        for (size_t j = i+1; j < states.size(); ++j) {
            if (!keep[j]) continue;
            if (states[i].greedySignature == states[j].greedySignature) {
                if (states[i].volumeUsed <= states[j].volumeUsed)
                    keep[j] = false;
                else
                    keep[i] = false;
            }
        }
    }
    vector<State> filtered;
    for (size_t i = 0; i < states.size(); ++i)
        if (keep[i]) filtered.push_back(move(states[i]));
    states = move(filtered);
}

double evaluateState(State& s, const vector<Box>& allBoxes, vector<Block>& blocks) {
    Container tempContainer = s.container;
    vector<Box> tempBoxes = s.remainingBoxes;
    Solution tempSol = greedySolve(tempContainer, tempBoxes, blocks, s.knapCache);
    double volume = s.volumeUsed + tempSol.volumeUtilization();

    // FIX: la signature incluye cajas ya colocadas + las que agrega el greedy.
    // Sin esto, dos estados con distinta historia pero igual completion
    // parecen idénticos y removeSimilarStates elimina estados válidos.
    vector<int> sig(allBoxes.size() + 1, 0);
    for (const auto& pb : s.container.placed) {   // ya colocadas
        if (pb.boxId >= 1 && pb.boxId < (int)sig.size())
            sig[pb.boxId]++;
    }
    for (const auto& pb : tempSol.placedBoxes) {  // greedy-completion
        if (pb.boxId >= 1 && pb.boxId < (int)sig.size())
            sig[pb.boxId]++;
    }
    s.greedySignature = sig;
    return volume;
}

Solution beamSearch(Container& initialContainer,
                    vector<Box>& boxes,
                    vector<Block>& blocks,
                    int beamWidth,
                    int timeLimitSeconds,
                    bool stronglyHeterogeneous) {
    auto startTime = time(nullptr);

    State initialState{initialContainer, boxes, 0.0, 0, 0.0, {}, KnapsackCache()};
    initialState.greedyScore = evaluateState(initialState, boxes, blocks);

    vector<State> currentBeam = {initialState};
    State bestState = initialState;
    int iteration = 0;

    while (iteration < 300) {
        if (difftime(time(nullptr), startTime) > timeLimitSeconds) break;
        if (currentBeam.empty()) break;

        vector<State> allSuccessors;
        bool isRoot = (iteration == 0 && currentBeam.size() == 1);

        for (const auto& parentState : currentBeam) {
            double remainingVolume = 0.0;
            for (const auto& rb : parentState.remainingBoxes) {
                remainingVolume += (double)rb.l * rb.w * rb.h * rb.quantity;
            }
            if (parentState.volumeUsed + remainingVolume <= bestState.volumeUsed) {
                continue; // este estado no puede mejorar la mejor solución
            }

            Container tempContainer = parentState.container;
            int spaceIdx = tempContainer.selectFreeSpace();
            if (spaceIdx == -1) {
                if (parentState.volumeUsed > bestState.volumeUsed)
                    bestState = parentState;
                continue;
            }

            Cuboid selectedSpace = tempContainer.freeSpaces[spaceIdx];

            vector<pair<double, int>> blockScores;
            for (size_t i = 0; i < blocks.size(); ++i) {
                const Block& blk = blocks[i];
                if (!blk.fitsIn(selectedSpace)) continue;

                vector<int> usage(boxes.size()+1, 0);
                bool feasible = true;
                for (const auto& pb : blk.boxes) usage[pb.boxId]++;
                for (const auto& rb : parentState.remainingBoxes) {
                    if (usage[rb.id] > rb.quantity) { feasible = false; break; }
                }
                if (!feasible) continue;

                double score = evaluateBlock(blk, selectedSpace, parentState.remainingBoxes,
                                             parentState.knapCache);
                blockScores.push_back({score, (int)i});
            }

            if (blockScores.empty()) {
                if (parentState.volumeUsed > bestState.volumeUsed)
                    bestState = parentState;
                continue;
            }

            sort(blockScores.begin(), blockScores.end(),
                 [](const auto& a, const auto& b) { return a.first > b.first; });

            int expandCount = isRoot ? min(beamWidth*beamWidth, (int)blockScores.size())
                                     : min(beamWidth, (int)blockScores.size());

            for (int e = 0; e < expandCount; ++e) {
                if (difftime(time(nullptr), startTime) > timeLimitSeconds) break;

                const Block& blk = blocks[blockScores[e].second];
                State newState = parentState;

                auto absBoxes = blk.toAbsolute(selectedSpace.x, selectedSpace.y, selectedSpace.z);
                newState.container.placeBlock(selectedSpace, blk.l, blk.w, blk.h, absBoxes);
                newState.volumeUsed += (double)blk.usedVolume;
                newState.blocksPlaced++;

                for (const auto& pb : blk.boxes) {
                    for (auto& rb : newState.remainingBoxes) {
                        if (rb.id == pb.boxId) {
                            rb.quantity--;
                            break;
                        }
                    }
                }

                newState.greedyScore = evaluateState(newState, boxes, blocks);
                if (newState.volumeUsed > bestState.volumeUsed)
                    bestState = newState;

                allSuccessors.push_back(move(newState));
            }
        }

        if (allSuccessors.empty()) break;

        removeSimilarStates(allSuccessors);

        sort(allSuccessors.begin(), allSuccessors.end(),
             [](const auto& a, const auto& b) { return a.greedyScore > b.greedyScore; });

        int keep;
        if (stronglyHeterogeneous) {
            // Ancho fijo pequeño para instancias heterogéneas
            keep = min(10, (int)allSuccessors.size());
        } else {
            int adaptiveW = adaptiveBeamWidth(allSuccessors, beamWidth, beamWidth*2);
            keep = min(adaptiveW, (int)allSuccessors.size());
        }

        currentBeam.clear();
        for (int i = 0; i < keep; ++i)
            currentBeam.push_back(move(allSuccessors[i]));

        ++iteration;
    }

    Solution solution;
    for (const auto& pb : bestState.container.placed)
        solution.placedBoxes.push_back(pb);
    return solution;
}