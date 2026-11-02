# Cpp_26_Project_Skeleton

Minimal cross-platform C++26 project template using Xmake and standard C++ named modules.

The template deliberately treats Modules as the default project structure:

- `src/cpp26_skeleton.cppm` — primary module interface
- `src/cpp26_skeleton.cpp` — module implementation unit
- `src/main.cpp` — importer using `import std;` and `import cpp26_skeleton;`

There is no project header for the C++ interface. The module boundary is the interface boundary.

## Supported toolchains

The reference configurations are intentionally narrow and are the same configurations exercised by CI:

- Linux: GCC 16 on Ubuntu 26.04
- macOS: Apple Clang on macOS 26
- Windows: MSVC from Visual Studio 2026 on Windows Server 2025

Other platform/toolchain combinations may work but are not part of the supported matrix.

## Requirements

- Xmake 3.1.2 or newer
- A compiler/toolchain capable of C++26 mode and standard C++ named modules
- Standard-library module support for `import std;`

The project explicitly enables Xmake's C++ module and standard-library-module policies. Xmake handles module dependency scanning, BMI/CMI build order, and compiler-specific module artifacts.

`import std;` remains dependent on the compiler and standard-library implementation. CI is therefore intentionally used as a compatibility gate rather than assuming identical behavior across libstdc++, libc++, and the MSVC STL.

## Build

### Debug

```bash
xmake f -m debug -c
xmake
xmake test
```

On Linux and macOS, Debug builds enable ASan + UBSan. Linux also enables LeakSanitizer. `xmake test` additionally builds and runs a separate ThreadSanitizer target on Linux and macOS.

Debug builds also enable runtime/STL hardening where available:

- Linux: libstdc++ debug/assertion modes
- macOS: libc++ debug hardening mode
- Windows: MSVC iterator debugging, runtime checks, SDL checks, and static analysis

### Release

```bash
xmake f -m release -c
xmake
xmake test
```

Release builds use C++26 mode with O2-equivalent optimization and no sanitizers.

## Linux GCC 16

The reference Linux configuration uses GCC 16 explicitly:

```bash
xmake f -m debug -c --cc=gcc-16 --cxx=g++-16
xmake
xmake test
```

Use `-m release` for the Release configuration.

## Targets

- `cpp26_skeleton` — main executable and smoke test
- `cpp26_skeleton_tsan` — Debug-only ThreadSanitizer executable on Linux/macOS; excluded from normal builds but included by `xmake test`

## Module layout

The module interface exports the public API:

```cpp
export module cpp26_skeleton;

import std;

export namespace cpp26_skeleton {

[[nodiscard]] std::string greeting(std::string_view name);
[[nodiscard]] int answer() noexcept;

}
```

The implementation remains separate:

```cpp
module cpp26_skeleton;

import std;

namespace cpp26_skeleton {

// definitions

}
```

Consumers import the module instead of including a project header:

```cpp
import std;
import cpp26_skeleton;
```

## CI

GitHub Actions builds and tests Debug and Release configurations on the pinned reference environments:

- Ubuntu 26.04 with GCC 16
- macOS 26 with Apple Clang
- Windows Server 2025 with Visual Studio 2026

CI installs Xmake 3.1.2 explicitly. Warnings are treated as errors for project targets.
