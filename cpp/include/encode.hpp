#ifndef ENCODE_H
#define ENCODE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Use nlohmann::json for JSON handling
using json = nlohmann::json;

namespace UQPack {
    // Compression types enum for easier selection
    enum class CompressionType {
        NONE = 0,
        LZ4 = 1,
        ZSTD = 2,
        BROTLI = 3
    };

    /**
     * Low-level encode function: Encode binary data into a URL-safe string.
     * For simplicity, we use two hardcoded charsets.
     * The encoding converts the binary data (treated as a big integer) to a string,
     * adds a header ("PX" + compression flag + encoding/cipher flag) and a checksum.
     * 
     * @param data The binary data to encode
     * @param compressionType Type of compression used (NONE, LZ4, ZSTD)
     * @param baseIndex The index of the character set to use (0 for Base64, 1 for Base70)
     * @return URL-safe encoded string
     */
    std::string encode(const std::vector<std::uint8_t>& data, CompressionType compressionType = CompressionType::ZSTD, int baseIndex = -1);
    
    // Encode JSON data with compression type
    std::string encode(const json& jsonData, CompressionType compressionType = CompressionType::ZSTD, int baseIndex = -1);

    // Compression functions
    std::vector<std::uint8_t> compressWithLZ4(const std::uint8_t* data, size_t dataSize);
    std::vector<std::uint8_t> compressWithZstd(const std::uint8_t* data, size_t dataSize);
    std::vector<std::uint8_t> compressWithBrotli(const std::uint8_t* data, size_t dataSize, int quality = 11);
}

#endif // ENCODE_H
