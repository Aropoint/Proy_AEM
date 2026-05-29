#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
OUTPUT_DIR="$PROJECT_DIR/results"
DATA_DIR="$PROJECT_DIR/data"
REPORT_FILE="$OUTPUT_DIR/report_$(date +%Y%m%d_%H%M%S).txt"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}=== COMPILANDO PROYECTO ===${NC}"
cd "$BUILD_DIR" || exit 1
cmake .. > /dev/null 2>&1
if ! cmake --build . ; then
    echo "Error en compilación"
    exit 1
fi
echo -e "${GREEN}✓ Compilación exitosa${NC}\n"

# Initialize report
{
    echo "========================================"
    echo "INFORME DE EJECUCIÓN - $(date)"
    echo "========================================"
    echo ""
} > "$REPORT_FILE"

# Arrays to store results
declare -a FILES
declare -a GREEDY_UTIL
declare -a BEAM_UTIL
declare -a IMPROVEMENT

# Find and sort BR files
for i in {0..15}; do
    if [ -f "$DATA_DIR/BR$i.txt" ]; then
        FILES+=("BR$i.txt")
    fi
done

TOTAL_FILES=${#FILES[@]}
CURRENT=0

# Execute each instance
for file in "${FILES[@]}"; do
    CURRENT=$((CURRENT + 1))
    instance_num="${file%.*}"

    echo -e "${YELLOW}[$CURRENT/$TOTAL_FILES]${NC} Ejecutando $instance_num..."

    # Run and capture output
    OUTPUT=$("$BUILD_DIR/clp_solver" "$DATA_DIR/$file" 0 2>&1)

    # Print output in real-time
    echo "$OUTPUT"
    echo ""

    # Extract metrics using grep
    GREEDY=$(echo "$OUTPUT" | grep "^Greedy:" | awk '{print $2}')
    BEAM=$(echo "$OUTPUT" | grep "^Beam Search:" | awk '{print $3}')
    IMPROVEMENT=$(echo "$OUTPUT" | grep "^Mejoría" | awk '{print $NF}')

    # Store results
    GREEDY_UTIL+=("$GREEDY")
    BEAM_UTIL+=("$BEAM")
    IMPROVEMENT+=("$IMPROVEMENT")

    # Append to report
    {
        echo "=============== $instance_num ==============="
        echo "Greedy:                    $GREEDY"
        echo "Beam Search:               $BEAM"
        echo "Mejoría:                   $IMPROVEMENT"
        echo ""
    } >> "$REPORT_FILE"
done

# Summary statistics
echo -e "\n${BLUE}=== GENERANDO RESUMEN ===${NC}\n"

{
    echo ""
    echo "========================================"
    echo "RESUMEN EJECUTIVO"
    echo "========================================"
    echo ""
    echo "Total de instancias ejecutadas: $TOTAL_FILES"
    echo ""
    echo "--- RESULTADOS POR INSTANCIA ---"
    echo ""
} >> "$REPORT_FILE"

# Print summary table
{
    printf "%-12s %-15s %-15s %-15s\n" "Instancia" "Greedy %" "Beam %" "Mejora %"
    printf "%-12s %-15s %-15s %-15s\n" "----------" "--------" "-------" "---------"
} | tee -a "$REPORT_FILE"

for i in "${!FILES[@]}"; do
    printf "%-12s %-15s %-15s %-15s\n" "${FILES[$i]}" "${GREEDY_UTIL[$i]}" "${BEAM_UTIL[$i]}" "${IMPROVEMENT[$i]}" | tee -a "$REPORT_FILE"
done

# Calculate averages
echo "" | tee -a "$REPORT_FILE"

# Use awk to calculate averages from arrays
AVG_GREEDY=$(printf '%s\n' "${GREEDY_UTIL[@]}" | awk '{sum+=$1; count++} END {if(count>0) printf "%.2f", sum/count; else print "N/A"}')
AVG_BEAM=$(printf '%s\n' "${BEAM_UTIL[@]}" | awk '{sum+=$1; count++} END {if(count>0) printf "%.2f", sum/count; else print "N/A"}')
AVG_IMPROVEMENT=$(printf '%s\n' "${IMPROVEMENT[@]}" | awk '{sum+=$1; count++} END {if(count>0) printf "%.2f", sum/count; else print "N/A"}')

{
    echo "--- PROMEDIOS GENERALES ---"
    echo "Promedio Greedy:           $AVG_GREEDY%"
    echo "Promedio Beam Search:      $AVG_BEAM%"
    echo "Promedio Mejora:           $AVG_IMPROVEMENT%"
    echo ""
    echo "Reporte guardado en: $REPORT_FILE"
    echo "Fecha de finalización: $(date)"
} | tee -a "$REPORT_FILE"

echo -e "\n${GREEN}✓ Proceso completado. Reporte guardado en: $REPORT_FILE${NC}"
