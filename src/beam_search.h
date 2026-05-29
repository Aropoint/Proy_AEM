#pragma once
#include "greedy.h"
#include "box.h"
#include "container.h"
#include "block.h"
#include <vector>

using namespace std;

struct State {
    Container container;
    vector<Box> remainingBoxes;
    double volumeUsed;
    int blocksPlaced;
    double greedyScore;
    vector<int> greedySignature;
    mutable KnapsackCache knapCache;

    bool operator<(const State& other) const;
};

int adaptiveBeamWidth(const vector<State>& states, int wMin, int wMax);
void removeSimilarStates(vector<State>& states);
double evaluateState(State& s, const vector<Box>& allBoxes, vector<Block>& blocks);
Solution beamSearch(Container& initialContainer,
                    vector<Box>& boxes,
                    vector<Block>& blocks,
                    int beamWidth,
                    int timeLimitSeconds, 
                    bool stronglyHeterogeneous);