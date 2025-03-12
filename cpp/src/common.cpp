#include <common.hpp>
#include <base.hpp>
#include <siphash.hpp>

namespace UQPack {

std::string calculateChecksum(const std::string& input) {
    internal::SipHashKey sipKey;
    uint64_t hashValue = internal::siphash24(input.data(), input.size(), &sipKey);
    return internal::convertToBase64(hashValue).substr(0, 2);
}

namespace internal {

uint64_t siphash24(const void* data, size_t size, const SipHashKey* key) {
    siphash::Key sipKey(key->k0, key->k1);
    return siphash::siphash24(data, size, &sipKey);
}

std::string convertToBase64(uint64_t value) {
    // Reuse the existing base64 conversion from base.hpp
    return UQPack::convertToBase64(value);
}

} // namespace internal
} // namespace UQPack
