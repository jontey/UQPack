#include <emscripten/bind.h>
#include <string>
#include <vector>
#include <encode.hpp>
#include <decode.hpp>

using namespace emscripten;

// Helper function to convert std::vector<uint8_t> to emscripten::val (Uint8Array)
emscripten::val vectorToTypedArray(const std::vector<uint8_t>& data) {
    return emscripten::val(emscripten::typed_memory_view(data.size(), data.data()));
}

// Helper function to convert JavaScript Uint8Array to std::vector<uint8_t>
std::vector<uint8_t> typedArrayToVector(const emscripten::val& array) {
    const auto length = array["length"].as<unsigned>();
    std::vector<uint8_t> result(length);
    emscripten::val memoryView = array["constructor"].new_(emscripten::val::module_property("HEAPU8")["buffer"], array["byteOffset"].as<unsigned>(), length);
    memoryView.call<void>("set", array);
    for (unsigned i = 0; i < length; ++i) {
        result[i] = memoryView[i].as<uint8_t>();
    }
    return result;
}

// WebAssembly exported functions
std::string encode(const emscripten::val& data, bool useZstd) {
    auto input = typedArrayToVector(data);
    std::string inputStr(input.begin(), input.end());
    json j = json::parse(inputStr);
    return UQPack::encode(j, useZstd ? UQPack::CompressionType::ZSTD : UQPack::CompressionType::LZ4);
}

emscripten::val decode(const std::string& encoded) {
    std::string decoded = UQPack::decode<std::string>(encoded);
    std::vector<uint8_t> decodedVec(decoded.begin(), decoded.end());
    return vectorToTypedArray(decodedVec);
}

EMSCRIPTEN_BINDINGS(uqpack_module) {
    function("encode", &encode);
    function("decode", &decode);
}
