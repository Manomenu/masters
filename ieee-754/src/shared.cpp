#include "shared.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>

static constexpr uint32_t MANTISSA_MASK = 0x007FFFFFu;
static constexpr uint32_t INTERNAL_SIGN = 0x00800000u; // sign bit (bit 23)

// ---------------------------------------------------------------------------
// Bit / float reinterpretation
// ---------------------------------------------------------------------------

float bits_to_float(uint32_t v) noexcept {
    float f;
    __builtin_memcpy(&f, &v, sizeof f);
    return f;
}

uint32_t float_to_bits(float f) noexcept {
    uint32_t v;
    __builtin_memcpy(&v, &f, sizeof v);
    return v;
}

// ---------------------------------------------------------------------------
// Core conversions
//
// The internal format is IEEE 754 with bias 128 and the sign bit moved to
// bit 23. The only numeric difference (bias 128 vs 127) means that a word
// represents HALF the value of the IEEE word built from the same fields:
//
//     value_internal(X) = value_ieee(repack X to IEEE layout) / 2
//
// So we repack the sign/exponent/mantissa into the other layout and scale the
// value by two. Doing the scaling with real IEEE float arithmetic reproduces
// IEEE behaviour for every class: signed zero, subnormals, Inf/NaN, overflow
// (-> Inf) and underflow (-> subnormal / 0).
// ---------------------------------------------------------------------------

uint32_t internal_to_ieee754(uint32_t internal) noexcept {
    const uint32_t S    = (internal & INTERNAL_SIGN) ? 1u : 0u;
    const uint32_t E_b  = (internal >> 24u) & 0xFFu;
    const uint32_t mant = internal & MANTISSA_MASK;

    // Same fields in IEEE layout (sign 31, exp 30-23, mantissa 22-0).
    const uint32_t as_ieee = (S << 31u) | (E_b << 23u) | mant;

    // value_internal = value_ieee(as_ieee) / 2  (bias 128 -> 127).
    return float_to_bits(bits_to_float(as_ieee) * 0.5f);
}

uint32_t ieee754_to_internal(uint32_t ieee) noexcept {
    // value_ieee(as_ieee) = 2 * value_ieee(input)  (bias 127 -> 128).
    const uint32_t as_ieee = float_to_bits(bits_to_float(ieee) * 2.0f);

    const uint32_t S    = (as_ieee >> 31u) & 1u;
    const uint32_t E_b  = (as_ieee >> 23u) & 0xFFu;
    const uint32_t mant = as_ieee & MANTISSA_MASK;

    // Internal layout: exponent in bits 31-24, sign moved to bit 23.
    return (E_b << 24u) | (S << 23u) | mant;
}

// ---------------------------------------------------------------------------
// Decoded numeric values (for display)
// ---------------------------------------------------------------------------

double internal_to_value(uint32_t internal) noexcept {
    const uint32_t S    = (internal & INTERNAL_SIGN) ? 1u : 0u;
    const uint32_t E_b  = (internal >> 24u) & 0xFFu;
    const uint32_t mant = internal & MANTISSA_MASK;
    const uint32_t as_ieee = (S << 31u) | (E_b << 23u) | mant;
    return static_cast<double>(bits_to_float(as_ieee)) * 0.5; // value / 2
}

double ieee_to_value(uint32_t ieee) noexcept {
    return static_cast<double>(bits_to_float(ieee));
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

void print_separator() { std::cout << "---------------------------------\n"; }

void print_ieee_bits(uint32_t v) {
    std::cout << "  Bit  [31]    = " << ((v >> 31) & 1) << " (znak)\n";
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
                  << " ze stala, E = " << std::dec << (exp_b - 127)
                  << std::hex << std::uppercase << std::setfill('0') << ")\n";
    std::cout << std::dec << "  Bits [22:0]  = ";
    for (int i = 22; i >= 0; --i) std::cout << ((v >> i) & 1);
    std::cout << " (mantysa)\n";
}

void print_internal_bits(uint32_t v) {
    const int exp_b = static_cast<int>((v >> 24) & 0xFF);
    std::cout << "  Bits [31:24] = ";
    for (int i = 31; i >= 24; --i) std::cout << ((v >> i) & 1);
    if (exp_b == 0)
        std::cout << " (0 = zero/zdenorm.)\n";
    else if (exp_b == 255)
        std::cout << " (255 = Inf/NaN)\n";
    else
        std::cout << std::dec << " (0x" << std::hex << std::uppercase << exp_b
                  << " ze stala 128, E = " << std::dec << (exp_b - 128) << ")\n";
    std::cout << std::dec << "  Bit  [23]    = " << ((v >> 23) & 1) << " (znak)\n";
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
