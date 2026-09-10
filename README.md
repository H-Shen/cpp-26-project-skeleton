# Cpp23ProjectDemo

Minimal cross-platform C++23 project template using Xmake.

## Supported toolchains

- Windows: MSVC with static CRT (`/MTd` in Debug, `/MT` in Release)
- macOS: Clang / Apple Clang
- Linux: GCC

Other platform/toolchain combinations are intentionally unsupported.

## Build

Xmake 2.8.5 or newer is required.

### Debug

```bash
xmake f -m debug -c
xmake
xmake test
```

On Linux and macOS, Debug builds enable ASan + UBSan. Linux also enables LeakSanitizer. `xmake test` additionally builds and runs the separate TSan target on Linux and macOS.

Debug builds also enable STL/runtime hardening where available:

- Linux: libstdc++ debug/assertion modes
- macOS: libc++ debug hardening mode
- Windows: MSVC iterator debugging, runtime checks, SDL checks, and static analysis

### Release

```bash
xmake f -m release -c
xmake
xmake test
```

Release builds use C++23 with O2-equivalent optimization and no sanitizers.

## Targets

- `my_app`: main executable and smoke test
- `my_app_tsan`: Debug-only ThreadSanitizer executable on Linux/macOS; excluded from normal builds but automatically built and executed by `xmake test`

## CI

GitHub Actions builds and tests Debug and Release configurations on Ubuntu, macOS, and Windows. Warnings are treated as errors for project targets.
