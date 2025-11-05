cmake_minimum_required(VERSION 3.20)

project(MyProject LANGUAGES CXX)

# -------------------------
# 只允许三种工具链组合：
# 1) Windows + MSVC
# 2) macOS  + Clang/AppleClang
# 3) Linux  + GCC
# 其他一律报错
# -------------------------
if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR "On Windows, only MSVC toolchain is supported.")
    endif()
    set(PLATFORM_TOOLCHAIN "WIN_MSVC")
elseif(APPLE)
    if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
            OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        message(FATAL_ERROR "On macOS, only Clang/AppleClang (LLVM) is supported.")
    endif()
    set(PLATFORM_TOOLCHAIN "MAC_LLVM")
elseif(UNIX)
    # 这里的 UNIX 已经排除了 APPLE，所以就是 Linux
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "On Linux, only GCC is supported.")
    endif()
    set(PLATFORM_TOOLCHAIN "LINUX_GCC")
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

# -------------------------
# C++23
# -------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器默认 Release
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# -------------------------
# 全局高强度告警 & 优化等级
# Debug: O0 + 调试信息
# Release: O2 不带调试信息
# -------------------------
if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
    message(STATUS "Configuring for Windows + MSVC")

    # 告警 + UTF-8 源码 + bigobj
    add_compile_options(
        /W4
        /utf-8
        /bigobj
    )

    # 静态运行库：/MTd (Debug) /MT (Release)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    # Debug: /Od /Zi
    add_compile_options(
        "$<$<CONFIG:Debug>:/Od;/Zi>"
    )

    # Release: /O2 + NDEBUG（不加 /Zi）
    add_compile_options(
        "$<$<CONFIG:Release>:/O2;/DNDEBUG>"
    )

elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM"
       OR PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
    message(STATUS "Configuring for Unix-like + ${CMAKE_CXX_COMPILER_ID}")

    # 强告警
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wfloat-equal
        -Wcast-align
        -Wnull-dereference
        -Wunused
        -Wuninitialized
        -Wformat=2
        -Wundef
        -Wpointer-arith
        -Wcast-qual
    )

    # Debug: O0 + g
    add_compile_options(
        "$<$<CONFIG:Debug>:-O0;-g>"
    )

    # Release: O2 + NDEBUG（不加 -g）
    add_compile_options(
        "$<$<CONFIG:Release>:-O2;-DNDEBUG>"
    )
endif()

# -------------------------
# 控制是否启用 Sanitizer（只在 Debug 用）
# 你可以在 cmake 命令里关掉：
# -DENABLE_SANITIZERS=OFF
# -------------------------
option(ENABLE_SANITIZERS "Enable sanitizers in Debug builds" ON)

# -------------------------
# 多线程库
# -------------------------
find_package(Threads REQUIRED)

# -------------------------
# 源码列表（按你的项目改）
# -------------------------
set(SRC_FILES
    src/main.cpp
    # src/foo.cpp
    # src/bar.cpp
)

# -------------------------
# 主可执行文件：my_app
# Debug: ASan + UBSan + Leak
# Release: 无任何 sanitizer
# -------------------------
add_executable(my_app
    ${SRC_FILES}
)

target_link_libraries(my_app
    PRIVATE
        Threads::Threads
)

if(ENABLE_SANITIZERS)
    if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
        # MSVC 目前只支持 AddressSanitizer
        target_compile_options(my_app PRIVATE
            "$<$<CONFIG:Debug>:/fsanitize=address>"
        )
        target_link_options(my_app PRIVATE
            "$<$<CONFIG:Debug>:/fsanitize=address>"
        )
    elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM"
           OR PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
        # Clang/GCC: ASan + UBSan + Leak
        target_compile_options(my_app PRIVATE
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined,leak;-fno-omit-frame-pointer>"
        )
        target_link_options(my_app PRIVATE
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>"
        )
    endif()
endif()

# -------------------------
# 线程 Sanitizer 版本：my_app_tsan
# 无论你 main 里现在用不用多线程，这个可执行文件都会生成
# Debug: -fsanitize=thread
# Release: 不启用该 target 的额外 sanitizer
# 注：MSVC 没有 TSan，就不生成 tsan 版本
# -------------------------
if(ENABLE_SANITIZERS
   AND (PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM"
        OR PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC"))

    add_executable(my_app_tsan
        ${SRC_FILES}
    )

    target_link_libraries(my_app_tsan
        PRIVATE
            Threads::Threads
    )

    target_compile_options(my_app_tsan PRIVATE
        "$<$<CONFIG:Debug>:-fsanitize=thread;-fno-omit-frame-pointer>"
    )
    target_link_options(my_app_tsan PRIVATE
        "$<$<CONFIG:Debug>:-fsanitize=thread>"
    )
endif()

# -------------------------
# 只对 main 源文件开 Werror(/WX)
# 其它文件只是高强度告警，不强制错误
# -------------------------
set(MAIN_SOURCE "src/main.cpp") # 按实际路径改

if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
    set_source_files_properties(${MAIN_SOURCE}
        PROPERTIES
            COMPILE_OPTIONS "/WX"
    )
else()
    set_source_files_properties(${MAIN_SOURCE}
        PROPERTIES
            COMPILE_OPTIONS "-Werror"
    )
endif()
