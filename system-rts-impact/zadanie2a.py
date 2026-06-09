#!/usr/bin/env python3
"""
Zadanie 2A a) — Pomiar odstępów czasowych między kolejnymi bajtami
Transmisja ciągła bajtu 0xC4 na wirtualny port szeregowy.
Parametry: 115200 baud, 8 bitów danych, bit parzystości, 1 bit stopu (8E1).

Uruchomienie:
    # Terminal 1 — utwórz parę wirtualnych portów:
    socat -d -d pty,raw,echo=0 pty,raw,echo=0
    # (zapamiętaj wypisane ścieżki, np. /dev/ttys001 i /dev/ttys002)

    # Terminal 2 — uruchom pomiar:
    python3 zadanie2a.py /dev/ttys001

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

BYTE_VAL = bytes([0xC4])
WARMUP_N = 200      # bajty rozgrzewki (pominięte w pomiarze)


# ── Pomiar ───────────────────────────────────────────────────────────────────

def measure_individual(ser: serial.Serial, n_samples: int) -> np.ndarray:
    """
    Wysyła n_samples bajtów 0xC4 jeden po drugim.
    Mierzy czas między kolejnymi wywołaniami write()+flush().
    Zwraca tablicę odstępów w µs (n_samples - 1 wartości).
    """
    for _ in range(WARMUP_N):
        ser.write(BYTE_VAL)
    ser.flush()
    time.sleep(0.05)

    intervals: list[float] = []
    t_prev = time.perf_counter()

    for _ in range(n_samples):
        ser.write(BYTE_VAL)
        ser.flush()
        t_curr = time.perf_counter()
        intervals.append((t_curr - t_prev) * 1e6)
        t_prev = t_curr

    return np.array(intervals[1:])


def measure_block(ser: serial.Serial, block_size: int, n_blocks: int) -> np.ndarray:
    """
    Metoda zastępcza: wysyła bloki po block_size bajtów, mierzy czas całkowity.
    Zwraca tablicę średnich czasów na bajt w µs (n_blocks wartości).
    """
    block = BYTE_VAL * block_size
    avg_times: list[float] = []

    for _ in range(n_blocks):
        t0 = time.perf_counter()
        ser.write(block)
        ser.flush()
        t1 = time.perf_counter()
        avg_times.append((t1 - t0) * 1e6 / block_size)

    return np.array(avg_times)


# ── Main ─────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Zadanie 2A a) — pomiar odstępów między bajtami na porcie szeregowym"
    )
    parser.add_argument("port",    help="Port szeregowy, np. /dev/ttys001")
    parser.add_argument("--samples", type=int, default=10_000,
                        help="Liczba wysyłanych bajtów (domyślnie 10000)")
    parser.add_argument("--label", type=str, default="",
                        help="Etykieta scenariusza np. 'idle' lub '10apps'")
    parser.add_argument("--block", action="store_true",
                        help="Użyj metody blokowej zamiast indywidualnej")
    parser.add_argument("--block-size", type=int, default=100,
                        help="Rozmiar bloku w metodzie blokowej (domyślnie 100)")
    parser.add_argument("--output-dir", type=str, default=".",
                        help="Katalog zapisu histogramu (domyślnie bieżący katalog)")
    args = parser.parse_args()

    output_dir = Path(args.output_dir).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    print("\nParametry teoretyczne:")
    print(f"  Czas bitu        : {BIT_TIME_US:.2f} µs  (@ {BAUD_RATE} baud)")
    print(f"  Czas znaku (11b) : {CHAR_TIME_US:.2f} µs")
    print(f"  Max gap MODBUS   : {MAX_GAP_US:.2f} µs")

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

        if args.block:
            n_blocks = args.samples // args.block_size
            print(f"Metoda blokowa: {n_blocks} bloków × {args.block_size} bajtów...")
            intervals = measure_block(ser, args.block_size, n_blocks)
        else:
            print(f"Metoda indywidualna: {args.samples} bajtów 0xC4...")
            intervals = measure_individual(ser, args.samples)

    print_stats(intervals, label=args.label)
    plot_histogram(intervals, label=args.label, output_dir=output_dir)


if __name__ == "__main__":
    main()
