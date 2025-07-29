cmake_minimum_required(VERSION 3.12)
project(SecretSharingPSI VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# 编译选项
if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# 查找依赖包
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# 可选：查找APSI依赖（如果要使用APSI组件）
find_package(PkgConfig QUIET)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 源文件
set(SOURCES
    secret_sharing_tee.cpp
    psi_item.cpp
    arithmetic_secret_sharing.cpp
    secret_sharing_psi.cpp
)

# 头文件
set(HEADERS
    secret_sharing_tee.h
    psi_item.h
    arithmetic_secret_sharing.h
    secret_sharing_psi.h
)

# 创建静态库
add_library(secretsharingpsi STATIC ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(secretsharingpsi 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)

# 设置目标属性
target_include_directories(secretsharingpsi PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

# 编译器特定设置
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(secretsharingpsi PRIVATE -march=native)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(secretsharingpsi PRIVATE -march=native)
endif()

# 选项：构建示例程序
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 选项：构建测试
option(BUILD_TESTS "Build test programs" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装设置
install(TARGETS secretsharingpsi
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${HEADERS}
    DESTINATION include/secretsharingpsi
)

# 创建配置文件
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SecretSharingPSIConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SecretSharingPSIConfig.cmake"
    INSTALL_DESTINATION lib/cmake/SecretSharingPSI
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SecretSharingPSIConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SecretSharingPSIConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SecretSharingPSIConfigVersion.cmake"
    DESTINATION lib/cmake/SecretSharingPSI
)

# 打印配置信息
message(STATUS "SecretSharingPSI Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")