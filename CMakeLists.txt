cmake_minimum_required(VERSION 3.10)
project(stdc VERSION 1.0.0 LANGUAGES C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 添加编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/W4 /Od)
    else()
        add_compile_options(-Wall -Wextra -g -O0)
    endif()
else()
    if(MSVC)
        add_compile_options(/W4 /O2)
    else()
        add_compile_options(-Wall -Wextra -O2)
    endif()
endif()

# 平台特定设置
if(WIN32)
    add_definitions(-D_WIN32)
    # Windows 特定编译选项
    if(MSVC)
        add_compile_options(/W4)
    endif()
elseif(APPLE)
    add_definitions(-D__DARWIN__)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-D_GNU_SOURCE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_definitions(-D__FreeBSD__)
endif()

# 库源文件
set(STDC_SOURCES
    stdc.c
)

set(STDC_HEADERS
    stdc.h
)

# 创建静态库
add_library(stdc STATIC ${STDC_SOURCES})

# 设置库的输出名称和位置
set_target_properties(stdc PROPERTIES
    OUTPUT_NAME "stdc"
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER "${STDC_HEADERS}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# 包含目录
target_include_directories(stdc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# 平台特定链接库
if(APPLE)
    # macOS 需要 CoreFoundation 框架
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(stdc PUBLIC ${COREFOUNDATION_LIBRARY})
elseif(UNIX AND NOT APPLE)
    # Linux/BSD 需要 pthread
    find_package(Threads REQUIRED)
    target_link_libraries(stdc PUBLIC Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Linux 可能需要 rt 库
        target_link_libraries(stdc PUBLIC rt)
    endif()
elseif(WIN32)
    # Windows 需要 ws2_32 (Windows Sockets)
    target_link_libraries(stdc PUBLIC ws2_32)
endif()

# 安装规则
include(GNUInstallDirs)

install(TARGETS stdc
    EXPORT stdc-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 导出目标配置
install(EXPORT stdc-targets
    FILE stdc-targets.cmake
    NAMESPACE stdc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stdc
)

# 生成配置文件
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stdc-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/stdc-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stdc
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/stdc-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/stdc-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/stdc-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stdc
)

# 打印构建信息
message(STATUS "=== stdc Static Library ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==========================")
