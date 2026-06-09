#include "shared.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

static constexpr uint32_t INTERNAL_SIGN_BIT = 0x80000000u;
static constexpr uint32_t MANTISSA_MASK     = 0x007FFFFFu;
static constexpr uint32_t IEEE_EXP_BIAS     = 127u;

// ---------------------------------------------------------------------------
// Core conversions
// ---------------------------------------------------------------------------

uint32_t internal_to_ieee754(uint32_t internal) noexcept {
    if (internal == 0u) return 0u;
    const uint32_t E      = (internal >> 24u) & 0x7Fu;
    const uint32_t mant   = internal & MANTISSA_MASK;
    const uint32_t E_ieee = (E + IEEE_EXP_BIAS) & 0xFFu;
    return (E_ieee << 23u) | mant;
}

uint32_t ieee754_to_internal(uint32_t ieee) noexcept {
    if (ieee == 0u) return 0u;

    const uint32_t sign     = (ieee >> 31u) & 1u;
    const uint32_t E_biased = (ieee >> 23u) & 0xFFu;
    const uint32_t mant     = ieee & MANTISSA_MASK;

    if (sign != 0u) {
        std::cerr << "Blad: liczba ujemna : kod wewnetrzny obsluguje tylko wartosci nieujemne.\n";
        return 0xFFFFFFFFu;
    }
    if (E_biased == 0xFFu) {
        std::cerr << "Blad: Inf/NaN nie ma reprezentacji w kodzie wewnetrznym.\n";
        return 0xFFFFFFFFu;
    }
    if (E_biased == 0u) {
        std::cerr << "Blad: liczba zdenormalizowana : poza zakresem kodu wewnetrznego.\n";
        return 0xFFFFFFFFu;
    }

    const int32_t E = static_cast<int32_t>(E_biased) - static_cast<int32_t>(IEEE_EXP_BIAS);
    if (E < 0) {
        std::cerr << "Blad: wartosc < 1.0 : kod wewnetrzny obsluguje tylko wartosci >= 1.\n";
        return 0xFFFFFFFFu;
    }
    if (E > 127) {
        std::cerr << "Blad: wykladnik " << E << " > 127 : poza zakresem kodu wewnetrznego.\n";
        return 0xFFFFFFFFu;
    }

    return INTERNAL_SIGN_BIT | (static_cast<uint32_t>(E) << 24u) | mant;
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

void print_separator() { std::cout << "---------------------------------\n"; }

float bits_to_float(uint32_t v) noexcept {
    float f;
    __builtin_memcpy(&f, &v, sizeof f);
    return f;
}

void print_ieee_bits(uint32_t v) {
    std::cout << "  Bits [31]    = " << ((v >> 31) & 1) << " (znak)\n";
    std::cout << "  Bits [30:23] = ";
    for (int i = 30; i >= 23; --i) std::cout << ((v >> i) & 1);
    const int exp_b = static_cast<int>((v >> 23) & 0xFF);
    if (exp_b == 0)
        std::cout << " (0 = zero/zdenorm.)\n";
    else if (exp_b == 255)
        std::cout << " (255 = Inf/NaN)\n";
    else
        std::cout << std::hex << std::uppercase << std::setfill('0')
                  << " (0x" << std::setw(2) << exp_b
                  << " biased, E = " << std::dec << (exp_b - 127)
                  << std::hex << std::uppercase << std::setfill('0') << ")\n";
    std::cout << "  Bits [22:0]  = ";
    for (int i = 22; i >= 0; --i) std::cout << ((v >> i) & 1);
    std::cout << " (mantysa)\n";
}

void print_internal_bits(uint32_t v) {
    std::cout << "  Bit  [31]    = " << ((v >> 31) & 1) << " (jedynka wiodaca)\n";
    std::cout << "  Bits [30:24] = ";
    for (int i = 30; i >= 24; --i) std::cout << ((v >> i) & 1);
    std::cout << std::dec << " (E = " << ((v >> 24) & 0x7Fu)
              << std::hex << std::uppercase << std::setfill('0') << ")\n";
    std::cout << "  Bit  [23]    = " << ((v >> 23) & 1) << " (padding)\n";
    std::cout << "  Bits [22:0]  = ";
    for (int i = 22; i >= 0; --i) std::cout << ((v >> i) & 1);
    std::cout << " (mantysa)\n";
}

// ---------------------------------------------------------------------------
// Input parsers
// ---------------------------------------------------------------------------

bool parse_hex32(const std::string& s, uint32_t& out) {
    try {
        std::size_t pos;
        const unsigned long v = std::stoul(s, &pos, 16);
        if (pos != s.size()) throw std::invalid_argument("");
        if (v > 0xFFFFFFFFul) throw std::out_of_range("");
        out = static_cast<uint32_t>(v);
    } catch (...) {
        std::cerr << "Blad: nieprawidlowa wartosc hex '" << s << "' (np. 0x41100000 lub 41100000).\n";
        return false;
    }
    return true;
}

bool parse_binary32(const std::string& s, uint32_t& out) {
    std::string bits;
    bits.reserve(32);
    for (char c : s) {
        if (c == '0' || c == '1') { bits += c; continue; }
        if (c == ' ' || c == '\t') continue;
        std::cerr << "Blad: niedozwolony znak '" << c << "' (tylko 0 i 1).\n";
        return false;
    }
    if (bits.size() != 32) {
        std::cerr << "Blad: wymagane dokladnie 32 bity (podano " << bits.size() << ").\n";
        return false;
    }
    out = 0;
    for (char b : bits) out = (out << 1) | static_cast<uint32_t>(b - '0');
    return true;
}


