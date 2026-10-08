cmake_minimum_required(VERSION 3.15)

# 注意：如果使用 toolchain.cmake，toolchain 必须在 project() 之前加载
# 示例：cmake -DCMAKE_TOOLCHAIN_FILE=./toolchain-arm-none-eabi.cmake ...

project(C8724 LANGUAGES C)

# ===================== 可配置编译开关 =====================
option(C8724_BUILD_TEST    "编译测试代码" ON)
option(C8724_BUILD_EXAMPLE "编译示例代码" ON)

# ===================== 核心静态库目标 =====================
add_library(c8724 STATIC
    src/driver_c8724.c
)

# 别名，上层项目推荐使用 C8724::C8724 链接
add_library(C8724::C8724 ALIAS c8724)

target_compile_features(c8724 PUBLIC c_std_99)

# 裸机静态库，关闭 PIE，不要生成位置无关代码（单片机不需要）
# set_target_properties(c8724 PROPERTIES
#     POSITION_INDEPENDENT_CODE OFF
# )

# 对外公开头文件路径：src 驱动头、interface 硬件接口层
target_include_directories(c8724
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/interface
)

# ===================== test 测试代码（可选编译，默认 ON） =====================
# if(C8724_BUILD_TEST)
#     add_library(c8724_test STATIC
#         test/driver_c8724_config_test.c
#         test/driver_c8724_display_test.c
#     )
#     target_link_libraries(c8724_test PUBLIC C8724::C8724)
#     target_include_directories(c8724_test
#         PUBLIC
#             ${CMAKE_CURRENT_SOURCE_DIR}/test
#     )
#     add_library(C8724::test ALIAS c8724_test)
# endif()

if(C8724_BUILD_TEST)
    target_sources(c8724 PRIVATE
        test/driver_c8724_display_test.c
    )
    target_include_directories(c8724 PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )
endif()

# ===================== example 示例代码（可选编译，默认 ON） =====================
# if(C8724_BUILD_EXAMPLE)
#     add_library(c8724_example STATIC
#         example/driver_c8724_basic.c
#     )
#     target_link_libraries(c8724_example PUBLIC C8724::C8724)
#     target_include_directories(c8724_example
#         PUBLIC
#             ${CMAKE_CURRENT_SOURCE_DIR}/example
#     )
#     add_library(C8724::example ALIAS c8724_example)
# endif()

if(C8724_BUILD_EXAMPLE)
    target_sources(c8724 PRIVATE
        example/driver_c8724_basic.c
    )
    target_include_directories(c8724 PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/example
    )
endif()