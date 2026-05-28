#pragma once
#include "box.h"
#include "container.h"
#include "block.h"
#include <vector>

using namespace std;

// Estructura para almacenar la solución
struct Solution {
    vector<PlacedBox> placedBoxes;
    double volumeUtilization() const;
};

// Algoritmo Greedy del paper Araya & Riff 2014
Solution greedySolve(Container& container, 
                     vector<Box>& boxes, 
                     vector<Block>& blocks);
