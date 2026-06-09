"""
shared.py — wspólne stałe i funkcje dla zadanie2a.py i zadanie2b.py
"""

from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

# ── Stałe teoretyczne ────────────────────────────────────────────────────────
BAUD_RATE     = 115200
BITS_PER_CHAR = 11                               # 1 start + 8 data + 1 parity + 1 stop
BIT_TIME_US   = 1e6 / BAUD_RATE                  # ≈ 8.68 µs
CHAR_TIME_US  = BITS_PER_CHAR * BIT_TIME_US      # ≈ 95.49 µs
MAX_GAP_US    = 1.5 * CHAR_TIME_US               # ≈ 143.23 µs  (limit MODBUS RTU)


# ── Formatowanie procentów ────────────────────────────────────────────────────

def fmt_pct(pct: float) -> str:
    """Formatuje procent z precyzją dobraną do wielkości wartości."""
    if pct == 0.0:
        return "0"
    elif pct < 0.001:
        return f"{pct:.2e}"
    elif pct < 0.1:
        return f"{pct:.4f}"
    elif pct < 1.0:
        return f"{pct:.3f}"
    else:
        return f"{pct:.2f}"


# ── Statystyki ───────────────────────────────────────────────────────────────

def print_stats(intervals: np.ndarray, label: str = "",
                extra_lines: list[str] | None = None) -> float:
    """
    Drukuje statystyki tablicy odstępów.

    extra_lines — opcjonalne dodatkowe wiersze wstawiane po bloku parametrów
                  teoretycznych (np. czas ramki dla zadanie2b).
    Zwraca procent próbek przekraczających MAX_GAP_US.
    """
    exceed_n   = int(np.sum(intervals > MAX_GAP_US))
    exceed_pct = 100.0 * exceed_n / len(intervals)
    print(f"\n{'='*55}")
    print(f"  Scenariusz: {label or 'bazowy'}")
    print(f"{'='*55}")
    print(f"  Liczba próbek :  {len(intervals)}")
    print(f"  Średnia       :  {np.mean(intervals):.2f} µs")
    print(f"  Mediana       :  {np.median(intervals):.2f} µs")
    print(f"  Odch. std     :  {np.std(intervals):.2f} µs")
    print(f"  Min / Max     :  {np.min(intervals):.2f} / {np.max(intervals):.2f} µs")
    print(f"  P95 / P99     :  {np.percentile(intervals,95):.2f} / {np.percentile(intervals,99):.2f} µs")
    print(f"\n  Czas bitu        : {BIT_TIME_US:.2f} µs  (@ {BAUD_RATE} baud)")
    print(f"  Czas znaku (11b) : {CHAR_TIME_US:.2f} µs")
    if extra_lines:
        for line in extra_lines:
            print(line)
    print(f"  Max gap MODBUS   : {MAX_GAP_US:.2f} µs  (1.5 × czas znaku)")
    print(f"  Próbki > max gap : {exceed_n} / {len(intervals)}  ({fmt_pct(exceed_pct)}%)")
    return exceed_pct


# ── Wykres ───────────────────────────────────────────────────────────────────

def plot_histogram(
    intervals:   np.ndarray,
    label:       str  = "",
    save:        bool = True,
    output_dir:  Path = Path("."),
    xlabel:      str  = "Czas odstępu między kolejnymi bajtami [µs]",
    title_line1: str  = "Histogram czasów odstępu między kolejnymi bajtami",
) -> None:
    """
    Rysuje histogram częstości empirycznych tablicy odstępów.

    xlabel      — opis osi X
    title_line1 — pierwsza linia tytułu wykresu
    """
    exceed_n   = int(np.sum(intervals > MAX_GAP_US))
    exceed_pct = 100.0 * exceed_n / len(intervals)

    x_max = max(MAX_GAP_US * 3, np.percentile(intervals, 99.9))
    plot_data = intervals[intervals <= x_max]
    clipped_n = len(intervals) - len(plot_data)

    fig, ax = plt.subplots(figsize=(13, 6))

    counts, edges, _ = ax.hist(
        plot_data, bins=300, color="steelblue", edgecolor="none", alpha=0.85
    )

    centers = (edges[:-1] + edges[1:]) / 2
    mask = counts > 0
    ax.scatter(
        centers[mask], counts[mask],
        color="crimson", s=18, zorder=5, linewidths=0,
        label="wierzchołek słupka",
    )

    ax.axvline(CHAR_TIME_US, color="green", lw=2, ls="--",
               label=f"Czas znaku 11 bit = {CHAR_TIME_US:.1f} µs")
    ax.axvline(MAX_GAP_US, color="red", lw=2, ls="--",
               label=f"Max gap MODBUS 1.5× = {MAX_GAP_US:.1f} µs")
    ax.axvline(np.mean(intervals), color="orange", lw=2, ls="-",
               label=f"Średnia = {np.mean(intervals):.1f} µs")

    subtitle = f"Scenariusz: {label}  |  " if label else ""
    ax.set_title(
        f"{title_line1}\n"
        f"115200 baud, 8E1  |  {subtitle}"
        f"N = {len(intervals)}  |  przekroczenia max gap: {exceed_n} szt. ({fmt_pct(exceed_pct)}%)"
        + (f"  (pominięto {clipped_n} wartości > {x_max:.0f} µs)" if clipped_n else ""),
        fontsize=12,
    )
    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel("Liczba wystąpień", fontsize=12)
    ax.legend(fontsize=11)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()

    if save:
        safe_label = label.replace(" ", "_").lower() if label else "base"
        fname = output_dir / f"histogram_{safe_label}.png"
        plt.savefig(fname, dpi=150)
        print(f"\nZapisano wykres: {fname}")

    plt.show()
