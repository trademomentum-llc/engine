# x86_64-linux-gnu.cmake
# Cross-compilation toolchain for NUC nodes (TP-HCF plane 1)
# Usage: cmake -B build-x86_64 -S . -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-linux-gnu.cmake

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Size-optimized release flags for deployed NUC binaries
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -fvisibility=hidden -fno-rtti -fno-exceptions -march=x86-64-v2")
set(CMAKE_C_FLAGS_MINSIZEREL   "-Os -DNDEBUG -fvisibility=hidden -march=x86-64-v2")
