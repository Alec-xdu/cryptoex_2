# 示例程序CMakeLists.txt

# 基本示例
add_executable(basic_psi_example basic_psi_example.cpp)
target_link_libraries(basic_psi_example secretsharingpsi)

# 标签PSI示例
add_executable(labeled_psi_example labeled_psi_example.cpp)
target_link_libraries(labeled_psi_example secretsharingpsi)

# 性能测试示例
add_executable(performance_test performance_test.cpp)
target_link_libraries(performance_test secretsharingpsi)

# Beaver三元组测试
add_executable(beaver_triplet_test beaver_triplet_test.cpp)
target_link_libraries(beaver_triplet_test secretsharingpsi)

# 安装示例程序
install(TARGETS 
    basic_psi_example 
    labeled_psi_example 
    performance_test 
    beaver_triplet_test
    DESTINATION bin/examples
)