#pragma once
#include "box.h"
#include "container.h"
#include <unordered_map>
#include <vector>

using namespace std;

// Cache para knapsack: key = (sig_cajas, capacidad), value = mejor valor
// La key combina firma de cajas Y capacidad para evitar colisiones entre
// espacios de distinto tamaño con las mismas cajas disponibles.
struct KnapsackCache {
    // key = sig ^ (cap * 2654435761ULL) para combinar ambos en un hash
    unordered_map<size_t, int> cacheLength;
    unordered_map<size_t, int> cacheWidth;
    unordered_map<size_t, int> cacheHeight;
};

size_t boxesSignature(const vector<Box>& boxes);

struct Block {
    int l, w, h;
    vector<PlacedBox> boxes;
    int usedVolume;

    double fillRate() const {
        int total = l * w * h;
        return total > 0 ? (double)usedVolume / total : 0.0;
    }

    bool fitsIn(const Cuboid& r) const {
        return l <= r.l && w <= r.w && h <= r.h;
    }

    vector<PlacedBox> toAbsolute(int ox, int oy, int oz) const;
};

vector<Block> generateSimpleBlocks(const vector<Box>& boxes, int L, int W, int H);
bool combineBlocks(const Block& b1, const Block& b2, double minFillRate, Block& result);
vector<Block> generateBlocks(const vector<Box>& boxes, int L, int W, int H,
                              int maxBlocks, double minFillRate);
double evaluateBlock(const Block& b, const Cuboid& r,
                     const vector<Box>& remaining, KnapsackCache& cache);