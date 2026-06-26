#include "shared.h"

#include <iomanip>
#include <iostream>
#include <string>

// Zadanie 5.3: konwersja kodu IEEE 754 (hex) na kod wewnetrzny (hex)

static void run(uint32_t ieee) {
    const uint32_t internal = ieee754_to_internal(ieee);

    print_separator();
    std::cout << std::hex << std::uppercase << std::setfill('0');
    std::cout << "IEEE 754:        0x" << std::setw(8) << ieee << '\n';
    print_ieee_bits(ieee);
    std::cout << std::hex << std::uppercase << std::setfill('0')
              << "\nKod wewnetrzny:  0x" << std::setw(8) << internal << '\n';
    print_internal_bits(internal);
    std::cout << std::dec << "\nWartosc:         " << ieee_to_value(ieee) << '\n';
    print_separator();
}

static void print_usage(const char* prog) {
    std::cout << "Uzycie:\n"
              << "  " << prog << "           tryb interaktywny\n"
              << "  " << prog << " <hex>     IEEE 754 (hex) -> kod wewnetrzny (hex)\n\n"
              << "Wejscie: 32-bitowa wartosc szesnastkowa (z lub bez przedrostka 0x)\n\n"
              << "Przyklady:\n"
              << "  " << prog << " 0x41100000\n"
              << "  " << prog << " 477FFF00\n"
              << "  " << prog << " 0x3F800000\n";
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    if (argc == 2) {
        const std::string arg(argv[1]);
        if (arg == "-h" || arg == "--help") { print_usage(argv[0]); return 0; }
        uint32_t bits;
        if (!parse_hex32(arg, bits)) return 1;
        run(bits);
        return 0;
    }

    if (argc > 2) { print_usage(argv[0]); return 1; }

    // tryb interaktywny
    std::cout << "=================================\n"
              << "  Zadanie 5.3: IEEE -> internal  \n"
              << "=================================\n\n"
              << "Podaj 32-bitowy kod IEEE 754 w postaci szesnastkowej\n"
              << "(np. 0x41100000 lub 41100000):\n> ";

    std::string s;
    std::getline(std::cin, s);
    uint32_t bits;
    if (!parse_hex32(s, bits)) return 1;
    run(bits);
    return 0;
}
