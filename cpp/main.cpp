#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <lz4.h>

// Use nlohmann::json (https://github.com/nlohmann/json)
using json = nlohmann::json;

// UQEncode: Encode binary data into a URL-safe string.
// For simplicity, we use three hardcoded charsets.
// The encoding converts the binary data (treated as a big integer) to a string,
// adds a header ("UQ" + compression flag + base index) and a checksum.
std::string UQEncode(const std::vector<char>& data, bool useLZ4, bool useMessagePack, int baseIndex) {
    // Define our URL-safe character sets.
    const std::vector<std::string> bases = {
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",         // Base62
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",         // Base64 URL-safe
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~"         // Base70
    };
    std::string charset = bases[baseIndex];
    const int base = charset.size();

    // Convert input data (vector<char>) to a vector of unsigned bytes.
    std::vector<unsigned char> input(data.begin(), data.end());
    
    // We'll perform a division algorithm on the input bytes (big-endian representation).
    // First, represent the input as a vector of digits (each digit is in base 256).
    std::vector<int> digits;
    for (unsigned char c : input) {
        digits.push_back(c);
    }

    std::string encoded = "";
    // Repeatedly divide the number (in base 256) by 'base'
    while (!digits.empty() && !(digits.size() == 1 && digits[0] == 0)) {
        int remainder = 0;
        std::vector<int> newDigits;
        for (int d : digits) {
            int num = remainder * 256 + d;
            int quotient = num / base;
            remainder = num % base;
            if (!(newDigits.empty() && quotient == 0))
                newDigits.push_back(quotient);
        }
        encoded.push_back(charset[remainder]);
        digits = newDigits;
    }
    std::reverse(encoded.begin(), encoded.end());

    // Compute a simple checksum: sum all bytes modulo 256, then format as two hex digits.
    unsigned int sum = 0;
    for (unsigned char c : input) {
        sum += c;
    }
    unsigned int checksum = sum % 256;
    char checksumStr[3];
    snprintf(checksumStr, sizeof(checksumStr), "%02X", checksum);

    // Build header: "UQ" + compression flag (4 bits for encoding details) + baseIndex digit.
    std::string header = "UQ";
    
    // Create a 4-bit flag (represented as a hex digit) to indicate the encoding process:
    // Bit 0 (0x1): LZ4 compression used
    // Bit 1 (0x2): MessagePack used
    // Bit 2 (0x4): Reserved for future use
    // Bit 3 (0x8): Reserved for future use
    int encodingFlags = 0;
    if (useLZ4) encodingFlags |= 0x1;
    if (useMessagePack) encodingFlags |= 0x2;
    
    // Convert to hex digit (0-F)
    char flagHex = (encodingFlags < 10) ? ('0' + encodingFlags) : ('A' + encodingFlags - 10);
    header.push_back(flagHex);
    header.push_back('0' + baseIndex);

    // Final format: header + ":" + encoded data + ":" + checksum
    std::string finalStr = header + ":" + encoded + ":" + checksumStr;
    return finalStr;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <json_string>" << std::endl;
        return 1;
    }

    // Parse the input JSON string.
    std::string jsonStr = argv[1];
    json j = json::parse(jsonStr);
    // std::cout << "Parsed JSON: " << j.dump(2) << std::endl;

    // Convert the JSON to MessagePack format.
    bool useMessagePack = false;
    std::vector<uint8_t> msgpackData;
    if (useMessagePack) {
        msgpackData = json::to_msgpack(j);
    } else {
        // If not using MessagePack, convert JSON to a string and use that
        std::string jsonDump = j.dump();
        msgpackData = std::vector<uint8_t>(jsonDump.begin(), jsonDump.end());
    }

    // Compress the data using LZ4 (if enabled).
    bool useLZ4 = true;
    int inputSize = msgpackData.size();
    std::vector<char> processedData;
    
    if (useLZ4) {
        // Allocate a buffer for compression (worst case size).
        int maxCompressedSize = LZ4_compressBound(inputSize);
        std::vector<char> compressBuffer(maxCompressedSize);
        
        // Compress the data.
        int compressedSize = LZ4_compress_default(
            reinterpret_cast<const char*>(msgpackData.data()),
            compressBuffer.data(),
            inputSize,
            maxCompressedSize
        );
        
        if (compressedSize <= 0) {
            std::cerr << "Compression failed!" << std::endl;
            return 1;
        }
        
        // Use the compressed data.
        processedData = std::vector<char>(compressBuffer.begin(), compressBuffer.begin() + compressedSize);
        std::cout << "Compressed size: " << compressedSize << " bytes (from " << inputSize << " bytes)" << std::endl;
    } else {
        // Use the original data without compression.
        processedData = std::vector<char>(
            reinterpret_cast<const char*>(msgpackData.data()),
            reinterpret_cast<const char*>(msgpackData.data()) + msgpackData.size()
        );
    }

    // Choose a base index based on a simple hash of the input.
    unsigned int hash = 0;
    for (char c : jsonStr) {
        hash = (hash * 31) + c;
    }
    int baseIndex = hash % 3; // Choose among 0, 1, or 2.

    // Encode using our method.
    std::string urlsafeString = UQEncode(processedData, useLZ4, useMessagePack, baseIndex);
    std::cout << "Encoded string: " << urlsafeString << std::endl;
    return 0;
}
