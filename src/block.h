#pragma once
#include "box.h"
#include "container.h"
#include <vector>

using namespace std;

// Un bloque es un conjunto de cajas colocadas compactamente
// dentro de su cuboide envolvente mínimo (K2 del paper)
struct Block {
    int l, w, h;                        // dimensiones del cuboide envolvente
    vector<PlacedBox> boxes;       // cajas con posiciones relativas dentro del bloque
    int usedVolume;                     // volumen real ocupado por las cajas

    double fillRate() const {
        int total = l * w * h;
        return total > 0 ? (double)usedVolume / total : 0.0;
    }

    // ¿Cabe este bloque en el espacio libre r?
    bool fitsIn(const Cuboid& r) const {
        return l <= r.l && w <= r.w && h <= r.h;
    }

    // Genera PlacedBoxes con coordenadas absolutas al colocar el bloque en (ox,oy,oz)
    vector<PlacedBox> toAbsolute(int ox, int oy, int oz) const;
};

// Genera todos los bloques simples (un solo tipo de caja)
// Para instancias BR0-BR7 (homogéneas y débilmente heterogéneas)
vector<Block> generateSimpleBlocks(const vector<Box>& boxes,
                                         int L, int W, int H);

// Combina dos bloques existentes en uno nuevo (a lo largo de un eje)
// Retorna true si la combinación es válida, false si no
bool combineBlocks(const Block& b1, const Block& b2,
                   double minFillRate, Block& result);

// Procedimiento principal: genera hasta maxBlocks bloques
// combinando iterativamente pares (K2 del paper, Fanslau & Bortfeldt)
// minFillRate: fracción mínima de uso del cuboide (98% para BR8-BR15, 100% para BR0-BR7)
vector<Block> generateBlocks(const vector<Box>& boxes,
                                   int L, int W, int H,
                                   int maxBlocks,
                                   double minFillRate);

// Función f(b,r) del paper (K4): evalúa qué tan bueno es colocar
// bloque b en espacio libre r. Considera volumen usado y pérdida estimada.
double evaluateBlock(const Block& b, const Cuboid& r,
                     const vector<Box>& remaining);