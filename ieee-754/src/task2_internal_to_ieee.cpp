#include "shared.h"

#include <iomanip>
#include <iostream>
#include <string>

// Zadanie 5.2: konwersja kodu wewnetrznego (32-bitowy kod binarny naturalny) na IEEE 754

static void run(uint32_t internal) {
    const uint32_t ieee = internal_to_ieee754(internal);

    print_separator();
    std::cout << std::hex << std::uppercase << std::setfill('0');
    std::cout << "Kod wewnetrzny:  0x" << std::setw(8) << internal << '\n';
    print_internal_bits(internal);
    std::cout << "\nIEEE 754:        0x" << std::setw(8) << ieee << '\n';
    print_ieee_bits(ieee);
    std::cout << std::dec << "\nWartosc:         " << bits_to_float(ieee) << '\n';
    print_separator();
}

static void print_usage(const char* prog) {
    std::cout << "Uzycie:\n"
              << "  " << prog << "                         tryb interaktywny\n"
              << "  " << prog << " <32 bity>               kod wewnetrzny -> IEEE 754\n\n"
              << "Wejscie: 32-bitowy kod binarny naturalny (spacje dozwolone)\n\n"
              << "Przyklady:\n"
              << "  " << prog << " 10000011000100000000000000000000\n"
              << "  " << prog << " \"1000 0011 0001 0000 0000 0000 0000 0000\"\n";
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    if (argc == 2) {
        const std::string arg(argv[1]);
        if (arg == "-h" || arg == "--help") { print_usage(argv[0]); return 0; }
        uint32_t v;
        if (!parse_binary32(arg, v)) return 1;
        run(v);
        return 0;
    }

    if (argc > 2) { print_usage(argv[0]); return 1; }

    // tryb interaktywny
    std::cout << "=================================\n"
              << "  Zadanie 5.2: internal -> IEEE  \n"
              << "=================================\n\n"
              << "Podaj 32-bitowy kod wewnetrzny (kod binarny naturalny, spacje dozwolone):\n> ";

    std::string s;
    std::getline(std::cin, s);
    uint32_t v;
    if (!parse_binary32(s, v)) return 1;
    run(v);
    return 0;
}
