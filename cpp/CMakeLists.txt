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

# Directly set LZ4 paths (Homebrew installation)
set(LZ4_INCLUDE_DIRS "/opt/homebrew/opt/lz4/include")
set(LZ4_LIBRARIES "/opt/homebrew/opt/lz4/lib/liblz4.dylib")

# Add the executable
add_executable(url_safe_encoder main.cpp)

# Link libraries
target_link_libraries(url_safe_encoder
    PRIVATE
        nlohmann_json::nlohmann_json
        ${LZ4_LIBRARIES}
)

# Include directories for LZ4
target_include_directories(url_safe_encoder
    PRIVATE
        ${LZ4_INCLUDE_DIRS}
) 