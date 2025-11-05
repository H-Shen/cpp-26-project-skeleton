cmake_minimum_required(VERSION 3.20)

project(MyProject LANGUAGES CXX)

# --------------------------------------------------
# 平台 / 编译器组合检查
#   - Windows  : MSVC
#   - macOS    : Clang / AppleClang
#   - Linux    : GCC
#   - FreeBSD  : Clang
# --------------------------------------------------
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

elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "On FreeBSD, Clang is expected as the C++ compiler.")
    endif()
    set(PLATFORM_TOOLCHAIN "FREEBSD_CLANG")

elseif(UNIX)
    # 其它非 Apple 的 UNIX，当作 Linux + GCC
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "On Linux, only GCC is supported.")
    endif()
    set(PLATFORM_TOOLCHAIN "LINUX_GCC")

else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

# --------------------------------------------------
# C++ 标准：C++23，无扩展
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器默认 Release
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ==================================================
# 各平台：警告、优化、Debug 检测机制
# ==================================================

if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
    message(STATUS "Configuring for Windows + MSVC")

    # 通用设置
    add_compile_options(
        /W4         # 高等级警告
        /utf-8      # 源码 UTF-8
        /bigobj
    )

    # 静态 CRT：Debug /MTd，Release /MT
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    # Debug：不开优化 + 调试信息 + 一堆安全/运行时/静态检查
    add_compile_options(
        "$<$<CONFIG:Debug>:
            /Od
            /Zi
            /RTC1
            /sdl
            /analyze
            /permissive-
        >"
    )

    # Release：O2 + NDEBUG
    add_compile_options(
        "$<$<CONFIG:Release>:/O2;/DNDEBUG>"
    )

elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM"
       OR PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC"
       OR PLATFORM_TOOLCHAIN STREQUAL "FREEBSD_CLANG")

    message(STATUS "Configuring for Unix-like + ${CMAKE_CXX_COMPILER_ID} (${PLATFORM_TOOLCHAIN})")

    # 通用高强度 warnings
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

    # Debug：O0 + g + stack protector
    add_compile_options(
        "$<$<CONFIG:Debug>:
            -O0
            -g
            -fstack-protector-strong
        >"
    )

    # Release：O2 + NDEBUG + stack protector + FORTIFY
    add_compile_options(
        "$<$<CONFIG:Release>:
            -O2
            -DNDEBUG
            -fstack-protector-strong
            -D_FORTIFY_SOURCE=2
        >"
    )

    # STL 断言：GCC / Clang + libc++
    if(PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
        # libstdc++ runtime assertions
        add_compile_options(
            "$<$<CONFIG:Debug>:-D_GLIBCXX_ASSERTIONS>"
        )
    elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM"
           OR PLATFORM_TOOLCHAIN STREQUAL "FREEBSD_CLANG")
        # libc++ assertions
        add_compile_options(
            "$<$<CONFIG:Debug>:-D_LIBCPP_ENABLE_ASSERTIONS=1>"
        )
    endif()
endif()

# ==================================================
# Sanitizers（仅非 Windows Debug 用）
# ==================================================
option(ENABLE_SANITIZERS "Enable sanitizers in Debug builds (non-Windows only)" ON)

set(PROJECT_SANITIZER_COMPILE_FLAGS "")
set(PROJECT_SANITIZER_LINK_FLAGS "")

if(ENABLE_SANITIZERS)
    if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
        message(STATUS "Sanitizers are not configured for MSVC; using /RTC1, /sdl, /analyze instead.")
    elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM")
        # macOS: ASan + UBSan
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
             >"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:
                -fsanitize=address,undefined
             >"
        )
    elseif(PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
        # Linux + GCC: ASan + UBSan + Leak
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:
                -fsanitize=address,undefined,leak
                -fno-omit-frame-pointer
             >"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:
                -fsanitize=address,undefined,leak
             >"
        )
    elseif(PLATFORM_TOOLCHAIN STREQUAL "FREEBSD_CLANG")
        # FreeBSD + Clang: 先稳妥用 ASan + UBSan
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
             >"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:
                -fsanitize=address,undefined
             >"
        )
    endif()
endif()

# ==================================================
# 多线程库
# ==================================================
find_package(Threads REQUIRED)

# ==================================================
# 源文件列表
# ==================================================
set(SRC_FILES
    src/main.cpp
)

# ==================================================
# 主可执行文件：my_app
# ==================================================
add_executable(my_app
    ${SRC_FILES}
)

target_link_libraries(my_app
    PRIVATE
        Threads::Threads
)

if(PROJECT_SANITIZER_COMPILE_FLAGS)
    target_compile_options(my_app PRIVATE ${PROJECT_SANITIZER_COMPILE_FLAGS})
endif()
if(PROJECT_SANITIZER_LINK_FLAGS)
    target_link_options(my_app PRIVATE ${PROJECT_SANITIZER_LINK_FLAGS})
endif()

# ==================================================
# 线程 Sanitizer 版本：my_app_tsan
# 仅在 Linux / macOS 且 ENABLE_SANITIZERS 开启时生成
# ==================================================
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
        "$<$<CONFIG:Debug>:
            -fsanitize=thread
            -fno-omit-frame-pointer
         >"
    )
    target_link_options(my_app_tsan PRIVATE
        "$<$<CONFIG:Debug>:
            -fsanitize=thread
         >"
    )
endif()

# ==================================================
# 只对 main 源文件启用 Werror(/WX)
# ==================================================
set(MAIN_SOURCE "src/main.cpp")

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
