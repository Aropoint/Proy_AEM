#pragma once
#include "greedy.h"
#include "box.h"
#include "container.h"
#include "block.h"
#include <vector>

using namespace std;

struct State;

// Ancho adaptivo del beam según diversidad de estados
int adaptiveBeamWidth(vector<State>& states, int wMin, int wMax);

// Algoritmo Beam Search del paper Araya & Riff 2014
Solution beamSearch(Container& initialContainer,
                    vector<Box>& boxes,
                    vector<Block>& blocks,
                    int beamWidth,
                    int timeLimitSeconds);
