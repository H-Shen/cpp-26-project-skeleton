cmake_minimum_required(VERSION 3.20)

# 工程名你可以自己改
project(MyProject LANGUAGES CXX)

# -------------------------
# 平台 / 编译器组合检查
# 只允许：
#   - Windows + MSVC
#   - macOS  + Clang/AppleClang
#   - Linux  + GCC
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
    # 这里的 UNIX 已排除 APPLE，即为 Linux
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "On Linux, only GCC is supported.")
    endif()
    set(PLATFORM_TOOLCHAIN "LINUX_GCC")

else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

# -------------------------
# C++ 标准：C++23，无扩展
# -------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器（Makefile/Ninja）默认 Release
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# -------------------------
# 全局告警 & 优化等级
# Debug:  O0 + 调试信息
# Release: O2 + DNDEBUG
# -------------------------
if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
    message(STATUS "Configuring for Windows + MSVC")

    # 强告警 + UTF-8 源文件 + bigobj
    add_compile_options(
        /W4
        /utf-8
        /bigobj
    )

    # 静态运行库：Debug /MTd，Release /MT
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    # Debug：/Od /Zi
    add_compile_options(
        "$<$<CONFIG:Debug>:/Od;/Zi>"
    )

    # Release：/O2 + NDEBUG（不加 /Zi）
    add_compile_options(
        "$<$<CONFIG:Release>:/O2;/DNDEBUG>"
    )

elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM"
       OR PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
    message(STATUS "Configuring for Unix-like + ${CMAKE_CXX_COMPILER_ID}")

    # 强告警（包括 unused、未初始化等）
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

    # Debug：-O0 -g
    add_compile_options(
        "$<$<CONFIG:Debug>:-O0;-g>"
    )

    # Release：-O2 -DNDEBUG（不加 -g）
    add_compile_options(
        "$<$<CONFIG:Release>:-O2;-DNDEBUG>"
    )
endif()

# -------------------------
# Sanitizer 开关（仅 Debug 生效）
# 你也可以在命令行关掉：
#   cmake -DENABLE_SANITIZERS=OFF ..
# -------------------------
option(ENABLE_SANITIZERS "Enable sanitizers in Debug builds" ON)

# 为主目标准备 sanitizer flag（稍后挂到 my_app 上）
set(PROJECT_SANITIZER_COMPILE_FLAGS "")
set(PROJECT_SANITIZER_LINK_FLAGS "")

if(ENABLE_SANITIZERS)
    if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
        # MSVC 目前主要支持 AddressSanitizer
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:/fsanitize=address>"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:/fsanitize=address>"
        )

    elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM")
        # macOS (AppleClang)：不支持单独 -fsanitize=leak
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined;-fno-omit-frame-pointer>"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
        )

    elseif(PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
        # Linux + GCC：可以使用 address,undefined,leak
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined,leak;-fno-omit-frame-pointer>"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>"
        )
    endif()
endif()

# -------------------------
# 多线程库
# -------------------------
find_package(Threads REQUIRED)

# -------------------------
# 源文件列表
# 目前就一个 HelloWorld
# 后续自己往 SRC_FILES 里加就行
# -------------------------
set(SRC_FILES
    src/main.cpp
)

# -------------------------
# 主可执行文件：my_app
# Debug：可选 ASan + UBSan (+ leak on Linux)
# Release：无 sanitizer
# -------------------------
add_executable(my_app
    ${SRC_FILES}
)

target_link_libraries(my_app
    PRIVATE
        Threads::Threads
)

# 给 my_app 挂上 sanitizer（仅 Debug 配置下有效）
if(PROJECT_SANITIZER_COMPILE_FLAGS)
    target_compile_options(my_app PRIVATE ${PROJECT_SANITIZER_COMPILE_FLAGS})
endif()

if(PROJECT_SANITIZER_LINK_FLAGS)
    target_link_options(my_app PRIVATE ${PROJECT_SANITIZER_LINK_FLAGS})
endif()

# -------------------------
# 线程 Sanitizer 版本：my_app_tsan
# 仅在 macOS/Linux 且开启 ENABLE_SANITIZERS 时生成
# Debug：-fsanitize=thread
# Release：无额外 sanitizer
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
# 只对 main 源文件启用 Werror(/WX)
# 其它源文件：高强度告警但不强制为错误
# -------------------------
set(MAIN_SOURCE "src/main.cpp")  # main 文件路径

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
