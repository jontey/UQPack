#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <encode.hpp>
#include <decode.hpp>

// Use nlohmann::json (https://github.com/nlohmann/json)
using json = nlohmann::json;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <json_string> [compression_type]" << std::endl;
        std::cerr << "  compression_type: none, lz4, zlib, zstd (default: lz4)" << std::endl;
        return 1;
    }

    try {
        // Parse the input JSON string
        std::string jsonStr = argv[1];
        json j = json::parse(jsonStr);
        
        // Determine compression type from command line argument (if provided)
        UQPack::CompressionType compressionType = UQPack::CompressionType::LZ4; // Default
        
        if (argc >= 3) {
            std::string compressionArg = argv[2];
            if (compressionArg == "none") {
                compressionType = UQPack::CompressionType::NONE;
            } else if (compressionArg == "lz4") {
                compressionType = UQPack::CompressionType::LZ4;
            } else if (compressionArg == "zstd") {
                compressionType = UQPack::CompressionType::ZSTD;
            } else {
                // std::cerr << "Unknown compression type: " << compressionArg << std::endl;
                // std::cerr << "Using default (lz4)" << std::endl;
            }
        }
        
        // Encode using our method with specified compression and MessagePack enabled
        // The baseIndex will be automatically selected based on the input
        std::string urlsafeString = UQPack::encode(j, compressionType);
        std::cout << "Encoded string: " << urlsafeString << std::endl;

        // Decode the string here
        try {
            json decodedData = UQPack::decode<json>(urlsafeString);
            std::cout << "Decoded data: " << decodedData << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
