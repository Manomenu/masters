#!/usr/bin/env bash
# run_pomiar_2a.sh — pomiar transmisji bajt po bajcie (Zadanie 2A)

set -uo pipefail
cd "$(dirname "$0")"
source "$(dirname "$0")/shared.sh"

# ── Nagłówek ──────────────────────────────────────────────────────────────────
echo -e "${BOLD}"
echo "╔══════════════════════════════════════════════════╗"
echo "║    Pomiar transmisji szeregowej — Zadanie 2A     ║"
echo "╚══════════════════════════════════════════════════╝"
echo -e "${RESET}"

start_socat
start_drainer

# ── Parametry pomiaru ─────────────────────────────────────────────────────────
echo -e "${BOLD}Parametry pomiaru${RESET} (Enter = wartość w nawiasie)\n"

read -r -p "  Liczba próbek          [10000] : " SAMPLES
SAMPLES=${SAMPLES:-10000}

read -r -p "  Etykieta scenariusza   [base]  : " LABEL
LABEL=${LABEL:-base}

read -r -p "  Metoda blokowa?  (t/n) [n]     : " USE_BLOCK
USE_BLOCK=${USE_BLOCK:-n}

BLOCK_ARGS=()
case "$USE_BLOCK" in
    t|T|tak|y|Y|yes)
        read -r -p "  Rozmiar bloku          [100]   : " BLOCK_SIZE
        BLOCK_SIZE=${BLOCK_SIZE:-100}
        BLOCK_ARGS=(--block --block-size "$BLOCK_SIZE")
        ;;
esac

read -r -p "  Katalog wyjściowy      [.]     : " OUTPUT_DIR
OUTPUT_DIR=${OUTPUT_DIR:-.}

# ── Podsumowanie konfiguracji ─────────────────────────────────────────────────
echo ""
echo -e "${BOLD}Konfiguracja:${RESET}"
echo -e "  Port       : ${PTY1}"
echo -e "  Próbki     : ${SAMPLES}"
echo -e "  Etykieta   : ${LABEL}"
echo -e "  Wyj. katalog: ${OUTPUT_DIR}"
if [[ ${#BLOCK_ARGS[@]} -gt 0 ]]; then
    echo -e "  Metoda     : blokowa (rozmiar bloku = ${BLOCK_SIZE})"
else
    echo -e "  Metoda     : indywidualna (1 bajt na raz)"
fi
echo ""
read -r -p "Naciśnij Enter aby rozpocząć pomiar (Ctrl+C aby anulować)..."
echo ""

# ── Pomiar ────────────────────────────────────────────────────────────────────
echo -e "${BOLD}Startuje pomiar...${RESET}\n"

CMD_ARGS=("$PTY1" --samples "$SAMPLES" --label "$LABEL" --output-dir "$OUTPUT_DIR")
[[ ${#BLOCK_ARGS[@]} -gt 0 ]] && CMD_ARGS+=("${BLOCK_ARGS[@]}")

uv run python zadanie2a.py "${CMD_ARGS[@]}"

echo -e "\n${GREEN}Pomiar zakończony. Wykres zapisany do ${OUTPUT_DIR}/histogram_${LABEL}.png${RESET}"
