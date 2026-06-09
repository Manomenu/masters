#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// CRC-16/Modbus: poly 0x8005, reflected -> 0xA001, init 0xFFFF
// Table generated at compile time - fits in L1 cache (512 bytes)
struct CrcTable { uint16_t v[256]; };

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

// noinline: prevents the compiler from hoisting the n-iteration loop body into a single call
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
uint16_t crc16_modbus(const uint8_t* data, std::size_t len) noexcept {
    uint16_t crc = 0xFFFF;
    while (len--)
        crc = static_cast<uint16_t>((crc >> 8) ^ TABLE.v[(crc ^ *data++) & 0xFF]);
    return crc;
}

static void print_separator() {
    std::cout << "---------------------------------\n";
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::cout << "=================================\n"
              << "  Modbus RTU CRC-16 Calculator   \n"
              << "=================================\n\n";

    // ---- input: hex bytes ----
    std::cout << "Hex bytes (space-separated or continuous, upper/lowercase, max 256):\n> ";
    std::string line;
    std::getline(std::cin, line);

    // Strips all whitespace to enable both "01 02 03" and "010203" formats
    std::string hex;
    hex.reserve(line.size());
    for (char c : line)
        if (c != ' ' && c != '\t') hex += c;

    if (hex.empty()) {
        std::cerr << "Error: no bytes entered.\n";
        return 1;
    }
    if (hex.size() % 2 != 0) {
        std::cerr << "Error: odd number of hex characters — bytes must be pairs (e.g. 0A, ff, FF).\n";
        return 1;
    }
    if (hex.size() > 512) {
        std::cerr << "Error: maximum 256 bytes allowed.\n";
        return 1;
    }

    std::vector<uint8_t> data;
    data.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        std::string pair = hex.substr(i, 2);
        try {
            std::size_t pos;
            unsigned long v = std::stoul(pair, &pos, 16);
            if (pos != 2) throw std::invalid_argument("");
            data.push_back(static_cast<uint8_t>(v));
        } catch (...) {
            std::cerr << "Error: '" << pair << "' is not a valid hex byte.\n";
            return 1;
        }
    }

    // ---- input: repetitions ----
    long long n = 0;
    std::cout << "Repetitions (1 - 1000000000):\n> ";
    if (!(std::cin >> n) || n < 1 || n > 1'000'000'000LL) {
        std::cerr << "Error: n must be in range 1..1000000000.\n";
        return 1;
    }

    // ---- benchmark ----
    std::cout << "\nComputing " << n << " iteration(s)...\n";

    volatile uint16_t sink = 0; // prevents dead-code elimination of the loop
    auto t0 = std::chrono::high_resolution_clock::now();
    for (long long i = 0; i < n; ++i)
        sink = crc16_modbus(data.data(), data.size());
    auto t1 = std::chrono::high_resolution_clock::now();

    const uint16_t crc = sink;
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // ---- output ----
    std::cout << '\n';
    print_separator();
    std::cout << std::hex << std::uppercase << std::setfill('0');
    std::cout << "CRC-16 (Modbus): "
              << std::setw(2) << (crc & 0xFF) << " "
              << std::setw(2) << (crc >> 8) << '\n'
              << "  (low byte first)\n";
    print_separator();
    std::cout << std::dec;
    std::cout << "Input bytes:   " << data.size()  << '\n';
    std::cout << "Repetitions:   " << n            << '\n';
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Total time:    " << ms           << " ms\n";
    if (n > 1) {
        const double per_ms = ms / static_cast<double>(n);
        std::cout << std::scientific << std::setprecision(3);
        std::cout << "Per iteration: " << per_ms << " ms";
        // human-readable units
        std::cout << std::fixed;
        if (per_ms < 1e-3) {
            std::cout << std::setprecision(3) << "  (" << per_ms * 1e6 << " ns)";
        } else if (per_ms < 1.0) {
            std::cout << std::setprecision(3) << "  (" << per_ms * 1e3 << " us)";
        } else {
            std::cout << std::setprecision(3) << "  (" << per_ms << " ms)";
        }
        std::cout << '\n';
    }
    print_separator();

    return 0;
}
