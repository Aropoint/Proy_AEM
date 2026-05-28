#include "box.h"
#include <set>

vector<tuple<int,int,int>> getOrientations(const Box& box) {
    int l = box.l, w = box.w, h = box.h;

    // Las 6 permutaciones posibles de (l, w, h)
    vector<tuple<int,int,int>> all = {
        {l, w, h}, {l, h, w},
        {w, l, h}, {w, h, l},
        {h, l, w}, {h, w, l}
    };

    // En los archivos BR, ol/ow/oh indican si la dimensión puede quedar
    // en posición vertical (eje Z). Si ol=0, la dimensión l NO puede ser altura.
    // Filtramos orientaciones donde la altura (tercer elemento) no está permitida.
    vector<tuple<int,int,int>> valid;
    set<tuple<int,int,int>> seen; // evita duplicados si hay dims iguales

    for (auto& [ol, ow, oh] : all) {
        // oh corresponde al tercer elemento (altura en el contenedor)
        // Verificamos qué dimensión original está en posición de altura
        bool allowed = true;
        if (oh == l && !box.ol) allowed = false;
        if (oh == w && !box.ow) allowed = false;
        if (oh == h && !box.oh) allowed = false;

        if (allowed && seen.find({ol, ow, oh}) == seen.end()) {
            valid.push_back({ol, ow, oh});
            seen.insert({ol, ow, oh});
        }
    }

    // Si ninguna orientación es válida (caso extremo), devolvemos la original
    if (valid.empty())
        valid.push_back({l, w, h});

    return valid;
}