set_project("Cpp26ProjectSkeleton")
set_version("0.2.0")
set_xmakever("3.1.2")
set_allowedplats("windows", "macosx", "linux")

add_rules("mode.debug", "mode.release")

if is_plat("windows") then
    set_toolchains("msvc")
elseif is_plat("macosx") then
    set_toolchains("clang")
elseif is_plat("linux") then
    set_toolchains("gcc")
end

local function configure_common_target()
    set_kind("binary")
    set_languages("cxx26")
    set_policy("build.c++.modules.std", true)

    add_files(
        "src/main.cpp",
        "src/cpp26_skeleton.cpp",
        "src/cpp26_skeleton.cppm"
    )

    set_warnings("allextra", "pedantic", "error")

    if is_mode("debug") then
        set_optimize("none")
    else
        set_optimize("faster")
    end

    if is_plat("windows") then
        set_runtimes(is_mode("debug") and "MTd" or "MT")
        add_cxxflags("/utf-8", "/bigobj", {force = true})

        if is_mode("debug") then
            add_defines("_ITERATOR_DEBUG_LEVEL=2")
            add_cxxflags(
                "/RTC1",
                "/sdl",
                "/analyze",
                "/permissive-",
                "/Zc:preprocessor",
                "/Zc:strictStrings",
                "/Zc:throwingNew",
                "/Zc:inline",
                {force = true}
            )
        end
    else
        add_cxxflags(
            "-Wconversion",
            "-Wshadow",
            "-Wnon-virtual-dtor",
            "-Wold-style-cast",
            "-Wfloat-equal",
            "-Wcast-align",
            "-Wnull-dereference",
            "-Wunused",
            "-Wuninitialized",
            "-Wformat=2",
            "-Wundef",
            "-Wpointer-arith",
            "-Wcast-qual",
            {force = true}
        )

        if is_mode("debug") then
            add_cxxflags("-fstack-protector-strong", "-fno-omit-frame-pointer")

            if is_plat("linux") then
                add_defines("_GLIBCXX_DEBUG", "_GLIBCXX_DEBUG_PEDANTIC", "_GLIBCXX_ASSERTIONS")
            elseif is_plat("macosx") then
                add_defines("_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG")
            end
        end
    end

    if is_plat("linux") then
        add_syslinks("pthread")
    end
end

target("cpp26_skeleton")
    configure_common_target()
    add_tests("smoke")

    if is_mode("debug") and is_plat("linux") then
        add_cxxflags("-fsanitize=address,undefined,leak", "-fno-omit-frame-pointer")
        add_ldflags("-fsanitize=address,undefined,leak")
    elseif is_mode("debug") and is_plat("macosx") then
        add_cxxflags("-fsanitize=address,undefined", "-fno-omit-frame-pointer")
        add_ldflags("-fsanitize=address,undefined")
    end

if is_mode("debug") and is_plat("linux", "macosx") then
    target("cpp26_skeleton_tsan")
        configure_common_target()
        set_default(false)
        add_cxxflags("-fsanitize=thread", "-fno-omit-frame-pointer")
        add_ldflags("-fsanitize=thread")
        add_tests("tsan")
end
