#include <decode.hpp>
#include <encode.hpp>
#include <base.hpp>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <siphash.hpp>
#include <brotli/decode.h>
#include <common.hpp>

#ifndef EMSCRIPTEN
#include <lz4.h>
#include <zstd.h>
#endif

// UQPack::decode: Decode a URL-safe string back into binary data.
// This function reverses the encoding process performed by UQEncode.
namespace UQPack {
    #ifndef EMSCRIPTEN
    // Helper function to decompress data using LZ4
    std::vector<std::uint8_t> decompressWithLZ4(const std::uint8_t* compressedData, size_t compressedSize, size_t originalSize) {
        std::vector<std::uint8_t> decompressBuffer(originalSize);
        
        int decompressedSize = LZ4_decompress_safe(
            reinterpret_cast<const char*>(compressedData),
            reinterpret_cast<char*>(decompressBuffer.data()),
            compressedSize,
            originalSize
        );
        
        if (decompressedSize < 0) {
            throw std::runtime_error("LZ4 decompression failed");
        }
        
        // Return only the decompressed portion
        return std::vector<std::uint8_t>(decompressBuffer.begin(), decompressBuffer.begin() + decompressedSize);
    }

    // Helper function to decompress data using zstd
    std::vector<std::uint8_t> decompressWithZstd(const std::uint8_t* compressedData, size_t compressedSize) {
        // Retrieve the stored decompressed size from the frame header
        unsigned long long storedSize = ZSTD_getFrameContentSize(compressedData, compressedSize);
        if (storedSize == ZSTD_CONTENTSIZE_ERROR) {
            throw std::runtime_error("Error reading stored decompressed size from compressed data");
        }
        if (storedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            throw std::runtime_error("Decompressed size unknown in compressed data frame");
        }

        // Prepare the buffer for decompression
        std::vector<std::uint8_t> decompressBuffer(storedSize);

        size_t decompressedSize = ZSTD_decompress(
            decompressBuffer.data(),
            storedSize,
            reinterpret_cast<const char*>(compressedData),
            compressedSize
        );

        if (ZSTD_isError(decompressedSize)) {
            throw std::runtime_error("Zstd decompression failed: " +
                                    std::string(ZSTD_getErrorName(decompressedSize)));
        }

        // Return only the decompressed portion
        return std::vector<std::uint8_t>(decompressBuffer.begin(),
                                        decompressBuffer.begin() + decompressedSize);
    }
    #endif

    // Helper function to decompress data using Brotli
    std::vector<std::uint8_t> decompressWithBrotli(const std::uint8_t* compressedData, size_t compressedSize) {
        // Create a decoder state
        BrotliDecoderState* state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
        if (!state) {
            throw std::runtime_error("Failed to create Brotli decoder instance");
        }

        // Start with a reasonable buffer size
        size_t bufferSize = compressedSize * 4;  // Assume up to 4:1 compression ratio
        std::vector<std::uint8_t> decompressBuffer(bufferSize);
        size_t availableIn = compressedSize;
        const uint8_t* nextIn = compressedData;
        size_t availableOut = bufferSize;
        uint8_t* nextOut = decompressBuffer.data();
        size_t totalOut = 0;
        BrotliDecoderResult result;

        do {
            result = BrotliDecoderDecompressStream(
                state,
                &availableIn,
                &nextIn,
                &availableOut,
                &nextOut,
                &totalOut);

            if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
                // Buffer is too small, resize it
                size_t currentSize = decompressBuffer.size();
                decompressBuffer.resize(currentSize * 2);
                availableOut = currentSize;
                nextOut = decompressBuffer.data() + totalOut;
            }
        } while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT);

        BrotliDecoderDestroyInstance(state);

        if (result != BROTLI_DECODER_RESULT_SUCCESS) {
            throw std::runtime_error("Brotli decompression failed");
        }

        // Resize buffer to actual decompressed size
        decompressBuffer.resize(totalOut);
        return decompressBuffer;
    }

    std::vector<std::uint8_t> decodeInternal(const std::string& encodedString, CompressionType& outCompressionType) {
        // Parse the encoded string format: header + ":" + encoded data + ":" + checksum
        size_t firstColon = encodedString.find(':');
        size_t lastColon = encodedString.rfind(':');
        
        if (firstColon == std::string::npos || lastColon == std::string::npos || firstColon == lastColon) {
            throw std::runtime_error("Invalid encoded string format");
        }
        
        std::string header = encodedString.substr(0, firstColon);
        std::string encodedData = encodedString.substr(firstColon + 1, lastColon - firstColon - 1);
        std::string checksumStr = encodedString.substr(lastColon + 1);
        
        // Calculate checksum using the common implementation
        std::string computedChecksumStr = calculateChecksum(encodedData);
        
        // Validate checksum
        if (checksumStr != computedChecksumStr) {
            throw std::runtime_error("Checksum validation failed. Supposed to be " + computedChecksumStr + " but got " + checksumStr);
        }
        
        // Validate header format
        if (header.length() != 4 || header.substr(0, 2) != "PX") {
            throw std::runtime_error("Invalid header format");
        }
        
        // Parse compression flags (4 bits)
        char compressionHex = header[2];
        int compressionFlags = 0;
        
        if (compressionHex >= '0' && compressionHex <= '9') {
            compressionFlags = compressionHex - '0';
        } else if (compressionHex >= 'A' && compressionHex <= 'F') {
            compressionFlags = compressionHex - 'A' + 10;
        } else if (compressionHex >= 'a' && compressionHex <= 'f') {
            compressionFlags = compressionHex - 'a' + 10;
        } else {
            throw std::runtime_error("Invalid compression flag format");
        }
        
        // Extract compression flags
        // Bit 0 (0x1): LZ4 compression used
        // Bit 1 (0x2): Zstd compression used
        // Bit 2 (0x4): Brotli compression used
        // Bit 3 (0x8): Reserved for future use
        bool useLZ4 = (compressionFlags & 0x1) != 0;
        bool useZstd = (compressionFlags & 0x2) != 0;
        bool useBrotli = (compressionFlags & 0x4) != 0;
        
        // Validate compression flags - only one compression type should be set
        if (useLZ4 && useZstd) {
            throw std::runtime_error("Invalid compression flags: multiple compression types set");
        }
        
        // Parse encoding flags (4 bits)
        char encodingHex = header[3];
        int encodingFlags = 0;
        
        if (encodingHex >= '0' && encodingHex <= '9') {
            encodingFlags = encodingHex - '0';
        } else if (encodingHex >= 'A' && encodingHex <= 'F') {
            encodingFlags = encodingHex - 'A' + 10;
        } else if (encodingHex >= 'a' && encodingHex <= 'f') {
            encodingFlags = encodingHex - 'a' + 10;
        } else {
            throw std::runtime_error("Invalid encoding flag format");
        }
        
        // Determine which base was used for encoding
        int baseIndex = (encodingFlags & 0x1) ? 1 : 0;

        std::string charset = basesCharSet[baseIndex];
        const int base = charset.length();
        
        // Convert from base-N to bytes
        std::vector<std::uint8_t> decodedData = convertFromBase(encodedData, charset);
        
        // Set output parameters
        outCompressionType = CompressionType::NONE;
        
        // Handle decompression if needed
        if (useLZ4) {
            #ifndef EMSCRIPTEN
            outCompressionType = CompressionType::LZ4;
            // Get original size from the first 4 bytes. This is necessary for lz4 which does not support dynamically allocating memory during decompression
            if (decodedData.size() < 4) {
                throw std::runtime_error("Invalid compressed data: too short");
            }
            size_t originalSize = *reinterpret_cast<uint32_t*>(decodedData.data());
            decodedData = decompressWithLZ4(decodedData.data() + 4, decodedData.size() - 4, originalSize);
            #else
            throw std::runtime_error("LZ4 compression not supported on this platform");
            #endif
        } else if (useZstd) {
            #ifndef EMSCRIPTEN
            outCompressionType = CompressionType::ZSTD;
            decodedData = decompressWithZstd(decodedData.data(), decodedData.size());
            #else
            throw std::runtime_error("Zstd compression not supported on this platform");
            #endif
        } else if (useBrotli) {
            outCompressionType = CompressionType::BROTLI;
            decodedData = decompressWithBrotli(decodedData.data(), decodedData.size());
        }
        
        return decodedData;
    }
}
