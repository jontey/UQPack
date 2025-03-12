#ifndef DECODE_H
#define DECODE_H

#include <string>
#include <vector>
#include <utility>
#include <type_traits>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Forward declare CompressionType from encode.hpp
namespace UQPack {
    enum class CompressionType;
}

/**
 * UQDecode: Decode a URL-safe string back into the original data type.
 * This function reverses the encoding process performed by UQEncode.
 * It parses the header to determine the encoding parameters, validates the checksum,
 * and converts the encoded string back to the specified type.
 * 
 * @tparam T The type to decode into (e.g., std::string, json, std::vector<std::uint8_t>)
 * @param encodedString The URL-safe encoded string to decode
 * @return The decoded value of type T
 * @throws std::runtime_error if decoding fails or checksum validation fails
 */
namespace UQPack {
    // Helper type for static_assert
    template<typename T>
    struct always_false : std::false_type {};

    // Forward declare internal decode function
    std::vector<std::uint8_t> decodeInternal(const std::string& encodedString, CompressionType& outCompressionType, bool& outUseMessagePack);
    
    // Main decode function template
    template<typename T>
    inline T decode(const std::string& encodedString) {
        CompressionType compressionType;
        bool useMessagePack;
        std::vector<std::uint8_t> decodedData = decodeInternal(encodedString, compressionType, useMessagePack);
        
        if constexpr (std::is_same_v<T, std::vector<std::uint8_t>>) {
            return decodedData;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return std::string(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());
        } else if constexpr (std::is_same_v<T, json>) {
            if (useMessagePack) {
                return json::from_msgpack(decodedData);
            }
            return json::parse(decodedData.begin(), decodedData.end());
        } else {
            static_assert(always_false<T>::value, "Unsupported decode type");
        }
    }
}

#endif // DECODE_H
