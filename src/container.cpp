#include "container.h"
#include <algorithm>
#include <cmath>
#include <climits>

using namespace std;
// ---------------------------------------------------------------------------
// Cuboid
// ---------------------------------------------------------------------------

int Cuboid::manhattanDistance(int contL, int contW, int contH) const {
    // Cada esquina del cuboide tiene una esquina "asociada" del contenedor.
    // La esquina del cuboide más cercana a su esquina de contenedor asociada
    // define la distancia Manhattan mínima (K3 del paper, Araya & Riff 2014).
    //
    // Las 8 esquinas del cuboide y sus esquinas de contenedor asociadas:
    struct Corner { int cx, cy, cz; int tx, ty, tz; };
    Corner corners[8] = {
        {x,   y,   z,   0,    0,    0   },  // inferior-izq-frontal → (0,0,0)
        {x+l, y,   z,   contL,0,    0   },  // inferior-der-frontal → (L,0,0)
        {x,   y+w, z,   0,    contW,0   },  // inferior-izq-trasera → (0,W,0)
        {x+l, y+w, z,   contL,contW,0   },
        {x,   y,   z+h, 0,    0,    contH},
        {x+l, y,   z+h, contL,0,    contH},
        {x,   y+w, z+h, 0,    contW,contH},
        {x+l, y+w, z+h, contL,contW,contH},
    };

    int minDist = INT_MAX;
    for (auto& c : corners) {
        int d = abs(c.cx - c.tx) + abs(c.cy - c.ty) + abs(c.cz - c.tz);
        minDist = min(minDist, d);
    }
    return minDist;
}

// ---------------------------------------------------------------------------
// Container
// ---------------------------------------------------------------------------

Container::Container(int L, int W, int H) : L(L), W(W), H(H) {
    // Estado inicial: un único espacio libre = el contenedor completo
    freeSpaces.push_back({0, 0, 0, L, W, H});
}

double Container::volumeUsed() const {
    double total = 0;
    for (const auto& p : placed)
        total += (double)p.l * p.w * p.h;
    return total;
}

// ---------------------------------------------------------------------------
// Núcleo de la cover representation
// Al colocar un bloque b en espacio r, se generan hasta 3 nuevos cuboides
// que cubren el espacio restante de r. Ver Fig.1 del paper.
// ---------------------------------------------------------------------------
vector<Cuboid> Container::splitSpace(const Cuboid& r,
                                           int bl, int bw, int bh) const {
    vector<Cuboid> result;

    // Espacio a la derecha del bloque (eje X)
    if (r.l - bl > 0)
        result.push_back({r.x + bl, r.y, r.z, r.l - bl, r.w, r.h});

    // Espacio detrás del bloque (eje Y)
    if (r.w - bw > 0)
        result.push_back({r.x, r.y + bw, r.z, r.l, r.w - bw, r.h});

    // Espacio encima del bloque (eje Z)
    if (r.h - bh > 0)
        result.push_back({r.x, r.y, r.z + bh, r.l, r.w, r.h - bh});

    return result;
}

// Elimina espacios que solapan con el bloque recién colocado en (bx,by,bz)
void Container::removeOverlapping(int bx, int by, int bz,
                                   int bl, int bw, int bh) {
    // Un cuboide r solapa con el bloque si se intersectan en los 3 ejes
    auto overlaps = [&](const Cuboid& r) {
        return (r.x < bx + bl) && (r.x + r.l > bx) &&
               (r.y < by + bw) && (r.y + r.w > by) &&
               (r.z < bz + bh) && (r.z + r.h > bz);
    };

    vector<Cuboid> newSpaces;
    for (const auto& r : freeSpaces) {
        if (!overlaps(r)) {
            newSpaces.push_back(r);
            continue;
        }
        // El espacio solapa: lo dividimos en hasta 6 nuevos cuboides
        // (uno por cada cara del bloque que "corta" el espacio)
        if (r.x < bx)
            newSpaces.push_back({r.x, r.y, r.z, bx - r.x, r.w, r.h});
        if (r.x + r.l > bx + bl)
            newSpaces.push_back({bx + bl, r.y, r.z, r.x + r.l - (bx + bl), r.w, r.h});
        if (r.y < by)
            newSpaces.push_back({r.x, r.y, r.z, r.l, by - r.y, r.h});
        if (r.y + r.w > by + bw)
            newSpaces.push_back({r.x, by + bw, r.z, r.l, r.y + r.w - (by + bw), r.h});
        if (r.z < bz)
            newSpaces.push_back({r.x, r.y, r.z, r.l, r.w, bz - r.z});
        if (r.z + r.h > bz + bh)
            newSpaces.push_back({r.x, r.y, bz + bh, r.l, r.w, r.z + r.h - (bz + bh)});
    }
    freeSpaces = newSpaces;
}

// Elimina cuboides contenidos completamente dentro de otro (no-maximales)
void Container::removeNonMaximal() {
    auto isContained = [](const Cuboid& a, const Cuboid& b) {
        // ¿está 'a' completamente dentro de 'b'?
        return a.x >= b.x && a.y >= b.y && a.z >= b.z &&
               a.x + a.l <= b.x + b.l &&
               a.y + a.w <= b.y + b.w &&
               a.z + a.h <= b.z + b.h;
    };

    vector<bool> remove(freeSpaces.size(), false);
    for (int i = 0; i < (int)freeSpaces.size(); i++) {
        if (remove[i]) continue;
        for (int j = 0; j < (int)freeSpaces.size(); j++) {
            if (i == j || remove[j]) continue;
            if (isContained(freeSpaces[i], freeSpaces[j]) &&
                !(isContained(freeSpaces[j], freeSpaces[i]))) {
                remove[i] = true;
                break;
            }
        }
    }

    vector<Cuboid> maximal;
    for (int i = 0; i < (int)freeSpaces.size(); i++)
        if (!remove[i])
            maximal.push_back(freeSpaces[i]);
    freeSpaces = maximal;
}

void Container::placeBlock(const Cuboid& r, int bl, int bw, int bh,
                            const vector<PlacedBox>& boxesInBlock) {
    // Registrar las cajas colocadas
    for (const auto& pb : boxesInBlock)
        placed.push_back(pb);

    // 1. Generar los 3 nuevos cuboides del espacio r particionado
    auto newSpaces = splitSpace(r, bl, bw, bh);

    // 2. Quitar el espacio r que fue ocupado
    freeSpaces.erase(
        remove_if(freeSpaces.begin(), freeSpaces.end(),
            [&](const Cuboid& c) {
                return c.x == r.x && c.y == r.y && c.z == r.z &&
                       c.l == r.l && c.w == r.w && c.h == r.h;
            }),
        freeSpaces.end()
    );

    // 3. Agregar los nuevos espacios
    for (const auto& ns : newSpaces)
        freeSpaces.push_back(ns);

    // 4. Limpiar espacios que solapan con el bloque colocado
    removeOverlapping(r.x, r.y, r.z, bl, bw, bh);

    // 5. Eliminar cuboides no-maximales
    removeNonMaximal();
}

int Container::selectFreeSpace() const {
    if (freeSpaces.empty()) return -1;

    int best = 0;
    int bestDist = freeSpaces[0].manhattanDistance(L, W, H);
    int bestVol  = freeSpaces[0].volume();

    for (int i = 1; i < (int)freeSpaces.size(); i++) {
        int d = freeSpaces[i].manhattanDistance(L, W, H);
        int v = freeSpaces[i].volume();
        // Menor distancia Manhattan primero; empates se rompen por mayor volumen
        if (d < bestDist || (d == bestDist && v > bestVol)) {
            best = i;
            bestDist = d;
            bestVol  = v;
        }
    }
    return best;
}