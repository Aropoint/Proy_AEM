# CLP Solver - Beam Search vs Greedy

Implementación del algoritmo **Beam Search** para el **Container Loading Problem** basado en el paper:
**"A beam search approach to the container loading problem"** - Araya & Riff (2014)

## 📊 Algoritmos Implementados

### 1. **Greedy Algorithm**
- Selecciona el espacio libre con menor distancia Manhattan (K3)
- Evalúa bloques usando función f(b,r) del paper (K4)
- Coloca el mejor bloque en cada iteración
- **Tiempo O(n log n)** - muy rápido pero subóptimo

### 2. **Beam Search Algorithm**  
- Mantiene K estados candidatos en paralelo
- Expande cada estado con los Top-K mejores bloques
- Ancho de beam adaptivo según diversidad de estados
- **Mejor calidad** pero más lento (~1-100x más tiempo)

## 🔧 Estructura del Código

```
src/
├── box.h/cpp           # Tipos de cajas y orientaciones
├── container.h/cpp     # Gestión del contenedor (espacios libres)
├── block.h/cpp         # Generación de bloques (K2)
├── greedy.h/cpp        # Algoritmo Greedy
├── beam_search.h/cpp   # Algoritmo Beam Search
├── parser.h/cpp        # Parsing de archivos BR
└── main.cpp            # Función principal
```

## 📈 Resultados Experimentales

### Instancias Homogéneas (BR0-BR5)
- Greedy: **81% promedio**
- Beam Search: **81% promedio**
- **Conclusión**: Para instancias simples, Greedy es suficiente

### Instancias Heterogéneas (BR6-BR15)  
- **BR6**: Greedy 75.84% → Beam Search **81.73%** (+7.63%)
- **BR7**: Greedy 58.32% → Beam Search **82.72%** (+41.84%) ⭐
- **BR8**: Greedy 76.90% → Beam Search **78.09%** (+1.54%)

### Recomendaciones

| Instancia | Algoritmo | Razón |
|-----------|-----------|-------|
| BR0-BR5 | **Greedy** | Suficiente + Rápido |
| BR6-BR7 | **Beam Search** | Gran mejora (+7-42%) |
| BR8-BR15 | **Greedy o Beam(w=3)** | Equilibrio calidad/tiempo |

## 🚀 Compilación y Ejecución

```bash
# Compilar
g++ -std=c++17 -O2 -Wall src/*.cpp -o solver_new.exe

# Ejecutar
./solver_new.exe ./data/BR0.txt 0
./solver_new.exe ./data/BR8.txt 0
```

## 💡 Mejoras Futuras

1. **Generación dinámica de bloques** durante la búsqueda
   - Actualizar bloques disponibles según espacios residuales
   
2. **Heurística admisible** para estimar máximo potencial
   - Mejorar poda en Beam Search
   
3. **Local Search** después de Beam Search
   - Reinserción de cajas no colocadas
   
4. **Paralelización** en la expansión de estados
   - Multi-threading para acelerar búsqueda

## 📝 Referencias

- **Paper**: Araya, G., & Riff, M. C. (2014). "A beam search approach to the container loading problem". 
- **Dataset**: OR-Library instances BR0-BR15
- **Publicación**: Kluwer Academic Publishers

---

**Estado**: ✅ Implementación completa y funcional  
**Fecha**: Mayo 2026
