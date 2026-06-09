#pragma once
#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// Internal "unique" format (32-bit, non-negative numbers only):
//   bit  31    : 1 = positive non-zero (explicit leading '1' of mantissa)
//   bits 30-24 : 7-bit unbiased exponent  E in [0, 127]
//   bit  23    : 0 (padding)
//   bits 22-0  : 23-bit fractional mantissa (identical to IEEE 754 field)
//
// Value = (1 + M * 2^-23) * 2^E  for non-zero;  0x00000000 encodes zero.
// ---------------------------------------------------------------------------

// Core conversions
uint32_t internal_to_ieee754(uint32_t internal) noexcept;
uint32_t ieee754_to_internal(uint32_t ieee)     noexcept;
// Display helpers
void print_separator();
void print_ieee_bits(uint32_t v);
void print_internal_bits(uint32_t v);
float bits_to_float(uint32_t v) noexcept;

// Input parsers : return false and print error on failure
bool parse_binary32(const std::string& s, uint32_t& out); // 32-bit natural binary string
bool parse_hex32   (const std::string& s, uint32_t& out); // 32-bit hex value (0x... or bare)
