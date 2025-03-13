cmake_minimum_required(VERSION 3.14)
project(url_safe_encoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find or fetch nlohmann_json
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

# Find or fetch zstd
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)
set(DZSTD_BUILD_TESTS OFF)
set(DZSTD_LEGACY_SUPPORT OFF)
FetchContent_Declare(
    zstd
    URL "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(zstd)

# Find or fetch Brotli
set(BROTLI_BUNDLED_MODE ON)
set(BROTLI_DISABLE_TESTS ON)
FetchContent_Declare(
    brotli
    GIT_REPOSITORY https://github.com/google/brotli.git
    GIT_TAG v1.1.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(brotli)

# Fetch required Boost modules (header-only)
FetchContent_Declare(
    boost_mp
    GIT_REPOSITORY https://github.com/boostorg/multiprecision.git
    GIT_TAG boost-1.84.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(
    boost_config
    GIT_REPOSITORY https://github.com/boostorg/config.git
    GIT_TAG boost-1.84.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(
    boost_core
    GIT_REPOSITORY https://github.com/boostorg/core.git
    GIT_TAG boost-1.84.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(boost_mp boost_config boost_core)

# Add Boost headers to include path
include_directories(SYSTEM
    ${boost_mp_SOURCE_DIR}/include
    ${boost_config_SOURCE_DIR}/include
    ${boost_core_SOURCE_DIR}/include
)

# Source files for the encoder library
set(ENCODER_SOURCES
    src/encode.cpp
    src/decode.cpp
    src/common.cpp
    src/lz4.c
)

# Determine if this is being used as a library or an executable
option(BUILD_EXECUTABLE "Build as executable" ON)
option(BUILD_SHARED_LIBS "Build as shared library" OFF)

# Create the encoder library
add_library(uqpack ${ENCODER_SOURCES})

# Set include directories for the library
target_include_directories(uqpack
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${zstd_SOURCE_DIR}/lib
        ${json_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
)

# Link libraries to the encoder library
target_link_libraries(uqpack
    PRIVATE
        nlohmann_json::nlohmann_json
        libzstd_static
        brotlienc
        brotlidec
        brotlicommon
)

# If building as an executable
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    set(WASM_LINK_FLAGS
        "-s WASM=1"
        "-O3"
        "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']"
        "-s EXPORTED_FUNCTIONS=['_malloc','_free']"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s NO_EXIT_RUNTIME=1"
        "-s MODULARIZE=1"
        "-s EXPORT_NAME='UQPack'"
        "-s DISABLE_EXCEPTION_CATCHING=0"
        "-s WASM_BIGINT=1"
        "-s USE_ES6_IMPORT_META=0"
        "-s ENVIRONMENT='web,node'"
        "-s EMBIND_STD_STRING_IS_UTF8=1"
        "-s ASSERTIONS=1"
        "-lembind"
    )

    # Create WebAssembly module
    add_executable(uqpack_wasm wasm.cpp)
    target_link_libraries(uqpack_wasm PRIVATE uqpack)
    
    # Set WebAssembly-specific link flags
    string(REPLACE ";" " " WASM_LINK_FLAGS_STR "${WASM_LINK_FLAGS}")
    set_target_properties(uqpack_wasm PROPERTIES LINK_FLAGS ${WASM_LINK_FLAGS_STR})
    
    # Install WebAssembly files
    install(FILES
        ${CMAKE_BINARY_DIR}/uqpack_wasm.js
        ${CMAKE_BINARY_DIR}/uqpack_wasm.wasm
        DESTINATION ${CMAKE_BINARY_DIR}/../dist
    )
elseif(BUILD_EXECUTABLE)
    # Add the executable for native build
    add_executable(url_safe_encoder main.cpp)

    # Link libraries to the executable
    target_link_libraries(url_safe_encoder
        PRIVATE
            uqpack
            nlohmann_json::nlohmann_json
    )
endif()