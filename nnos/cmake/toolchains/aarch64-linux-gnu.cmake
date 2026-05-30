# aarch64-linux-gnu.cmake
# Cross-compilation toolchain for Jetson Orin nodes (TP-HCF plane 3)
# Usage: cmake -B build-aarch64 -S . -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Size-optimized release flags for deployed Orin binaries
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -fvisibility=hidden -fno-rtti -fno-exceptions -march=armv8.2-a+crc+crypto")
set(CMAKE_C_FLAGS_MINSIZEREL   "-Os -DNDEBUG -fvisibility=hidden -march=armv8.2-a+crc+crypto")
