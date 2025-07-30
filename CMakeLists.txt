cmake_minimum_required(VERSION 3.12)
project(SecretSharingPSI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/secret_sharing.cpp
    src/item.cpp
    src/hash_table.cpp
    src/cuckoo_hash.cpp
    src/psi_protocol.cpp
    src/oprf.cpp
)

# Header files
set(HEADERS
    include/secret_sharing.h
    include/item.h
    include/hash_table.h
    include/cuckoo_hash.h
    include/psi_protocol.h
    include/oprf.h
    include/common.h
)

# Create library
add_library(secret_sharing_psi ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(secret_sharing_psi 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    Threads::Threads
)

# Example executable
add_executable(psi_example examples/main.cpp)
target_link_libraries(psi_example secret_sharing_psi)

# Test executable
add_executable(psi_test tests/test_main.cpp)
target_link_libraries(psi_test secret_sharing_psi)

# Compiler flags
target_compile_options(secret_sharing_psi PRIVATE -Wall -Wextra -O3)