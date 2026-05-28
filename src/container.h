#pragma once
#include "box.h"
#include <vector>

using namespace std;
// Cuboide que representa una región del espacio (libre o bloque)
struct Cuboid {
    int x, y, z;       // esquina inferior-izquierda-frontal
    int l, w, h;       // dimensiones

    int volume() const { return l * w * h; }

    // Distancia Manhattan desde la esquina más cercana del contenedor
    // Usada para seleccionar el próximo espacio libre (K3 del paper)
    int manhattanDistance(int contL, int contW, int contH) const;

    // Verifica si otro cuboide cabe dentro de este espacio
    bool fits(int bl, int bw, int bh) const {
        return bl <= l && bw <= w && bh <= h;
    }
};

// Estado del contenedor en un momento dado (solución parcial)
struct Container {
    int L, W, H;                        // dimensiones fijas

    vector<Cuboid> freeSpaces;     // espacios libres (pueden solaparse)
    vector<PlacedBox> placed;      // cajas ya colocadas

    // Inicializa con el contenedor vacío (un solo espacio libre = todo el contenedor)
    Container(int L, int W, int H);

    // Copia profunda (necesaria para beam search: cada estado es independiente)
    Container(const Container&) = default;
    Container& operator=(const Container&) = default;

    // Volumen total cargado actualmente
    double volumeUsed() const;

    // Porcentaje de utilización (la métrica del paper)
    double utilization() const { return volumeUsed() / (double)(L * W * H) * 100.0; }

    // Coloca un bloque (l x w x h) en la esquina (x,y,z) del espacio libre r
    // Actualiza freeSpaces según la cover representation
    void placeBlock(const Cuboid& r, int bl, int bw, int bh,
                    const vector<PlacedBox>& boxesInBlock);

    // Selecciona el espacio libre con menor distancia Manhattan (K3)
    // Retorna índice en freeSpaces, o -1 si no hay espacios
    int selectFreeSpace() const;

private:
    // Genera los 3 nuevos cuboides al colocar un bloque en un espacio libre
    vector<Cuboid> splitSpace(const Cuboid& r, int bl, int bw, int bh) const;

    // Elimina cuboides no-maximales (contenidos completamente en otro)
    void removeNonMaximal();

    // Elimina cuboides que solapan con el bloque recién colocado
    void removeOverlapping(int bx, int by, int bz, int bl, int bw, int bh);
};