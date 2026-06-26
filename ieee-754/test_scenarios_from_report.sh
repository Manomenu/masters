#!/usr/bin/env bash
#
# test_scenarios_from_report.sh
#
# Uruchamia oba programy (task2, task3) na WSZYSTKICH parach input-output
# wystepujacych w pracy (tabele zadan 5.1, 5.2, 5.3) i zapisuje pelne wyjscia
# do plikow outputs/ex_<wartosc>.txt. Na koniec weryfikuje, czy wynik kazdego
# programu zgadza sie z wartoscia podana w pracy.
#
# Uzycie:  ./test_scenarios_from_report.sh
#
set -u
cd "$(dirname "$0")"

# --- platforma / nazwy plikow wykonywalnych ---------------------------------
EXE=""
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) EXE=".exe";; esac
T2="./task2_internal_to_ieee$EXE"
T3="./task3_ieee_to_internal$EXE"
CXX="${CXX:-c++}"

# --- budowanie programow ----------------------------------------------------
echo "Budowanie programow ($CXX)..."
$CXX -std=c++17 -O2 -o "$T2" src/task2_internal_to_ieee.cpp src/shared.cpp || exit 1
$CXX -std=c++17 -O2 -o "$T3" src/task3_ieee_to_internal.cpp src/shared.cpp || exit 1

# --- katalog wynikowy -------------------------------------------------------
OUT="outputs"
rm -rf "$OUT"; mkdir -p "$OUT"

# --- konwersja hex (8 cyfr) -> 32-bitowy ciag binarny (wejscie task2) -------
nib() {
  case "$1" in
    0) printf 0000;; 1) printf 0001;; 2) printf 0010;; 3) printf 0011;;
    4) printf 0100;; 5) printf 0101;; 6) printf 0110;; 7) printf 0111;;
    8) printf 1000;; 9) printf 1001;; a|A) printf 1010;; b|B) printf 1011;;
    c|C) printf 1100;; d|D) printf 1101;; e|E) printf 1110;; f|F) printf 1111;;
  esac
}
hex_to_bin32() {
  local h=${1#0x} out="" i
  for (( i=0; i<${#h}; i++ )); do out+=$(nib "${h:i:1}"); done
  printf '%s' "$out"
}
upper() { printf '%s' "$1" | tr 'a-f' 'A-F'; }

# --- scenariusze z pracy:  wartosc | IEEE 754 | kod wewn. | task2? | task3? --
SCEN="
0|0x00000000|0x00000000|1|1
1|0x3F800000|0x80000000|1|1
2|0x40000000|0x81000000|0|1
9|0x41100000|0x83100000|1|1
65535|0x477FFF00|0x8F7FFF00|1|1
65536|0x47800000|0x90000000|1|1
0.5|0x3F000000|0x7F000000|1|1
0.25|0x3E800000|0x7E000000|0|1
-1|0xBF800000|0x80800000|1|1
-9|0xC1100000|0x83900000|1|1
pi|0x40490FDB|0x81490FDB|0|1
"

pass=0; fail=0
echo
echo "Uruchamianie scenariuszy:"
echo "-----------------------------------------------------------------------"

while IFS='|' read -r val ieee intern t2 t3; do
  [ -z "${val// }" ] && continue
  file="$OUT/ex_${val}.txt"
  {
    echo "==================================================================="
    echo " Scenariusz z pracy: wartosc = $val"
    echo "   oczekiwane: IEEE 754       = $ieee"
    echo "               Kod wewnetrzny = $intern"
    echo "==================================================================="
  } > "$file"

  # --- kierunek IEEE 754 -> kod wewnetrzny (task3) ---
  if [ "$t3" = "1" ]; then
    {
      echo
      echo ">>> $T3 $ieee     # IEEE 754 -> kod wewnetrzny"
    } >> "$file"
    out=$("$T3" "$ieee" 2>&1); printf '%s\n' "$out" >> "$file"
    got=$(printf '%s\n' "$out" | awk '/Kod wewnetrzny:/{print $3}')
    if [ "$(upper "$got")" = "$(upper "$intern")" ]; then
      printf "  ex_%-7s task3  %s -> %s   OK\n" "$val" "$ieee" "$got"; pass=$((pass+1))
    else
      printf "  ex_%-7s task3  %s -> %s   FAIL (oczekiwano %s)\n" "$val" "$ieee" "$got" "$intern"; fail=$((fail+1))
    fi
  fi

  # --- kierunek kod wewnetrzny -> IEEE 754 (task2) ---
  if [ "$t2" = "1" ]; then
    bin=$(hex_to_bin32 "$intern")
    {
      echo
      echo ">>> $T2 $bin     # kod wewnetrzny -> IEEE 754"
    } >> "$file"
    out=$("$T2" "$bin" 2>&1); printf '%s\n' "$out" >> "$file"
    got=$(printf '%s\n' "$out" | awk '/IEEE 754:/{print $3}')
    if [ "$(upper "$got")" = "$(upper "$ieee")" ]; then
      printf "  ex_%-7s task2  %s -> %s   OK\n" "$val" "$intern" "$got"; pass=$((pass+1))
    else
      printf "  ex_%-7s task2  %s -> %s   FAIL (oczekiwano %s)\n" "$val" "$intern" "$got" "$ieee"; fail=$((fail+1))
    fi
  fi
done <<EOF
$SCEN
EOF

echo "-----------------------------------------------------------------------"
echo "PODSUMOWANIE: $pass OK, $fail FAIL"
echo "Pelne wyjscia zapisano w katalogu: $OUT/"
[ "$fail" -eq 0 ]
