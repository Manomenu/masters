# IEEE 754 — Projekt 5

Dwa programy do konwersji między unikalnym kodem "wewnętrznym" a standardem IEEE 754 single precision.

Opis wszystkich plików w projekcie: [OPIS_PLIKOW.txt](OPIS_PLIKOW.txt)

## Wymagania

| Narzędzie | Wersja | Uwagi |
|-----------|--------|-------|
| [just](https://github.com/casey/just) | ≥ 1.0 | runner budowania (`brew install just` / `cargo install just`) |
| clang++ lub g++ | ≥ C++17 | na macOS domyślnie clang++, na Linux g++ |
| System operacyjny | macOS lub Linux | na Windows wymaga MinGW/g++ |

## Budowanie

```
just build
```

Buduje oba programy jednocześnie. Można też budować osobno:

```
just build-task2
just build-task3
```

Po zbudowaniu powstają pliki wykonywalne w bieżącym katalogu:

| Plik | Opis |
|------|------|
| `task2_internal_to_ieee` | konwersja kod wewnętrzny → IEEE 754 |
| `task3_ieee_to_internal` | konwersja IEEE 754 → kod wewnętrzny |

Usunięcie plików wykonywalnych: `just clean`

---

## Task 2 — kod wewnętrzny → IEEE 754

**Wejście:** 32-bitowy ciąg w kodzie binarnym naturalnym (NKB).  
**Wyjście:** wzorzec bitowy IEEE 754 w postaci szesnastkowej.

### Użycie

```
./task2_internal_to_ieee <32 bity>
./task2_internal_to_ieee          # tryb interaktywny
```

Spacje w ciągu bitów są dozwolone i ignorowane.

### Przykłady

| Liczba | Wejście (NKB)                      | Wyjście (IEEE 754 hex) |
|-------:|------------------------------------|------------------------|
| 0      | `00000000000000000000000000000000` | `0x00000000`           |
| 1      | `10000000000000000000000000000000` | `0x3F800000`           |
| 9      | `10000011000100000000000000000000` | `0x41100000`           |
| 65535  | `10001111011111111111111100000000` | `0x477FFF00`           |
| 65536  | `10010000000000000000000000000000` | `0x47800000`           |

```
$ ./task2_internal_to_ieee 10000011000100000000000000000000

$ ./task2_internal_to_ieee "1000 0011 0001 0000 0000 0000 0000 0000"
```

---

## Task 3 — IEEE 754 → kod wewnętrzny

**Wejście:** wzorzec bitowy IEEE 754 w postaci szesnastkowej (z `0x` lub bez).  
**Wyjście:** wzorzec bitowy kodu wewnętrznego w postaci szesnastkowej.

### Użycie

```
./task3_ieee_to_internal <hex>
./task3_ieee_to_internal          # tryb interaktywny
```

### Przykłady

| Liczba | Wejście (IEEE 754 hex) | Wyjście (kod wewnętrzny hex) |
|-------:|------------------------|------------------------------|
| 0      | `0x00000000`           | `0x00000000`                 |
| 1      | `0x3F800000`           | `0x80000000`                 |
| 9      | `0x41100000`           | `0x83100000`                 |
| 65535  | `0x477FFF00`           | `0x8F7FFF00`                 |
| 65536  | `0x47800000`           | `0x90000000`                 |
| 0.5    | `0x3F000000`           | `0x7F000000`                 |
| −9     | `0xC1100000`           | `0x83900000`                 |
| π      | `0x40490FDB`           | `0x81490FDB`                 |

```
$ ./task3_ieee_to_internal 0x41100000   # 9
$ ./task3_ieee_to_internal 0x3F000000   # 0.5
$ ./task3_ieee_to_internal 0xC1100000   # -9
```

Kod wewnętrzny jest **wiernym analogiem IEEE 754**: ma te same klasy liczb
i te same wartości zarezerwowane, różni się tylko nadmiarem (128 zamiast 127)
i położeniem bitu znaku (bit 23 zamiast 31). Obsługuje więc, dokładnie jak IEEE:
liczby ujemne, ułamki < 1, **zero ze znakiem** (`+0 = 0x00000000`,
`−0 = 0x00800000`), liczby zdenormalizowane oraz Inf/NaN.

### Zachowanie dla przypadków szczególnych

Konwersja jest **totalna** (każda wartość ma odpowiednik) i odwzorowuje
semantykę IEEE 754:

| Wejście                          | Wynik                                   |
|----------------------------------|-----------------------------------------|
| `±0`, liczba (de)znormalizowana  | ta sama wartość, znak zachowany         |
| `±∞`, NaN                        | `±∞`, NaN                               |
| `\|x\| ≥ 2¹²⁷` (poza zakresem)    | nadmiar → `±∞` (jak zawężanie w IEEE)   |
| `\|x\| < 2⁻¹⁵⁰`                   | niedomiar → `±0` (jak w IEEE)           |

Wynika to z tożsamości `wartość_wewn = wartość_IEEE / 2` (różnica nadmiaru o 1).
Implementacja realizuje konwersję przez przeniesienie bitu znaku i mnożenie/
dzielenie wartości przez 2 — sprzętowa arytmetyka `float` sama poprawnie
obsługuje wszystkie klasy liczb.
