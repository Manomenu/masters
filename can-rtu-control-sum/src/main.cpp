#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

// CRC-15/CAN: poly 0x4599 (x^15+x^14+x^10+x^8+x^7+x^4+x^3+1), MSB-first, init 0
// Table: 256 entries × 2 bytes = 512 bytes — fits entirely in L1 cache
struct CrcTable { uint16_t v[256]; };

static constexpr uint16_t crc15_entry(uint8_t b) noexcept {
    uint16_t crc = 0;
    for (int i = 7; i >= 0; --i) {
        const uint8_t crcnxt = static_cast<uint8_t>(((crc >> 14) ^ ((b >> i) & 1u)) & 1u);
        crc = static_cast<uint16_t>((crc << 1) & 0x7FFFu);
        crc ^= static_cast<uint16_t>(crcnxt * 0x4599u);  // branchless: no misprediction
    }
    return crc;
}

static constexpr CrcTable make_table() noexcept {
    CrcTable t{};
    for (unsigned i = 0; i < 256; ++i)
        t.v[i] = crc15_entry(static_cast<uint8_t>(i));
    return t;
}

// Generated at compile time — zero runtime init cost, lives in .rodata
static constexpr CrcTable TABLE = make_table();

// Input: up to 96 bits packed MSB-first into bytes[].
// bytes[0] bit7 = first input bit, bytes[0] bit0 = eighth input bit, etc.
struct BitInput {
    uint8_t bytes[12]{};
    int     nbits = 0;
};

// noinline: prevents the compiler from collapsing the n-iteration benchmark loop
// into a single call (loop-invariant code motion across constant input).
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
uint16_t can_crc15(const BitInput& in) noexcept {
    uint16_t crc = 0;
    const int full = in.nbits / 8;
    const int rem  = in.nbits % 8;

    // Byte-at-a-time via lookup table
    // key = top-8-bits-of-register XOR input-byte; then shift register 8 left.
    for (int i = 0; i < full; ++i) {
        const uint8_t key = static_cast<uint8_t>(((crc >> 7) ^ in.bytes[i]) & 0xFFu);
        crc = static_cast<uint16_t>(((crc << 8) & 0x7FFFu) ^ TABLE.v[key]);
    }

    // Remaining bits (0–7): branchless bit-by-bit for the partial last byte
    if (rem) {
        const uint8_t last = in.bytes[full];
        for (int i = 7; i >= 8 - rem; --i) {
            const uint8_t crcnxt = static_cast<uint8_t>(((crc >> 14) ^ ((last >> i) & 1u)) & 1u);
            crc = static_cast<uint16_t>((crc << 1) & 0x7FFFu);
            crc ^= static_cast<uint16_t>(crcnxt * 0x4599u);
        }
    }
    return crc;
}

static void separator() { std::cout << "---------------------------------\n"; }

// Parse and validate a raw string of '0'/'1' characters (spaces ignored).
// Returns false and prints an error on failure.
static bool parse_bits(const std::string& raw, BitInput& out) {
    std::string bits;
    bits.reserve(raw.size());
    for (char c : raw) {
        if (c == '0' || c == '1') { bits += c; continue; }
        if (c == ' ' || c == '\t') continue;
        std::cerr << "Error: '" << c << "' is not a valid bit (use 0 or 1).\n";
        return false;
    }
    if (bits.empty())          { std::cerr << "Error: no bits entered.\n"; return false; }
    if (bits.size() > 96)      { std::cerr << "Error: maximum 96 bits allowed ("
                                            << bits.size() << " entered).\n"; return false; }
    out.nbits = static_cast<int>(bits.size());
    for (int i = 0; i < out.nbits; ++i)
        if (bits[i] == '1')
            out.bytes[i / 8] |= static_cast<uint8_t>(1u << (7 - (i % 8)));
    return true;
}

// Parse and validate a repetition count string.
static bool parse_n(const std::string& s, long long& out) {
    try {
        std::size_t pos;
        out = std::stoll(s, &pos);
        if (pos != s.size() || out < 1 || out > 1'000'000'000LL)
            throw std::invalid_argument("");
    } catch (...) {
        std::cerr << "Error: repetitions must be an integer in range 1..1000000000.\n";
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    // ---- argument mode: ./can_crc <bits> [repetitions] ----
    if (argc >= 2) {
        const std::string arg1(argv[1]);
        if (arg1 == "-h" || arg1 == "--help") {
            std::cout << "Usage:\n"
                      << "  can_crc                    interactive mode\n"
                      << "  can_crc <bits>             compute CRC, 1 iteration\n"
                      << "  can_crc <bits> <n>         compute CRC, n iterations\n\n"
                      << "  bits  string of 0/1, max 96 characters\n"
                      << "  n     repetitions: 1 .. 1000000000\n";
            return 0;
        }

        BitInput inp;
        if (!parse_bits(arg1, inp)) return 1;

        long long n = 1;
        if (argc >= 3 && !parse_n(std::string(argv[2]), n)) return 1;

        volatile uint16_t sink = 0;
        const auto t0 = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < n; ++i)
            sink = can_crc15(inp);
        const auto t1 = std::chrono::high_resolution_clock::now();

        const uint16_t crc = sink;
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        separator();
        std::cout << std::hex << std::uppercase << std::setfill('0');
        std::cout << "CRC-15/CAN: 0x" << std::setw(4) << crc << '\n';
        separator();
        std::cout << std::dec;
        std::cout << "Input bits:    " << inp.nbits << '\n';
        std::cout << "Repetitions:   " << n         << '\n';
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Total time:    " << ms         << " ms\n";
        if (n > 1) {
            const double per_ms = ms / static_cast<double>(n);
            std::cout << std::scientific << std::setprecision(3);
            std::cout << "Per iteration: " << per_ms << " ms";
            std::cout << std::fixed;
            if (per_ms < 1e-3)
                std::cout << "  (" << std::setprecision(3) << per_ms * 1e6 << " ns)";
            else if (per_ms < 1.0)
                std::cout << "  (" << std::setprecision(3) << per_ms * 1e3 << " us)";
            else
                std::cout << "  (" << std::setprecision(3) << per_ms        << " ms)";
            std::cout << '\n';
        }
        separator();
        return 0;
    }

    // ---- interactive mode ----
    std::cout << "=================================\n"
              << "    CAN CRC-15 Calculator        \n"
              << "=================================\n\n";

    std::cout << "Bit string (0/1, max 96 bits, spaces allowed):\n> ";
    std::string line;
    std::getline(std::cin, line);

    BitInput inp;
    if (!parse_bits(line, inp)) return 1;

    long long n = 0;
    std::cout << "Repetitions (1 - 1000000000):\n> ";
    {
        std::string ns;
        std::cin >> ns;
        if (!parse_n(ns, n)) return 1;
    }

    // ---- benchmark ----
    std::cout << "\nComputing " << n << " iteration(s)...\n";

    volatile uint16_t sink = 0;  // volatile: prevents dead-code elimination of the loop
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (long long i = 0; i < n; ++i)
        sink = can_crc15(inp);
    const auto t1 = std::chrono::high_resolution_clock::now();

    const uint16_t crc = sink;
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // ---- output ----
    std::cout << '\n';
    separator();
    std::cout << std::hex << std::uppercase << std::setfill('0');
    std::cout << "CRC-15/CAN: 0x" << std::setw(4) << crc << '\n';
    separator();
    std::cout << std::dec;
    std::cout << "Input bits:    " << inp.nbits << '\n';
    std::cout << "Repetitions:   " << n         << '\n';
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Total time:    " << ms         << " ms\n";
    if (n > 1) {
        const double per_ms = ms / static_cast<double>(n);
        std::cout << std::scientific << std::setprecision(3);
        std::cout << "Per iteration: " << per_ms  << " ms";
        std::cout << std::fixed;
        if (per_ms < 1e-3)
            std::cout << "  (" << std::setprecision(3) << per_ms * 1e6 << " ns)";
        else if (per_ms < 1.0)
            std::cout << "  (" << std::setprecision(3) << per_ms * 1e3 << " us)";
        else
            std::cout << "  (" << std::setprecision(3) << per_ms        << " ms)";
        std::cout << '\n';
    }
    separator();

    return 0;
}
