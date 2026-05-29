#include "block.h"
#include <algorithm>
#include <set>
#include <tuple>
#include <numeric>
#include <functional>
#include <unordered_map>

using namespace std;
// ---------------------------------------------------------------------------
// Block
// ---------------------------------------------------------------------------

// Función para calcular hash de cajas restantes
size_t boxesSignature(const vector<Box>& boxes) {
    size_t seed = 0;
    for (const auto& b : boxes) {
        // combinar id y quantity
        seed ^= hash<int>{}(b.id) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        seed ^= hash<int>{}(b.quantity) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }
    return seed;
}


vector<PlacedBox> Block::toAbsolute(int ox, int oy, int oz) const {
    vector<PlacedBox> result;
    for (const auto& pb : boxes)
        result.push_back({pb.boxId, ox + pb.x, oy + pb.y, oz + pb.z,
                          pb.l, pb.w, pb.h});
    return result;
}

// ---------------------------------------------------------------------------
// Bloques simples
// Un bloque simple consiste en copias de UN solo tipo de caja,
// todas en la misma orientación, apiladas en una grilla 3D.
// ---------------------------------------------------------------------------

vector<Block> generateSimpleBlocks(const vector<Box>& boxes,
                                         int L, int W, int H) {
    vector<Block> result;
    set<tuple<int,int,int>> seen; // evita bloques con mismas dimensiones

    for (const auto& box : boxes) {
        auto orientations = getOrientations(box);

        for (auto& [bl, bw, bh] : orientations) {
            // Cuántas caben en cada eje sin exceder el contenedor ni la cantidad
            int maxX = L / bl;
            int maxY = W / bw;
            int maxZ = H / bh;
            // Asegurar no exceder cantidad disponible
            const int MAX_PER_DIM = 20;  // aumentado para generar más combinaciones simples
            maxX = min(maxX, MAX_PER_DIM);
            maxY = min(maxY, MAX_PER_DIM);
            maxZ = min(maxZ, MAX_PER_DIM);

            for (int nx = 1; nx <= maxX; nx++) {
                for (int ny = 1; ny <= maxY; ny++) {
                    for (int nz = 1; nz <= maxZ; nz++) {
                        // La cantidad total no puede superar las disponibles
                        if (nx * ny * nz > box.quantity) continue;

                        int blockL = nx * bl;
                        int blockW = ny * bw;
                        int blockH = nz * bh;

                        auto key = make_tuple(blockL, blockW, blockH);
                        if (seen.count(key)) continue;
                        seen.insert(key);

                        // Construir las PlacedBox con posiciones relativas
                        Block blk;
                        blk.l = blockL;
                        blk.w = blockW;
                        blk.h = blockH;
                        blk.usedVolume = nx * ny * nz * bl * bw * bh;

                        for (int ix = 0; ix < nx; ix++)
                            for (int iy = 0; iy < ny; iy++)
                                for (int iz = 0; iz < nz; iz++)
                                    blk.boxes.push_back({box.id,
                                        ix * bl, iy * bw, iz * bh,
                                        bl, bw, bh});

                        result.push_back(blk);
                    }
                }
            }
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Combinación de bloques (Fanslau & Bortfeldt, citado en Araya & Riff 2014)
// Intenta combinar b1 y b2 colocándolos en contacto a lo largo de un eje.
// La combinación es válida si el fill rate del nuevo bloque >= minFillRate.
// ---------------------------------------------------------------------------

bool combineBlocks(const Block& b1, const Block& b2,
                   double minFillRate, Block& result) {

    // Intentamos combinar en los 3 ejes: X (largo), Y (ancho), Z (alto)
    struct Attempt { int l, w, h; int axis; };
    vector<Attempt> attempts;

    // Eje X: b1 y b2 deben tener igual W y H
    if (b1.w == b2.w && b1.h == b2.h)
        attempts.push_back({b1.l + b2.l, b1.w, b1.h, 0});

    // Eje Y: b1 y b2 deben tener igual L y H
    if (b1.l == b2.l && b1.h == b2.h)
        attempts.push_back({b1.l, b1.w + b2.w, b1.h, 1});

    // Eje Z: b1 y b2 deben tener igual L y W
    if (b1.l == b2.l && b1.w == b2.w)
        attempts.push_back({b1.l, b1.w, b1.h + b2.h, 2});

    for (auto& att : attempts) {
        int newVol = att.l * att.w * att.h;
        int usedVol = b1.usedVolume + b2.usedVolume;
        double fill = (double)usedVol / newVol;

        if (fill < minFillRate) continue;

        // Construir el bloque combinado
        result.l = att.l;
        result.w = att.w;
        result.h = att.h;
        result.usedVolume = usedVol;

        // Cajas de b1 en su posición original
        for (const auto& pb : b1.boxes)
            result.boxes.push_back(pb);

        // Cajas de b2 desplazadas según el eje de combinación
        for (const auto& pb : b2.boxes) {
            PlacedBox shifted = pb;
            if (att.axis == 0) shifted.x += b1.l;
            if (att.axis == 1) shifted.y += b1.w;
            if (att.axis == 2) shifted.z += b1.h;
            result.boxes.push_back(shifted);
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Generación completa de bloques (K2)
// Parte de bloques simples y combina iterativamente en pares
// hasta alcanzar maxBlocks o no poder generar más.
// ---------------------------------------------------------------------------

vector<Block> generateBlocks(const vector<Box>& boxes,
                                   int L, int W, int H,
                                   int maxBlocks,
                                   double minFillRate) {

    // Paso 1: bloques simples como semilla
    vector<Block> blocks = generateSimpleBlocks(boxes, L, W, H);

    // Paso 2: combinación iterativa por pares
    int prevSize = 0;
    while ((int)blocks.size() < maxBlocks) {
        int currentSize = blocks.size();
        if (currentSize == prevSize) break; // no se generaron nuevos bloques
        prevSize = currentSize;

        vector<Block> newBlocks;

        for (int i = 0; i < currentSize && (int)blocks.size() + (int)newBlocks.size() < maxBlocks; i++) {
            for (int j = i; j < currentSize && (int)blocks.size() + (int)newBlocks.size() < maxBlocks; j++) {
                Block combined;
                if (!combineBlocks(blocks[i], blocks[j], minFillRate, combined))
                    continue;

                // No exceder dimensiones del contenedor
                if (combined.l > L || combined.w > W || combined.h > H)
                    continue;

                // Verificar que las cajas usadas no excedan la cantidad disponible
                // Contar cuántas cajas de cada tipo usa el bloque combinado
                vector<int> usage(boxes.size() + 1, 0);
                bool feasible = true;
                for (const auto& pb : combined.boxes)
                    usage[pb.boxId]++;
                for (const auto& box : boxes) {
                    if (usage[box.id] > box.quantity) {
                        feasible = false;
                        break;
                    }
                }
                if (!feasible) continue;

                newBlocks.push_back(combined);
            }
        }

        if (newBlocks.empty()) break;
        for (auto& b : newBlocks)
            blocks.push_back(b);
    }

    // Limitar al máximo pedido
    if ((int)blocks.size() > maxBlocks)
        blocks.resize(maxBlocks);

    return blocks;
}

// ---------------------------------------------------------------------------
// Función f(b,r) — evaluación de bloque en espacio libre (K4, Araya & Riff 2014)
// f(b,r) = V(b) - Vloss(b,r)
// Vloss estima el volumen desperdiciado en el espacio residual de r
// tras colocar b, usando combinaciones lineales de dimensiones de cajas.
// ---------------------------------------------------------------------------

// EvaluateBlock con knapsack DP (usando cache)
double evaluateBlock(const Block& b, const Cuboid& r,
                     const vector<Box>& remaining,
                     KnapsackCache& cache) {
    if (!b.fitsIn(r)) return -1e9;

    int capX = r.l - b.l;
    int capY = r.w - b.w;
    int capZ = r.h - b.h;
    if (capX < 0 || capY < 0 || capZ < 0) return -1e9;

    // Recoger dimensiones de cajas restantes
    vector<int> lengths, widths, heights;
    for (const auto& box : remaining) {
        if (box.quantity <= 0) continue;
        auto orients = getOrientations(box);
        for (auto& [l, w, h] : orients) {
            lengths.push_back(l);
            widths.push_back(w);
            heights.push_back(h);
        }
    }
    sort(lengths.begin(), lengths.end());
    lengths.erase(unique(lengths.begin(), lengths.end()), lengths.end());
    sort(widths.begin(), widths.end());
    widths.erase(unique(widths.begin(), widths.end()), widths.end());
    sort(heights.begin(), heights.end());
    heights.erase(unique(heights.begin(), heights.end()), heights.end());

    // FIX: la key del cache combina firma de cajas + capacidad del eje
    // Así dos espacios distintos con igual firma pero distinta capacidad
    // no colisionan, evitando accesos fuera de bounds.
    size_t sig = boxesSignature(remaining);
    auto makeKey = [](size_t s, int cap) -> size_t {
        return s ^ (static_cast<size_t>(cap) * 2654435761ULL);
    };

    auto getKnap = [&](unordered_map<size_t, int>& cacheMap,
                       int cap, const vector<int>& dims) -> int {
        if (cap <= 0) return 0;
        size_t key = makeKey(sig, cap);
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) return it->second;
        // Calcular knapsack solo para esta capacidad específica
        vector<int> best(cap + 1, 0);
        for (int c = 1; c <= cap; ++c)
            for (int d : dims)
                if (d <= c && best[c - d] + d > best[c])
                    best[c] = best[c - d] + d;
        int result = best[cap];
        cacheMap[key] = result;
        return result;
    };

    int lmax = getKnap(cache.cacheLength, capX, lengths);
    int wmax = getKnap(cache.cacheWidth,  capY, widths);
    int hmax = getKnap(cache.cacheHeight, capZ, heights);

    double usableVol = (double)(b.l + lmax) * (b.w + wmax) * (b.h + hmax);
    double vloss = (double)r.volume() - usableVol;
    return (double)b.usedVolume - vloss;
}