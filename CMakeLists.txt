cmake_minimum_required(VERSION 3.15)

# 注意：如果使用toolchain.cmake，toolchain必须在project()之前加载
# example: cmake -DCMAKE_TOOLCHAIN_FILE=./toolchain-arm-none-eabi.cmake ...

project(C8724Q LANGUAGES C)

# ===================== 可配置编译开关 =====================
option(C8724Q_BUILD_TEST "Build test source code" ON)
option(C8724Q_BUILD_EXAMPLE "Build example source code" ON)

# ===================== 核心静态库目标 =====================
add_library(c8724q STATIC
    src/driver_c8724q.c
)

# 别名，上层项目推荐使用 C8724Q::C8724Q 链接
add_library(C8724Q::C8724Q ALIAS c8724q)

target_compile_features(c8724q PUBLIC c_std_99)

# 裸机静态库，关闭PIE，不要生成位置无关代码（单片机不需要）
# set_target_properties(c8724q PROPERTIES
#     POSITION_INDEPENDENT_CODE OFF
# )

# 对外公开头文件路径：src驱动头、interface硬件接口层
target_include_directories(c8724q
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/interface
)

# ===================== test 测试代码（可选编译，默认ON） =====================
# if(C8724Q_BUILD_TEST)
#     add_library(c8724q_test STATIC
#         test/driver_c8724q_config_test.c
#         test/driver_c8724q_display_test.c
#     )
#     target_link_libraries(c8724q_test PUBLIC C8724Q::C8724Q)
#     target_include_directories(c8724q_test
#         PUBLIC
#             ${CMAKE_CURRENT_SOURCE_DIR}/test
#     )
#     add_library(C8724Q::test ALIAS c8724q_test)
# endif()

if(C8724Q_BUILD_TEST)
    target_sources(c8724q PRIVATE
        test/driver_c8724q_config_test.c
        test/driver_c8724q_display_test.c
    )
    target_include_directories(c8724q PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
endif()

# ===================== example 示例代码（可选编译，默认OFF） =====================
# if(C8724Q_BUILD_EXAMPLE)
#     add_library(c8724q_example STATIC
#         example/driver_c8724q_basic.c
#     )
#     target_link_libraries(c8724q_example PUBLIC C8724Q::C8724Q)
#     target_include_directories(c8724q_example
#         PUBLIC
#             ${CMAKE_CURRENT_SOURCE_DIR}/example
#     )
#     add_library(C8724Q::example ALIAS c8724q_example)
# endif()

if(C8724Q_BUILD_EXAMPLE)
    target_sources(c8724q PRIVATE
        example/driver_c8724q_basic.c
    )
    target_include_directories(c8724q PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/example
    )
endif()