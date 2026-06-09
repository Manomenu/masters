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

```
$ ./task3_ieee_to_internal 0x41100000

$ ./task3_ieee_to_internal 477FFF00
```

### Obsługiwane błędy

Program odrzuca wartości spoza zakresu kodu wewnętrznego:

| Przypadek            | Przykład wejścia | Komunikat błędu                          |
|----------------------|------------------|------------------------------------------|
| Liczba ujemna        | `0xBF800000`     | liczba ujemna: niedozwolona              |
| Inf / NaN            | `0x7F800000`     | Inf/NaN: brak reprezentacji              |
| Liczba zdenorm.      | `0x00400000`     | subnormal: poza zakresem                 |
| Wartość < 1.0        | `0x3F000000`     | wartość < 1: poza zakresem              |
| Wykładnik > 127      | `0xFF000000`     | wykładnik > 127: poza zakresem           |
