// Weryfikacja: czy odwzorowanie (b15,b16) -> CRC jest bijekcja
// dla kazdego z 65536 mozliwych stanow startowych rejestru CRC.
#include <cstdint>
#include <cstdio>

// Tablica CRC-16/Modbus (poly 0xA001, init 0xFFFF)
struct CrcTable { uint16_t v[256]; };
constexpr CrcTable make_table() noexcept {
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
static constexpr CrcTable T = make_table();

inline uint16_t step(uint16_t crc, uint8_t b) noexcept {
    return static_cast<uint16_t>((crc >> 8) ^ T.v[(crc ^ b) & 0xFF]);
}

int main() {
    // seen[result] = 1 jesli wynik juz wystapil dla biezacego stanu R
    static uint8_t seen[65536];

    int failed = 0;
    for (int R = 0; R < 65536; ++R) {
        // wyczysc tablice
        for (int i = 0; i < 65536; ++i) seen[i] = 0;

        for (int val = 0; val < 65536; ++val) {
            uint8_t b15 = static_cast<uint8_t>(val >> 8);
            uint8_t b16 = static_cast<uint8_t>(val & 0xFF);
            uint16_t result = step(step(static_cast<uint16_t>(R), b15), b16);
            if (seen[result]) { ++failed; break; }
            seen[result] = 1;
        }
    }

    printf("Sprawdzono stanow: 65536\n");
    printf("Stanow z kolizja:  %d\n", failed);
    printf("Bijekcja dla wszystkich stanow: %s\n", failed == 0 ? "TAK" : "NIE");
    return failed == 0 ? 0 : 1;
}
