#include <encode.hpp>
#include <base.hpp>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <lz4.h>
#include <zstd.h>
#include <siphash.hpp>
#include <common.hpp>

namespace UQPack {
    // Helper function to calculate a simple hash for auto-selecting baseIndex
    int calculateBaseIndex(const std::string& input) {
        unsigned int hash = 0;
        for (char c : input) {
            hash = (hash * 31) + c;
        }
        return 0;
        // return hash % 2; // Choose among 0 or 1
    }

    // Helper function to compress data using LZ4
    std::vector<std::uint8_t> compressWithLZ4(const std::uint8_t* data, size_t dataSize) {
        // Allocate a buffer for compression (worst case size)
        int maxCompressedSize = LZ4_compressBound(dataSize);
        std::vector<std::uint8_t> compressBuffer(maxCompressedSize);
        
        // Compress the data
        int compressedSize = LZ4_compress_default(
            reinterpret_cast<const char*>(data),
            reinterpret_cast<char*>(compressBuffer.data()),
            dataSize,
            maxCompressedSize
        );
        
        if (compressedSize <= 0) {
            throw std::runtime_error("LZ4 compression failed");
        }
        
                // Return only the compressed portion
        compressBuffer.resize(compressedSize);

        // Prepend a custom header (4 bytes) containing the original data size.
        std::vector<std::uint8_t> finalBuffer(4 + compressBuffer.size());
        *reinterpret_cast<uint32_t*>(finalBuffer.data()) = static_cast<uint32_t>(dataSize);
        std::copy(compressBuffer.begin(), compressBuffer.end(), finalBuffer.begin() + 4);
        
        return finalBuffer;
    }

    // Helper function to compress data using zstd
    std::vector<std::uint8_t> compressWithZstd(const std::uint8_t* data, size_t dataSize) {
        // Calculate the upper bound for the compressed data
        size_t compressBound = ZSTD_compressBound(dataSize);
        std::vector<std::uint8_t> compressBuffer(compressBound);

        // Compress the data
        size_t compressedSize = ZSTD_compress(
            compressBuffer.data(),
            compressBound,
            reinterpret_cast<const char*>(data),
            dataSize,
            1  // Compression level (1-22, higher = better compression but slower)
        );

        if (ZSTD_isError(compressedSize)) {
            throw std::runtime_error("Zstd compression failed: " +
                                    std::string(ZSTD_getErrorName(compressedSize)));
        }

        // Resize the buffer to the actual compressed size
        compressBuffer.resize(compressedSize);

        // Check the stored decompressed size in the frame header
        unsigned long long storedSize = ZSTD_getFrameContentSize(compressBuffer.data(), compressedSize);
        if (storedSize == ZSTD_CONTENTSIZE_ERROR) {
            throw std::runtime_error("Error reading stored decompressed size from compressed data");
        }
        if (storedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            throw std::runtime_error("Decompressed size unknown in compressed data frame");
        }
        if (storedSize != dataSize) {
            throw std::runtime_error("Mismatch between original data size and stored decompressed size");
        }

        return compressBuffer;
    }

    // Helper function to compress data with the specified compression type
    std::vector<std::uint8_t> compressData(const std::uint8_t* data, size_t dataSize, CompressionType compressionType) {
        switch (compressionType) {
            case CompressionType::LZ4:
                return compressWithLZ4(data, dataSize);
            case CompressionType::ZSTD:
                return compressWithZstd(data, dataSize);
            case CompressionType::NONE:
            default:
                // No compression, just copy the data
                return std::vector<std::uint8_t>(reinterpret_cast<const std::uint8_t*>(data), reinterpret_cast<const std::uint8_t*>(data) + dataSize);
        }
    }

    // Low-level encode function for binary data with compression type
    std::string encode(const std::vector<std::uint8_t>& data, CompressionType compressionType, bool useMessagePack, int baseIndex) {
        // Define our URL-safe character sets.
        const std::vector<std::string> bases = {
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_",         // Base64 URL-safe
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~"         // Base70
        };

        std::string charset = bases[baseIndex];

        std::string encoded = convertToBase(data, charset);

        // Calculate checksum using the common implementation
        std::string checksumStr = calculateChecksum(encoded);

        // Build header: "PX" + compression flag (4 bits) + encoding/cipher flag (4 bits)
        std::string header = "PX";
        
        // Create a 4-bit flag (represented as a hex digit) to indicate the compression used:
        // Bit 0 (0x1): LZ4 compression used
        // Bit 1 (0x2): MessagePack used
        // Bit 2 (0x4): Zstd compression used
        // Bit 3 (0x8): Reserve for future use
        int compressionFlags = 0;
        
        // Set compression flags based on the compression type
        switch (compressionType) {
            case CompressionType::LZ4:
                compressionFlags |= 0x1;
                break;
            case CompressionType::ZSTD:
                compressionFlags |= 0x4;
                break;
            case CompressionType::NONE:
            default:
                // No compression flags set
                break;
        }
        
        // Set MessagePack flag if used
        if (useMessagePack) compressionFlags |= 0x2;
        
        // Create a 4-bit flag (represented as a hex digit) to indicate the encoding and cipher process:
        // First bit (0x1) - Encoding step:
        //   0x0 - Base64
        //   0x1 - Base70
        // Next 3 bits - Cipher steps:
        //   0x0 - Reverse bytes only
        //   0x1 - Caesar cipher only
        //   0x2 - Reverse bytes + Caesar cipher
        //   0x3 - Caesar cipher + Reverse bytes
        //   0x4-0x8 - Reserved for future use
        int encodingFlags = 0;
        
        // Set encoding bit (first bit) based on baseIndex
        if (baseIndex == 1) { // Base70
            encodingFlags |= 0x1;
        }
        // Note: baseIndex 0 (Base64) will have encoding bit 0
        
        // For now, we'll default to "Reverse bytes only" (0x0) for the cipher steps
        // This can be expanded later when cipher implementations are added
        
        // Convert compression flags to hex digit (0-F)
        char compressionHex = (compressionFlags < 10) ? ('0' + compressionFlags) : ('A' + compressionFlags - 10);
        header.push_back(compressionHex);
        
        // Convert encoding flags to hex digit (0-F)
        char encodingHex = (encodingFlags < 10) ? ('0' + encodingFlags) : ('A' + encodingFlags - 10);
        header.push_back(encodingHex);
        
        // Final format: header + ":" + encoded data + ":" + checksumStr
        std::string finalStr = header + ":" + encoded + ":" + checksumStr;
        return finalStr;
    }
    
    // Encode JSON data with compression type
    std::string encode(const json& jsonData, CompressionType compressionType, bool useMessagePack, int baseIndex) {
        // Convert the JSON to MessagePack or string format
        std::vector<uint8_t> serializedData;
        if (useMessagePack) {
            serializedData = json::to_msgpack(jsonData);
        } else {
            // If not using MessagePack, convert JSON to a string
            std::string jsonDump = jsonData.dump();
            serializedData = std::vector<uint8_t>(jsonDump.begin(), jsonDump.end());
        }
        
        // Process the data (compress if needed)
        std::vector<std::uint8_t> processedData;
        if (compressionType != CompressionType::NONE) {
            // Compress the data using the specified compression method
            processedData = compressData(
                serializedData.data(),
                serializedData.size(),
                compressionType
            );
            
            std::cout << "Compressed size: " << processedData.size() << " bytes (from " 
                      << serializedData.size() << " bytes) using ";
            
            switch (compressionType) {
                case CompressionType::LZ4: std::cout << "LZ4"; break;
                case CompressionType::ZSTD: std::cout << "Zstd"; break;
                default: std::cout << "Unknown"; break;
            }
            
            std::cout << std::endl;
        } else {
            // Use the original data without compression
            processedData = std::vector<std::uint8_t>(serializedData.begin(), serializedData.end());
        }
        
        // Auto-select baseIndex if not specified
        if (baseIndex < 0) {
            std::string jsonStr = jsonData.dump();
            baseIndex = calculateBaseIndex(jsonStr);
        }
        
        // Encode the processed data
        return encode(processedData, compressionType, useMessagePack, baseIndex);
    }
}