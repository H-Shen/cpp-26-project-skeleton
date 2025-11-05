cmake_minimum_required(VERSION 3.20)

project(MyProject LANGUAGES CXX)

# ------------------------------------------------------------
# 平台 / 编译器组合检查
# 允许：
#   - Windows + MSVC
#   - macOS  + Clang/AppleClang
#   - Linux  + GCC
# ------------------------------------------------------------
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
    # 非 APPLE 的 UNIX 视为 Linux
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "On Linux, only GCC is supported.")
    endif()
    set(PLATFORM_TOOLCHAIN "LINUX_GCC")

else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

# ------------------------------------------------------------
# C++ 标准：C++23，无扩展
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器默认 Release
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ============================================================
# 全局告警 & 优化等级 & 基础 Debug 设置
# ============================================================

if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
    message(STATUS "Configuring for Windows + MSVC")

    # 通用：高等级告警 + UTF-8 + bigobj
    add_compile_options(
        /W4
        /utf-8
        /bigobj
    )

    # 静态运行库：Debug /MTd，Release /MT
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    # Debug：无优化 + 调试信息 + MSVC 提供的各类安全/诊断开关
    # /RTC1    运行时检查（未初始化变量/栈帧/溢出等）
    # /sdl     安全检查（SDL）
    # /analyze 静态分析（潜在泄漏/危险路径等）
    # /permissive- 更严格的标准 C++
    add_compile_options(
        "$<$<CONFIG:Debug>:/Od;/Zi;/RTC1;/sdl;/analyze;/permissive->"
    )

    # Release：O2 + NDEBUG
    add_compile_options(
        "$<$<CONFIG:Release>:/O2;/DNDEBUG>"
    )

elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM"
       OR PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
    message(STATUS "Configuring for Unix-like + ${CMAKE_CXX_COMPILER_ID}")

    # 通用高强度告警
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

    # Debug：O0 + g
    add_compile_options(
        "$<$<CONFIG:Debug>:-O0;-g>"
    )

    # Release：O2 + NDEBUG
    add_compile_options(
        "$<$<CONFIG:Release>:-O2;-DNDEBUG>"
    )
endif()

# ============================================================
# Sanitizer（仅 Linux/macOS Debug）
# Windows 依赖 MSVC 自己的 /RTC1 /sdl /analyze 等
# ============================================================
option(ENABLE_SANITIZERS "Enable sanitizers in Debug builds (non-Windows only)" ON)

set(PROJECT_SANITIZER_COMPILE_FLAGS "")
set(PROJECT_SANITIZER_LINK_FLAGS "")

if(ENABLE_SANITIZERS)
    if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
        message(STATUS "Sanitizers not configured for MSVC; using /RTC1, /sdl, /analyze instead.")
    elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM")
        # macOS: ASan + UBSan
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined;-fno-omit-frame-pointer>"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"
        )
    elseif(PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
        # Linux + GCC: ASan + UBSan + Leak
        set(PROJECT_SANITIZER_COMPILE_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined,leak;-fno-omit-frame-pointer>"
        )
        set(PROJECT_SANITIZER_LINK_FLAGS
            "$<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>"
        )
    endif()
endif()

# ============================================================
# 多线程库
# ============================================================
find_package(Threads REQUIRED)

# ============================================================
# 源文件列表
# ============================================================
set(SRC_FILES
    src/main.cpp
)

# ============================================================
# 主可执行文件：my_app
# ============================================================
add_executable(my_app
    ${SRC_FILES}
)

target_link_libraries(my_app
    PRIVATE
        Threads::Threads
)

# 给 my_app 挂上 sanitizer（仅 Linux/macOS Debug）
if(PROJECT_SANITIZER_COMPILE_FLAGS)
    target_compile_options(my_app PRIVATE ${PROJECT_SANITIZER_COMPILE_FLAGS})
endif()
if(PROJECT_SANITIZER_LINK_FLAGS)
    target_link_options(my_app PRIVATE ${PROJECT_SANITIZER_LINK_FLAGS})
endif()

# ============================================================
# 平台专属的 Debug 加强项（STL / 栈保护 等）
# ============================================================

# Windows: STL 迭代器 Debug & 更严格语义
if(PLATFORM_TOOLCHAIN STREQUAL "WIN_MSVC")
    target_compile_definitions(my_app PRIVATE
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=2>
    )
    target_compile_options(my_app PRIVATE
        $<$<CONFIG:Debug>:/Zc:preprocessor;/Zc:strictStrings;/Zc:throwingNew;/Zc:inline>
    )
endif()

# Linux: libstdc++ Debug 模式 + 栈保护 + frame pointer
if(PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
    target_compile_definitions(my_app PRIVATE
        $<$<CONFIG:Debug>:_GLIBCXX_DEBUG;_GLIBCXX_DEBUG_PEDANTIC;_GLIBCXX_ASSERTIONS>
    )
    target_compile_options(my_app PRIVATE
        $<$<CONFIG:Debug>:-fstack-protector-strong;-fno-omit-frame-pointer>
    )
endif()

# macOS: libc++ hardening debug 模式 + 可选 iterator debug + 栈保护 + frame pointer
if(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM")
    target_compile_definitions(my_app PRIVATE
        $<$<CONFIG:Debug>:
            _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG;
            _LIBCPP_DEBUG=1
        >
    )
    target_compile_options(my_app PRIVATE
        $<$<CONFIG:Debug>:-fstack-protector-strong;-fno-omit-frame-pointer>
    )
endif()

# ============================================================
# 线程 Sanitizer 版本：my_app_tsan
# 仅在 Linux/macOS 且 ENABLE_SANITIZERS 开启时生成
# ============================================================
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

    # my_app_tsan 同样享受 STL / hardening / 栈保护

    if(PLATFORM_TOOLCHAIN STREQUAL "LINUX_GCC")
        target_compile_definitions(my_app_tsan PRIVATE
            $<$<CONFIG:Debug>:_GLIBCXX_DEBUG;_GLIBCXX_DEBUG_PEDANTIC;_GLIBCXX_ASSERTIONS>
        )
        target_compile_options(my_app_tsan PRIVATE
            $<$<CONFIG:Debug>:-fstack-protector-strong;-fno-omit-frame-pointer>
        )
    elseif(PLATFORM_TOOLCHAIN STREQUAL "MAC_LLVM")
        target_compile_definitions(my_app_tsan PRIVATE
            $<$<CONFIG:Debug>:
                _LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG;
                _LIBCPP_DEBUG=1
            >
        )
        target_compile_options(my_app_tsan PRIVATE
            $<$<CONFIG:Debug>:-fstack-protector-strong;-fno-omit-frame-pointer>
        )
    endif()
endif()

# ============================================================
# 只对 main 源文件启用 Werror(/WX)
# ============================================================
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
