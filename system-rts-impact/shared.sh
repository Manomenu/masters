#!/usr/bin/env bash
# shared.sh — wspólne funkcje dla run_pomiar_2a.sh i run_pomiar_2b.sh
# Source'uj ten plik: source "$(dirname "$0")/shared.sh"

# ── Kolory ────────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# ── Stan globalny (ustawiany przez start_socat / start_drainer) ───────────────
SOCAT_PID=""
SOCAT_LOG=""
DRAIN_PID=""
PTY1=""
PTY2=""

# ── Cleanup — wywoływany automatycznie przy EXIT / Ctrl+C / SIGTERM ───────────
cleanup() {
    echo -e "\n${YELLOW}Zamykanie socat...${RESET}"
    if [[ -n "$DRAIN_PID" ]] && kill -0 "$DRAIN_PID" 2>/dev/null; then
        kill "$DRAIN_PID" 2>/dev/null || true
        wait "$DRAIN_PID" 2>/dev/null || true
    fi
    if [[ -n "$SOCAT_PID" ]] && kill -0 "$SOCAT_PID" 2>/dev/null; then
        kill "$SOCAT_PID" 2>/dev/null || true
        wait "$SOCAT_PID" 2>/dev/null || true
    fi
    [[ -n "$SOCAT_LOG" ]] && rm -f "$SOCAT_LOG"
    echo -e "${GREEN}Zakończono.${RESET}"
}
trap cleanup EXIT INT TERM

# ── Uruchomienie socat i wykrycie portów PTY ──────────────────────────────────
# Po wywołaniu ustawione są zmienne: SOCAT_PID, SOCAT_LOG, PTY1, PTY2
start_socat() {
    echo -e "${CYAN}Uruchamianie socat przez nix run nixpkgs#socat...${RESET}"
    echo -e "${YELLOW}(pierwsze uruchomienie może chwilę trwać — Nix pobiera pakiet)${RESET}\n"

    SOCAT_LOG=$(mktemp)
    nix run nixpkgs#socat -- -d -d pty,raw,echo=0 pty,raw,echo=0 2>"$SOCAT_LOG" &
    SOCAT_PID=$!

    local MAX_ITER=300   # 300 × 0.2 s = 60 s
    local ITER=0
    printf "${CYAN}Czekam na inicjalizację portów PTY"

    while true; do
        local PTY_COUNT
        PTY_COUNT=$(grep -c "PTY is" "$SOCAT_LOG" 2>/dev/null || true)
        [[ "$PTY_COUNT" -ge 2 ]] && break

        if ! kill -0 "$SOCAT_PID" 2>/dev/null; then
            echo -e "\n${RED}Błąd: socat zakończył się nieoczekiwanie. Log:${RESET}"
            cat "$SOCAT_LOG"
            exit 1
        fi

        ITER=$((ITER + 1))
        if [[ "$ITER" -ge "$MAX_ITER" ]]; then
            echo -e "\n${RED}Błąd: timeout (60 s) — socat nie uruchomił portów PTY.${RESET}"
            exit 1
        fi

        sleep 0.2
        printf "."
    done

    echo -e " ${GREEN}OK${RESET}\n"

    PTY1=$(awk '/PTY is/{print $NF; exit}' "$SOCAT_LOG")
    PTY2=$(awk '/PTY is/{found++; if(found==2){print $NF; exit}}' "$SOCAT_LOG")

    echo -e "  Port transmisji  (używany przez skrypt) : ${GREEN}${PTY1}${RESET}"
    echo -e "  Port monitoringu (podgląd: cat/xxd)    : ${CYAN}${PTY2}${RESET}"
}

# ── Drainer — opróżnia bufor PTY2 w tle ──────────────────────────────────────
# Zapobiega zapełnieniu bufora jądra (~4 KB) i blokowaniu write() w Pythonie.
# Po wywołaniu ustawiona jest zmienna: DRAIN_PID
start_drainer() {
    cat "$PTY2" > /dev/null &
    DRAIN_PID=$!
    echo -e "  Drainer PTY2 uruchomiony               : PID ${DRAIN_PID}"
    echo ""
}
