#!/usr/bin/env bash
# run_pomiar_2b.sh — pomiar transmisji ramki MODBUS RTU (Zadanie 2B)

set -uo pipefail
cd "$(dirname "$0")"
source "$(dirname "$0")/shared.sh"

# ── Nagłówek ──────────────────────────────────────────────────────────────────
echo -e "${BOLD}"
echo "╔══════════════════════════════════════════════════╗"
echo "║    Pomiar transmisji szeregowej — Zadanie 2B     ║"
echo "║    Ramka MODBUS RTU: 01 03 00 00 00 02 C4 0B     ║"
echo "╚══════════════════════════════════════════════════╝"
echo -e "${RESET}"

start_socat
start_drainer

# ── Parametry pomiaru ─────────────────────────────────────────────────────────
echo -e "${BOLD}Parametry pomiaru${RESET} (Enter = wartość w nawiasie)\n"

read -r -p "  Liczba ramek           [10000] : " FRAMES
FRAMES=${FRAMES:-10000}

read -r -p "  Etykieta scenariusza   [base]  : " LABEL
LABEL=${LABEL:-base}

read -r -p "  Katalog wyjściowy      [.]     : " OUTPUT_DIR
OUTPUT_DIR=${OUTPUT_DIR:-.}

# ── Podsumowanie konfiguracji ─────────────────────────────────────────────────
echo ""
echo -e "${BOLD}Konfiguracja:${RESET}"
echo -e "  Port        : ${PTY1}"
echo -e "  Ramki       : ${FRAMES}"
echo -e "  Etykieta    : ${LABEL}"
echo -e "  Wyj. katalog: ${OUTPUT_DIR}"
echo -e "  Metoda      : zastępcza (T_ramka / 8 znaków)"
echo ""
read -r -p "Naciśnij Enter aby rozpocząć pomiar (Ctrl+C aby anulować)..."
echo ""

# ── Pomiar ────────────────────────────────────────────────────────────────────
echo -e "${BOLD}Startuje pomiar...${RESET}\n"

uv run python zadanie2b.py "$PTY1" \
    --frames "$FRAMES" \
    --label  "$LABEL"  \
    --output-dir "$OUTPUT_DIR"

echo -e "\n${GREEN}Pomiar zakończony. Wykres zapisany do ${OUTPUT_DIR}/histogram_${LABEL}.png${RESET}"
