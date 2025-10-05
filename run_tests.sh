# ============================================
# Автотесты для matrix_seq и matrix_parallel
# ============================================

set -euo pipefail

SEQ=./matrix_seq
OMP=./matrix_parallel

OUTDIR=results
OUTFILE=$OUTDIR/results.csv
mkdir -p "$OUTDIR"

# ===============================
# Вспомогательные функции
# ===============================

parse_time() { head -n1; }
parse_checksum() { grep -Eo 'checksum[[:space:]]*=?[[:space:]]*[0-9.]+([eE][-+]?[0-9]+)?' | awk -F'=' '{print $2}' | tr -d ' ' ; }

# Подсчёт объёма памяти в байтах
need_bytes() {
  python3 - "$@" <<'PY'
import sys
M,N,K = map(int, sys.argv[1:4])
print(8 * (M*N + N*K + M*K))
PY
}

MEM_LIMIT_GB="${MEM_LIMIT_GB:-8}"
fits_mem() {
  M="$1"; N="$2"; K="$3"
  bytes=$(need_bytes "$M" "$N" "$K")
  limit_bytes=$(python3 - <<PY
print(int(${MEM_LIMIT_GB} * (1024**3)))
PY
)
  if [ "$bytes" -le "$limit_bytes" ]; then return 0; else return 1; fi
}

# Число физических ядер
physical_cores() {
  if command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.physicalcpu 2>/dev/null && return 0
  fi
  echo 1
}

PHYS=$(physical_cores)
[ -z "$PHYS" ] && PHYS=1

echo "Category,Program,INIT,M,N,K,P,Time,Checksum" > "$OUTFILE"

run_case() {
  local category="$1" prog="$2" init="$3" M="$4" N="$5" K="$6" P="$7"
  if [ "$init" = "-" ]; then
    result=$($prog "$M" "$N" "$K" "$P" 2>&1)
  else
    result=$({ INIT="$init" $prog "$M" "$N" "$K" "$P"; } 2>&1)
  fi
  time=$(echo "$result" | parse_time)
  checksum=$(echo "$result" | parse_checksum)
  echo "$category,$prog,$init,$M,$N,$K,$P,$time,$checksum" >> "$OUTFILE"
}

echo "[+] PHYSICAL CORES detected: $PHYS"
echo "[+] Results -> $OUTFILE"

# ===============================
# 1) Проверка корректности
# ===============================
echo "[1/8] correctness..."
for init in ones rand identity; do
  for prog in "$SEQ" "$OMP"; do
    run_case correctness "$prog" "$init" 4 4 4 4
  done
done

# ===============================
# 2) Сложность O(N³)
# ===============================
echo "[2/8] complexity..."
for N in 100 200 400 800; do
  run_case complexity "$OMP" ones "$N" "$N" "$N" 4
done

# ===============================
# 3) Сильная масштабируемость
# ===============================
echo "[3/8] strong_scaling..."
M=1000; N=1000; K=1000
for P in 1 2 4 8; do
  run_case strong_scaling "$OMP" ones "$M" "$N" "$K" "$P"
done

# ===============================
# 4) Слабая масштабируемость
# ===============================
echo "[4/8] weak_scaling..."
baseN=500
for P in 1 2 4 8; do
  Ncalc=$(python3 - <<PY
import math
print(int($baseN * pow($P,1/3)))
PY
)
  run_case weak_scaling "$OMP" ones "$Ncalc" "$Ncalc" "$Ncalc" "$P"
done

# ===============================
# 5) INIT comparison
# ===============================
echo "[5/8] init_compare..."
for init in ones rand identity; do
  run_case init_compare "$OMP" "$init" 1000 1000 1000 8
done

# ===============================
# 6) Прямоугольные матрицы
# ===============================
echo "[6/8] rectangular..."
rects=(
  "512 256 128"
  "1024 512 256"
  "768 1024 256"
  "300 600 900"
  "1200 300 900"
)
for triple in "${rects[@]}"; do
  set -- $triple
  M="$1"; N="$2"; K="$3"
  run_case rectangular "$SEQ" ones "$M" "$N" "$K" 1
  run_case rectangular "$OMP" ones "$M" "$N" "$K" 8
done

# ===============================
# 7) Крупные размеры
# ===============================
echo "[7/8] big_sizes (limit ${MEM_LIMIT_GB}GB)..."
bigNs=(1200 1600 2000 2400 3000)
for N in "${bigNs[@]}"; do
  M="$N"; K="$N"
  if fits_mem "$M" "$N" "$K"; then
    run_case big_sizes "$OMP" ones "$M" "$N" "$K" 8
  else
    echo "[skip] big_sizes $N^3 > ${MEM_LIMIT_GB}GB" >&2
  fi
done

# ===============================
# 8) Перегруз потоков
# ===============================
echo "[8/8] threads_overload (phys=$PHYS)..."
P1=$(( PHYS > 0 ? PHYS : 1 ))
P2=$(( P1*2 ))
P3=$(( P1*3 ))
for P in "$P1" "$P2" "$P3"; do
  run_case threads_overload "$OMP" ones 1000 1000 1000 "$P"
done

echo "All tests complete. Results saved to $OUTFILE"

