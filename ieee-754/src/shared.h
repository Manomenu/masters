#pragma once
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// Internal "unique" format (32-bit) -- a faithful analogue of IEEE 754 single
// precision, differing ONLY in the two parameters dictated by the task:
//
//   bits 31-24 : 8-bit biased exponent  E_b   (bias = 128, IEEE uses 127)
//   bit  23    : sign S      (IEEE keeps the sign at bit 31)
//   bits 22-0  : 23-bit mantissa M           (identical field to IEEE)
//
// Every class of numbers and every reserved code mirrors IEEE 754 exactly:
//   E_b == 0,   M == 0 : signed zero      (+0 = 0x00000000, -0 = 0x00800000)
//   E_b == 0,   M != 0 : subnormal,  (-1)^S * (M/2^23) * 2^(1-128)
//   E_b in [1,254]     : normalized, (-1)^S * (1 + M/2^23) * 2^(E_b-128)
//   E_b == 255, M == 0 : +/- infinity
//   E_b == 255, M != 0 : NaN
//
// Because the only numeric difference is the exponent bias (128 vs 127), the
// value of a word equals the IEEE value of the SAME fields divided by two:
//
//     value_internal(X) = value_ieee( sign|exp|mant of X, IEEE layout ) / 2
//
// Hence conversion is exact and handles every class (signed zero, subnormals,
// Inf/NaN, overflow -> Inf, underflow -> subnormal/0) by reusing IEEE float
// arithmetic: relocate the sign bit and multiply / divide the value by two.
//
// All sample numbers from the task statement are positive integers, so they
// have S = 0 (bit 23 = 0) and E_b >= 128 (bit 31 = 1), which is exactly why
// bit 31 looked like a constant leading '1' in the original interpretation.
// ---------------------------------------------------------------------------

// Core conversions (total functions: every input maps to a valid output).
uint32_t internal_to_ieee754(uint32_t internal) noexcept;
uint32_t ieee754_to_internal(uint32_t ieee)     noexcept;

// Decoded numeric value of each format (for display).
double internal_to_value(uint32_t internal) noexcept;
double ieee_to_value(uint32_t ieee)         noexcept;

// Bit/float reinterpretation helpers
float    bits_to_float(uint32_t v) noexcept;
uint32_t float_to_bits(float f)    noexcept;

// Display helpers
void print_separator();
void print_ieee_bits(uint32_t v);
void print_internal_bits(uint32_t v);

// Input parsers : return false and print error on failure
bool parse_binary32(const std::string& s, uint32_t& out); // 32-bit natural binary string
bool parse_hex32   (const std::string& s, uint32_t& out); // 32-bit hex value (0x... or bare)
