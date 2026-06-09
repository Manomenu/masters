# CAN CRC-15 Calculator

Kalkulator sumy kontrolnej CRC-15/CAN zgodny z algorytmem z normy CAN 2.0.
Projekt 4 — Systemy czasu rzeczywistego, EiTI PW.

## Co robi

- Wczytuje ciąg do **96 bitów** z klawiatury
- Wyznacza **CRC-15/CAN** (wielomian `0x4599`) i wyświetla wynik w hex
- Mierzy czas obliczeń dla zadanej liczby powtórzeń (1 – 10⁹)

Implementacja zoptymalizowana: tablica przeglądowa 256 × 2 B generowana
w czasie kompilacji (`constexpr`), przetwarzanie bajt po bajcie (8× szybsze
niż bit po bicie), kod bezgałęziowy w pętli wewnętrznej.

## Budowanie

Wymagane: `clang++` (lub `g++` na Windows), [`just`](https://just.systems).

```
just build   # kompilacja
just run     # kompilacja + uruchomienie
just clean   # usuwa plik wykonywalny
```

## Uruchomienie

**Tryb argumentów** (nie wymaga interakcji):

```
./can_crc <bits> [repetitions] # na windows can_crc.exe
```

```
$ ./can_crc 10110100
---------------------------------
CRC-15/CAN: 0x0C65
---------------------------------
Input bits:    8
Repetitions:   1
Total time:    0.000 ms
---------------------------------

$ ./can_crc 10110100 1000000000
---------------------------------
CRC-15/CAN: 0x0C65
---------------------------------
Input bits:    8
Repetitions:   1000000000
Total time:    297.766 ms
Per iteration: 2.978E-07 ms  (0.298 ns)
---------------------------------

$ ./can_crc --help
Usage:
  can_crc                    interactive mode
  can_crc <bits>             compute CRC, 1 iteration
  can_crc <bits> <n>         compute CRC, n iterations
```

**Tryb interaktywny** (bez argumentów):

```
$ ./can_crc
=================================
    CAN CRC-15 Calculator
=================================

Bit string (0/1, max 96 bits, spaces allowed):
> 10110100 01110010
Repetitions (1 - 1000000000):
> 1000000000

Computing 1000000000 iteration(s)...

---------------------------------
CRC-15/CAN: 0x7747
---------------------------------
Input bits:    16
Repetitions:   1000000000
Total time:    319.483 ms
Per iteration: 3.195E-07 ms  (0.320 ns)
---------------------------------
```

Spacje w ciągu bitów są ignorowane — można grupować bity dla czytelności.

## Tło teoretyczne

Słowniczek pojęć CAN, struktura ramki, bit stuffing, teoria CRC-15 i szczegóły implementacyjne: [teoria.pdf](.raport/teoria.pdf)

## Wyniki benchmarku (Apple Silicon, `-O3 -march=native`)

| Wejście        | Bity | CRC-15/CAN | Czas / iterację |
|----------------|------|------------|-----------------|
| `00...0` (96×) |  96  | `0x0000`   | 0,320 ns        |
| `11...1` (96×) |  96  | `0x072A`   | 0,314 ns        |

