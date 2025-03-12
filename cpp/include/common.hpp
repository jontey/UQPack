#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <siphash.hpp>

namespace UQPack {

// Public interface for checksum operations
std::string calculateChecksum(const std::string& input);

namespace internal {
    // Internal implementation details
    struct SipHashKey {
        uint64_t k0;
        uint64_t k1;
        
        SipHashKey() : k0(0x0706050403020100ULL), k1(0x0F0E0D0C0B0A0908ULL) {}
    };
    
    uint64_t siphash24(const void* data, size_t size, const SipHashKey* key);
    std::string convertToBase64(uint64_t value);
} // namespace internal

} // namespace UQPack
