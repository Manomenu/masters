#!/usr/bin/env python3
"""
Zadanie 2B a) — Pomiar rozkładu czasów odstępu między znakami w ramce MODBUS RTU
Transmisja ciągła ramki zapytania RTU: 01 03 00 00 00 02 C4 0B
Parametry: 115200 baud, 8 bitów danych, bit parzystości, 1 bit stopu (8E1).

Metoda zastępcza: mierzy czas całej ramki (8 znaków), szacuje
średni czas na znak jako T_ramka / 8.

Uruchomienie:
    # Terminal 1 — utwórz parę wirtualnych portów:
    socat -d -d pty,raw,echo=0 pty,raw,echo=0

    # Terminal 2 — uruchom pomiar:
    python3 zadanie2b.py /dev/ttys001

    # Opcjonalnie — monitoruj drugi koniec pary:
    # cat /dev/ttys002 | xxd | head
"""

import argparse
import time
import sys
from pathlib import Path

import numpy as np
import serial

from shared import (
    BAUD_RATE, BIT_TIME_US, CHAR_TIME_US, MAX_GAP_US,
    print_stats, plot_histogram,
)

# ── Stałe specyficzne dla 2B ─────────────────────────────────────────────────
# Ramka MODBUS RTU: Read Holding Registers
#   Adres slave   : 0x01
#   Kod funkcji   : 0x03  (Read Holding Registers)
#   Adres startu  : 0x0000
#   Liczba rej.   : 0x0002
#   CRC-16        : 0xC4 0x0B  (little-endian: 0x0BC4)
FRAME    = bytes([0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B])
N_CHARS  = len(FRAME)                        # 8 znaków
FRAME_TIME_US = N_CHARS * CHAR_TIME_US       # ≈ 763.89 µs  (minimum teoretyczne)
WARMUP_N = 200                               # ramki rozgrzewki


# ── Pomiar ───────────────────────────────────────────────────────────────────

def measure_frame(ser: serial.Serial, n_frames: int) -> np.ndarray:
    """
    Metoda zastępcza: wysyła ramkę MODBUS RTU n_frames razy.
    Mierzy czas całej ramki i szacuje średni czas na znak: T_ramka / N_CHARS.
    Zwraca tablicę szacowanych T_znak w µs (n_frames - 1 wartości).
    """
    for _ in range(WARMUP_N):
        ser.write(FRAME)
    ser.flush()
    time.sleep(0.05)

    avg_times: list[float] = []

    for _ in range(n_frames):
        t0 = time.perf_counter()
        ser.write(FRAME)
        ser.flush()
        t1 = time.perf_counter()
        avg_times.append((t1 - t0) * 1e6 / N_CHARS)

    return np.array(avg_times[1:])   # pierwszy pomiar pomijamy (granica warmup)


# ── Main ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Zadanie 2B a) — pomiar odstępów między znakami w ramce MODBUS RTU"
    )
    parser.add_argument("port", help="Port szeregowy, np. /dev/ttys001")
    parser.add_argument("--frames", type=int, default=10_000,
                        help="Liczba wysyłanych ramek (domyślnie 10000)")
    parser.add_argument("--label", type=str, default="",
                        help="Etykieta scenariusza np. 'base' lub '10apps'")
    parser.add_argument("--output-dir", type=str, default=".",
                        help="Katalog zapisu histogramu (domyślnie bieżący katalog)")
    args = parser.parse_args()

    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    print("\nParametry teoretyczne:")
    print(f"  Czas bitu        : {BIT_TIME_US:.2f} µs  (@ {BAUD_RATE} baud)")
    print(f"  Czas znaku (11b) : {CHAR_TIME_US:.2f} µs")
    print(f"  Czas ramki (8z.) : {FRAME_TIME_US:.2f} µs  (min. teoretyczne)")
    print(f"  Max gap MODBUS   : {MAX_GAP_US:.2f} µs")
    print(f"  Ramka            : {FRAME.hex(' ').upper()}")

    try:
        ser = serial.Serial(
            port=args.port,
            baudrate=BAUD_RATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_EVEN,
            stopbits=serial.STOPBITS_ONE,
            write_timeout=2.0,
        )
    except serial.SerialException as e:
        print(f"\nBłąd otwarcia portu {args.port}: {e}", file=sys.stderr)
        sys.exit(1)

    with ser:
        print(f"\nOtwarto port : {ser.name}")
        print(f"Metoda zastępcza: {args.frames} ramek × {N_CHARS} znaków...")
        intervals = measure_frame(ser, args.frames)

    extra = [f"  Czas ramki (8z.) : {FRAME_TIME_US:.2f} µs  (min. teoretyczne)"]
    print_stats(intervals, label=args.label, extra_lines=extra)
    plot_histogram(
        intervals,
        label=args.label,
        output_dir=output_dir,
        xlabel="Szacowany średni czas znaku w ramce [µs]  (T_ramka / 8)",
        title_line1="Histogram szacowanego czasu znaku w ramce MODBUS RTU",
    )


if __name__ == "__main__":
    main()
