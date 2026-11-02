# Cpp_26_Project_Skeleton

Minimal cross-platform C++26 project template using Xmake and C++ named modules.

The template deliberately treats Modules as the default project structure:

- `src/cpp26_skeleton.cppm` — primary module interface
- `src/cpp26_skeleton.cpp` — module implementation unit
- `src/main.cpp` — importer using `import cpp26_skeleton;`

There is no project header for the C++ interface. The module boundary is the interface boundary.

The standard library is consumed through ordinary headers rather than `import std;`. Xmake's automatic standard-library module build is disabled with `build.c++.modules.std = false`.

## Supported toolchains

The reference configurations are intentionally narrow and are the same configurations exercised by CI:

- Linux: GCC 16 on Ubuntu 26.04
- macOS: LLVM Clang 21 on macOS 26
- Windows: MSVC from Visual Studio 2026 on Windows Server 2025

Other platform/toolchain combinations may work but are not part of the supported matrix.

## Requirements

- Xmake 3.1.2 or newer
- A compiler/toolchain capable of C++26 mode and standard C++ named modules

Xmake handles module dependency scanning, BMI/CMI build order, and compiler-specific module artifacts for the project module.

## Build

On Linux and macOS, select the toolchain using the platform-specific commands below first. Subsequent mode changes keep that selection.

### Debug

```bash
xmake f -m debug
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
xmake f -m release
xmake
xmake test
```

Release builds use C++26 mode with O2-equivalent optimization and no sanitizers.

## Linux GCC 16

The reference Linux configuration uses GCC 16 explicitly:

```bash
xmake f -m debug -c --toolchain=gcc-16
xmake
xmake test
```

Use `-m release` for the Release configuration. Selecting the toolchain also selects GCC 16 for linking.

## macOS LLVM 21

Use upstream LLVM for named modules. The tested Apple Clang 21 accepts C++26 mode but rejects the module declarations.

```bash
brew install llvm@21
xmake f -m debug -c --toolchain=llvm --sdk="$(brew --prefix llvm@21)"
xmake
xmake test
```

Use `-m release` for the Release configuration. The LLVM toolchain keeps the compiler and dependency scanner from the same installation.

## Targets

- `cpp26_skeleton` — main executable and smoke test
- `cpp26_skeleton_tsan` — Debug-only ThreadSanitizer executable on Linux/macOS; excluded from normal builds but included by `xmake test`

## Module layout

The module interface exports the public API while using standard-library headers in the global module fragment:

```cpp
module;

#include <string>
#include <string_view>

export module cpp26_skeleton;

export namespace cpp26_skeleton {

[[nodiscard]] std::string greeting(std::string_view name);
[[nodiscard]] int answer() noexcept;

}
```

The implementation remains separate:

```cpp
module;

#include <format>
#include <string>
#include <string_view>

module cpp26_skeleton;

namespace cpp26_skeleton {

// definitions

}
```

Consumers include whichever standard-library headers they need and import the project module:

```cpp
#include <print>

import cpp26_skeleton;
```

## CI

GitHub Actions builds and tests Debug and Release configurations on the pinned reference environments:

- Ubuntu 26.04 with GCC 16
- macOS 26 with LLVM Clang 21
- Windows Server 2025 with Visual Studio 2026

CI installs Xmake 3.1.2 explicitly. Warnings are treated as errors for project targets.
