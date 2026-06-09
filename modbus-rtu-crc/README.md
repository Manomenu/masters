# Modbus RTU CRC-16 Calculator

Aplikacja konsolowa wyznaczająca 16-bitową cykliczną sumę kontrolną (CRC-16)
zgodnie ze specyfikacją warstwy łącza danych sieci **Modbus RTU**.

## Uruchomienie

```bash
just build   # kompiluje dla aktualnej platformy (macOS ARM lub Windows)
just run     # build + uruchom
just clean   # usuń plik wykonywalny
```

Po uruchomieniu program pyta kolejno o:

1. **Bajty danych** w notacji hex (max 256 bajtów)
2. **Liczbę powtórzeń** algorytmu `n` (1 – 1 000 000 000)

### Przykłady poprawnego inputu

```
01 03 00 00 00 01        # bajty oddzielone spacjami
010300000001             # bajty bez spacji (ciągły zapis)
01 0300 0000 01          # mieszany — spacje są ignorowane
```

---

## Co wyświetla program

```
CRC-16 (Modbus):  0x0A84
  TX order: 84 0A  (low byte first)
Input bytes:   6
Repetitions:   1000000
Total time:    9.356 ms
Per iteration: 9.356E-06 ms  (9.356 ns)
```

### CRC-16 — wartość vs kolejność transmisji (TX order)

CRC-16 to liczba 16-bitowa, np. **`0x0A84`**. Jak każda liczba 16-bitowa
składa się z dwóch bajtów:

| Nazwa      | Bity  | Wartość w `0x0A84` |
|------------|-------|---------------------|
| High byte  | 15–8  | `0x0A`              |
| Low byte   |  7–0  | `0x84`              |

Gdy piszemy liczbę `0x0A84` w kodzie lub na wydruku, **high byte jest po
lewej** — bo tak zapisujemy liczby (podobnie jak w dziesiątkowym 1234 tysiąc
jest po lewej). To konwencja **big-endian**.

Specyfikacja Modbus RTU nakazuje jednak transmitować CRC w kolejności
**little-endian**: **najpierw low byte, potem high byte**. Oznacza to,
że `0x0A84` idzie na magistralę jako sekwencja `84 0A`.

Dlatego program wyświetla dwie rzeczy:

- **`CRC-16 (Modbus): 0x0A84`** — wartość liczbowa, high byte po lewej
  (standardowy zapis hex)
- **`TX order: 84 0A`** — kolejność bajtów w ramce na magistrali
  (**TX** od ang. *transmit*, skrót używany w telekomunikacji i elektronice
  na oznaczenie kierunku nadawania; analogicznie RX = *receive*);
  low byte pierwszy, tak jak wymaga Modbus RTU

Odbiorca dołącza oba bajty do ramki dokładnie w tej kolejności (`84`, `0A`)
i weryfikuje CRC po odebraniu całej ramki.

---

## Algorytm CRC-16/Modbus

### Czym jest CRC i po co go liczyć?

CRC (Cyclic Redundancy Check) to suma kontrolna dołączana do ramki przez
nadajnik. Odbiornik, po odebraniu ramki, liczy CRC samodzielnie na podstawie
otrzymanych danych i porównuje z CRC przesłanym przez nadajnik. Niezgodność
oznacza błąd transmisji i ramka jest odrzucana.

CRC jest **znacznie silniejszą** ochroną niż prosta suma bajtów — wykrywa
wszystkie błędy 1-bitowe, wszystkie błędy 2-bitowe (dla dowolnej długości
ramki) oraz wszystkie ciągłe serie błędów o długości ≤ 16 bitów.

---

### Matematyczna idea: dzielenie wielomianów w GF(2)

CRC opiera się na arytmetyce wielomianów nad ciałem **GF(2)** (Galois Field),
gdzie wszystkie współczynniki wynoszą 0 lub 1 i działają modulo 2. Dodawanie
i odejmowanie to w GF(2) to samo co XOR; nie ma przeniesień ani pożyczek.

Dane wejściowe traktuje się jako jeden wielki wielomian nad GF(2). Dla
przykładu bajt `0b10110001` odpowiada wielomianowi:

```
x^7 + x^5 + x^4 + x^0  =  x^7 + x^5 + x^4 + 1
```

Dla ciągu `n` bajtów danych otrzymujemy wielomian stopnia `8n - 1`.
CRC to **reszta z dzielenia** tego wielomianu przez ustalony **wielomian
generujący** (generator polynomial) stopnia 16. Wynik ma zawsze dokładnie
16 bitów — stąd nazwa CRC-16.

```
CRC = (Dane × x^16) mod Generator
```

Mnożenie przez `x^16` to przesunięcie danych o 16 bitów w lewo —
robimy miejsce na 16-bitową resztę. Dzielenie przez generator to
wielokrotne XOR-owanie.

---

### Krok po kroku — algorytm bitowy

Dla CRC-16/Modbus generator to `0x8005` = `1000000000000101` w binarnym.

Algorytm naiwny przetwarza dane **bit po bicie**, utrzymując 16-bitowy
rejestr CRC zainicjowany wartością `0xFFFF`:

```
dla każdego bajtu danych:
    XOR bajt z rejestrem CRC (młodszy bajt rejestru)
    dla każdego z 8 bitów bajtu:
        jeśli LSB rejestru == 1:
            przesuń rejestr w prawo o 1 bit
            XOR rejestr z generatorem 0xA001   ← odbita postać 0x8005
        w przeciwnym razie:
            przesuń rejestr w prawo o 1 bit
```

Przesuwamy w prawo (zamiast w lewo), bo używamy **odbitej (reflected)**
postaci generatora `0xA001` — to ekwiwalent przetwarzania bitów od LSB,
co jest naturalne przy transmisji szeregowej (UART wysyła LSB jako pierwszy).

Po przetworzeniu wszystkich bajtów rejestr zawiera wynik CRC.

Dla każdego bajtu danych wykonujemy **8 iteracji** tej pętli wewnętrznej.

---

### Parametry CRC-16/Modbus

| Parametr         | Wartość   | Opis                                        |
|------------------|-----------|---------------------------------------------|
| Wielomian        | `0x8005`  | Generator polynomial                        |
| Wielomian (odw.) | `0xA001`  | Postać reflected używana w implementacji    |
| Wartość startowa | `0xFFFF`  | Inicjalna wartość rejestru CRC              |
| Odbicie wejścia  | tak       | Bity każdego bajtu wejściowego są odbijane  |
| Odbicie wyjścia  | tak       | Bity wyniku końcowego są odbijane           |

Inicjalizacja `0xFFFF` (zamiast `0x0000`) sprawia, że ciąg samych zer
na początku ramki ma wpływ na CRC — bez tego nadajnik mógłby "darmowo"
doklejać zera na początku bez zmiany sumy kontrolnej.

---

## Zastosowane optymalizacje

### 1. Table lookup zamiast przetwarzania bit po bicie

**Plik:** `src/main.cpp`, linie 12–26 i 38

Naiwna implementacja CRC-16 wykonuje 8 operacji bitowych dla każdego bajtu
danych. Table lookup redukuje to do **jednego odczytu tablicy i dwóch operacji
bitowych** niezależnie od wartości bajtu.

```cpp
// 256-elementowa tablica — jedno wyszukanie na bajt wejściowy
crc = (crc >> 8) ^ TABLE.v[(crc ^ *data++) & 0xFF];
```

Dla ramki o długości `L` bajtów algorytm naiwny wykonuje `8·L` iteracji,
a table lookup — dokładnie `L` iteracji. Dla `n = 10^9` powtórzeń i `L = 10`
daje to **8× mniej operacji** w pętli wewnętrznej.

---

### 2. Generowanie tablicy w czasie kompilacji (`constexpr`)

**Plik:** `src/main.cpp`, linie 14–26

```cpp
constexpr CrcTable make_crc_table() noexcept {
    CrcTable t{};
    for (unsigned i = 0; i < 256; ++i) {
        uint16_t v = static_cast<uint16_t>(i);
        for (int j = 0; j < 8; ++j)
            v = (v & 1u) ? static_cast<uint16_t>((v >> 1) ^ 0xA001u)
                         : static_cast<uint16_t>(v >> 1);
        t.v[i] = v;
    }
    return t;
}

static constexpr CrcTable TABLE = make_crc_table();
```

Słowo kluczowe `constexpr` nakazuje kompilatorowi wykonać całą funkcję
podczas kompilacji. W rezultacie gotowa tablica trafia bezpośrednio do sekcji
`.rodata` pliku wykonywalnego — **zero kosztu w czasie uruchomienia**,
zero kodu inicjalizującego, zero gałęzi warunkowych przy starcie programu.

---

### 3. Dopasowanie tablicy do pamięci podręcznej L1

**Rozmiar:** 256 × 2 B = **512 bajtów**

Tablica zajmuje dokładnie 512 bajtów, co jest ułamkiem typowej pamięci
podręcznej L1 (32–64 KB). Po pierwszym przebiegu pętli cała tablica
rezyduje w L1, a każde kolejne wyszukanie jest obsługiwane bez odczytu
z RAM ani L2. Czas dostępu do L1 wynosi ~4 cykle CPU, podczas gdy
dostęp do RAM to ~200–300 cykli.

Dla porównania, tablice 4-bitowego (Slicing-by-4) lub 16-bitowego CRC
zajmują odpowiednio 32 KB i 128 KB i nie mieszczą się w L1.

---

### 4. Flagi kompilatora: `-O3 -march=native`

**Plik:** `justfile`

```
clang++ -std=c++17 -O3 -march=native -o modbus-crc src/main.cpp
```

- **`-O3`** — włącza agresywną optymalizację: rozwijanie pętli (loop
  unrolling), eliminację zbędnych skoków, wektoryzację, inlining funkcji
  pomocniczych i reordering instrukcji pod pipeline CPU.
- **`-march=native`** — generuje instrukcje specyficzne dla procesora,
  na którym odbywa się kompilacja (np. NEON/SVE na Apple Silicon,
  AVX2 na x86-64). Kompilator może wówczas np. załadować kilka bajtów
  naraz i rozwinąć pętlę CRC, zamiast przetwarzać bajt po bajcie.

---

### 5. Zapobieganie eliminacji pętli przez kompilator — `noinline`

**Plik:** `src/main.cpp`, linie 30–34

```cpp
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
uint16_t crc16_modbus(const uint8_t* data, std::size_t len) noexcept { ... }
```

Przy `-O3` kompilator może zastosować **loop-invariant code motion (LICM)**:
wykryć, że wejście pętli benchmarkowej (`data`, `len`) się nie zmienia,
i zamienić `n` wywołań na jedno, zapisując wynik w rejestrze. Benchmark
pokazałby wtedy czas jednej iteracji zamiast `n`.

`noinline` uniemożliwia to: kompilator nie może zajrzeć do wnętrza funkcji
przez granicę linkowania i musi wygenerować `n` rzeczywistych wywołań.

---

### 6. Zapobieganie eliminacji pętli przez kompilator — `volatile`

**Plik:** `src/main.cpp`, linia 104

```cpp
volatile uint16_t sink = 0;
for (long long i = 0; i < n; ++i)
    sink = crc16_modbus(data.data(), data.size());
```

Zapis do zmiennej `volatile` jest przez standard C++ traktowany jako
**obserwowalny efekt uboczny** — kompilator nie może go usunąć ani
przenieść poza pętlę. Nawet gdyby `noinline` został zignorowany (np. przy
LTO), `volatile` gwarantuje wykonanie wszystkich `n` iteracji.

`noinline` i `volatile` działają jako dwa niezależne, wzajemnie uzupełniające
się zabezpieczenia integralności benchmarku.

---

### 7. `noexcept` na funkcji CRC

**Plik:** `src/main.cpp`, linia 35

```cpp
uint16_t crc16_modbus(const uint8_t* data, std::size_t len) noexcept { ... }
```

`noexcept` informuje kompilator, że funkcja nigdy nie rzuca wyjątków.
W praktyce eliminuje to generowanie kodu obsługi wyjątków (tabele EH,
stack unwinding) wewnątrz i dookoła funkcji. Dla gorącej pętli
wywoływanej `n` razy oznacza to czystszy kod maszynowy i lepsze
wykorzystanie instruction cache.

---

### 8. Efektywna pętla: `while (len--)` i `*data++`

**Plik:** `src/main.cpp`, linia 37–38

```cpp
while (len--)
    crc = static_cast<uint16_t>((crc >> 8) ^ TABLE.v[(crc ^ *data++) & 0xFF]);
```

- `while (len--)` — warunek pętli i dekrement w jednej operacji; kompilator
  generuje pojedynczą instrukcję `SUBS`/`DEC` + skok warunkowy.
- `*data++` — inkrementacja wskaźnika składana z ładowaniem bajtu w jedno
  instrukcję przez kompilator (np. `LDRB` z post-increment na ARM).
- Brak indeksowania tablicowego `data[i]` — eliminuje dodatkowe obliczanie
  adresu `base + i * sizeof(T)` w każdej iteracji.

---

### 9. Dane wejściowe w `std::vector` z `reserve`

**Plik:** `src/main.cpp`, linia 78–79

```cpp
std::vector<uint8_t> data;
data.reserve(hex.size() / 2);
```

`reserve` alokuje pamięć z góry, zanim pętla parsowania doda bajty przez
`push_back`. Bez `reserve` vector realokuje się (i kopiuje dane) za każdym
razem, gdy przekroczy pojemność. Dla 256 bajtów unika to do 8 zbędnych
realokacji podczas wczytywania inputu.

---

### Podsumowanie

| # | Optymalizacja             | Efekt                                          |
|---|---------------------------|------------------------------------------------|
| 1 | Table lookup              | 8× mniej operacji na bajt vs. bit-by-bit       |
| 2 | `constexpr` table         | Zero kosztu inicjalizacji w runtime            |
| 3 | Tablica 512 B w L1        | Brak cache miss po rozgrzaniu pętli            |
| 4 | `-O3 -march=native`       | Unrolling, wektoryzacja, instrukcje natywne    |
| 5 | `noinline`                | Wymusza `n` prawdziwych wywołań (benchmark)    |
| 6 | `volatile sink`           | Zapobiega eliminacji pętli (benchmark)         |
| 7 | `noexcept`                | Brak kodu obsługi wyjątków w gorącej ścieżce  |
| 8 | `while(len--)` / `*data++`| Minimalna liczba instrukcji na iterację        |
| 9 | `vector::reserve`         | Brak realokacji przy wczytywaniu inputu        |
