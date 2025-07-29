cmake_minimum_required(VERSION 3.16)
project(SecretShareDistributor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

# Try to find APSI
find_path(APSI_INCLUDE_DIR
    NAMES apsi/apsi.h
    PATHS /usr/include /usr/local/include
)

find_library(APSI_LIBRARY
    NAMES apsi
    PATHS /usr/lib /usr/local/lib
)

if(APSI_INCLUDE_DIR AND APSI_LIBRARY)
    message(STATUS "Found APSI: ${APSI_LIBRARY}")
    set(APSI_FOUND TRUE)
else()
    message(STATUS "APSI not found, will use fallback implementation")
    set(APSI_FOUND FALSE)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(APSI_FOUND)
    include_directories(${APSI_INCLUDE_DIR})
endif()

# Create the main library
add_library(secret_share_distributor
    src/secret_share_distributor.cpp
    src/beaver_triple.cpp
    src/rng_utils.cpp
)

target_link_libraries(secret_share_distributor
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(APSI_FOUND)
    target_link_libraries(secret_share_distributor ${APSI_LIBRARY})
    target_compile_definitions(secret_share_distributor PRIVATE HAVE_APSI)
endif()

# Create example executable
add_executable(example
    examples/main.cpp
)

target_link_libraries(example secret_share_distributor)

# Compiler flags
target_compile_options(secret_share_distributor PRIVATE
    -Wall -Wextra -O2
)

# Install rules
install(TARGETS secret_share_distributor
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)