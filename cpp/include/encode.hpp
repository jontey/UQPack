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
        ZSTD = 2
    };

    /**
     * Low-level encode function: Encode binary data into a URL-safe string.
     * For simplicity, we use two hardcoded charsets.
     * The encoding converts the binary data (treated as a big integer) to a string,
     * adds a header ("PX" + compression flag + encoding/cipher flag) and a checksum.
     * 
     * @param data The binary data to encode
     * @param compressionType Type of compression used (NONE, LZ4, ZSTD)
     * @param useMessagePack Flag indicating if MessagePack was used
     * @param baseIndex The index of the character set to use (0 for Base64, 1 for Base70)
     * @return URL-safe encoded string
     */
    std::string encode(const std::vector<std::uint8_t>& data, CompressionType compressionType, bool useMessagePack, int baseIndex);
    
    /**
     * Encode JSON data into a URL-safe string.
     * This function handles the MessagePack serialization and compression internally.
     * 
     * @param jsonData The JSON data to encode
     * @param compressionType Type of compression to use (default: LZ4)
     * @param useMessagePack Whether to use MessagePack serialization (default: true)
     * @param baseIndex The index of the character set to use (default: auto-selected based on input)
     * @return URL-safe encoded string
     */
    std::string encode(const json& jsonData, CompressionType compressionType = CompressionType::LZ4, bool useMessagePack = true, int baseIndex = -1);

    // Compression functions
    std::vector<std::uint8_t> compressWithLZ4(const std::uint8_t* data, size_t dataSize);
    std::vector<std::uint8_t> compressWithZstd(const std::uint8_t* data, size_t dataSize);
}

#endif // ENCODE_H
