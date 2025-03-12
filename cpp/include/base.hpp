#ifndef BASE_H
#define BASE_H

#include <string>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

namespace UQPack {
    // Convert a uint64_t to a base64 string
    inline std::string convertToBase64(uint64_t value) {
        // Base64 URL-safe character set
        const char* base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        std::string result;
        
        // Convert to base64 by repeatedly dividing by 64
        while (value > 0) {
            result = base64chars[value % 64] + result;
            value /= 64;
        }
        
        return result;
    }

    // Convert a number to a string using the specified base charset
    inline std::string convertToBase(const std::vector<std::uint8_t>& digits, const std::string& charset) {
        const int base = charset.length();
        std::string result;
        result.reserve(digits.size() * 2); // Pre-allocate to avoid reallocations
        
        // Convert bytes to a single large integer
        boost::multiprecision::cpp_int value = 0;
        for (auto it = digits.begin(); it != digits.end(); ++it) {
            value = (value << 8) | *it;
        }
        
        // Convert to the target base
        if (value == 0) {
            result.push_back(charset[0]);
        } else {
            do {
                boost::multiprecision::cpp_int quotient;
                boost::multiprecision::cpp_int remainder;
                
                boost::multiprecision::divide_qr(value, boost::multiprecision::cpp_int(base), quotient, remainder);
                result.push_back(charset[static_cast<int>(remainder)]);
                value = quotient;
                
            } while (value > 0);
        }

        std::reverse(result.begin(), result.end());

        return result;
    }
    
    // Convert a string back to bytes using the specified base charset
    inline std::vector<std::uint8_t> convertFromBase(const std::string& str, const std::string& charset) {
        const int base = charset.length();
        
        // Create value lookup map
        std::unordered_map<char, int> charToValue;
        for (int i = 0; i < charset.length(); i++) {
            charToValue[charset[i]] = i;
        }
        
        // Convert string to large integer
        boost::multiprecision::cpp_int value = 0;
        for (char c : str) {
            if (charToValue.find(c) == charToValue.end()) {
                throw std::runtime_error("Invalid character in encoded data");
            }
            value = value * base + charToValue[c];
        }
        
        // Convert large integer back to bytes
        std::vector<std::uint8_t> result;
        while (value > 0) {
            result.push_back(static_cast<std::uint8_t>(value & 0xFF));
            value >>= 8;
        }
        
        // Handle special case for zero
        if (result.empty()) {
            result.push_back(0);
        } else {
            std::reverse(result.begin(), result.end());
        }
        
        return result;
    }

}

#endif // BASE_H
