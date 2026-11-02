# Building / 构建

## Toolchain / 工具链

已验证工具链为 Python 3.9+、Zig 0.14.1；CMake 方式另需 CMake 3.20+ 与 Ninja。源代码为 C11 / C++17。产物运行时不需要 Python、Zig、Sony 组件或第三方 ATRAC 库。

The verified toolchain is Python 3.9+ and Zig 0.14.1. The CMake route additionally uses CMake 3.20+ and Ninja. Sources use C11 / C++17. Python, Zig, Sony components, and third-party ATRAC libraries are not runtime dependencies.

```sh
python tools/verify_source.py
python build.py --zig /path/to/zig --target all
```

单目标 / Single target:

```sh
python build.py --zig /path/to/zig --target windows-x64
python build.py --zig /path/to/zig --target macos-arm64
python build.py --zig /path/to/zig --target macos-x64
```

`--target all` 生成三个目标各自的构建目录，不自动把两个 macOS 切片合成 Universal。Release 已提供 Universal 二进制；自行合并可在 Mac 使用系统 `lipo`。

`--target all` creates separate directories for all three targets; it does not automatically merge macOS slices. Releases provide merged Universal binaries. A local macOS build can merge slices using the system `lipo` tool.

`--jobs N` 控制并行数，`--build-dir DIR` 改输出目录，`--no-gui` / `--no-example` 关闭可选目标。GUI 与 CLI 要保持相邻；Mac `.app` 内已包含 CLI。构建不启动 GUI。

Use `--jobs N`, `--build-dir DIR`, `--no-gui`, and `--no-example` as needed. Keep the GUI beside its CLI; each Mac `.app` contains its matching CLI. Building never launches the GUI.

## CMake

```sh
cmake -S . -B build-cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/zig-toolchain.cmake -DSONY_ZIG_EXECUTABLE=/path/to/zig -DSONY_ZIG_TARGET=windows-x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake --parallel 6
cmake --install build-cmake --prefix install
```

`SONY_ZIG_TARGET` 还可为 `macos-arm64` / `macos-x64`。这些 `SONY_` 变量只是沿用的内部构建变量名，不代表官方组件依赖。可选目标开关为 `ATRACODEC_BUILD_GUI`、`ATRACODEC_BUILD_EXAMPLE`。

`SONY_ZIG_TARGET` also accepts `macos-arm64` / `macos-x64`. The historical `SONY_` variable names are internal build conventions, not component dependencies. Optional targets use `ATRACODEC_BUILD_GUI` and `ATRACODEC_BUILD_EXAMPLE`.

原生 MSVC `cl` 未验证；部分数学代码使用 Clang/GNU 扩展。公共 C ABI 可由宿主应用链接。macOS `.tbd` 文件是最小系统符号导入描述，仅用于交叉链接，不是运行替代库，也不是分发 Apple SDK。

Native MSVC `cl` support is unverified; some math code uses Clang/GNU extensions. Applications can consume the public C ABI. The minimal macOS `.tbd` descriptions support cross-linking system symbols; they are not runtime replacements or a redistributed Apple SDK.

## Source integrity / 源码完整性

`sources.json` 锁定 296 个编解码/CLI/示例输入和 20 个 GUI 构建输入。`source-manifest.json` 保留原始 0.5.0-test1 源码交付树的 327 项记录（清单文件自身除外）；本次新增 README、许可证及发布文档不在该原始清单内。

`sources.json` binds 296 codec/CLI/example inputs and 20 GUI build inputs. `source-manifest.json` preserves 327 entries from the original 0.5.0-test1 source delivery, excluding itself. Newly added READMEs, licensing, and publication documentation are outside that baseline inventory.

```sh
python tools/verify_source.py
```

有意修改代码后可用原始 `python record_sources.py` 更新构建输入哈希；同时需要重新进行相应一致性验收。它不会自动更新原始交付树清单。保持 `-fno-fast-math`、`-ffp-contract=off` 等已验证浮点选项；改变数学计算顺序可能改变编码字节。

After intentional code changes, `python record_sources.py` updates build-input hashes; corresponding conformance checks are still required. It does not refresh the original delivery inventory. Keep the validated strict floating-point settings, including `-fno-fast-math` and `-ffp-contract=off`; changing arithmetic order can change encoded bytes.

## Build outputs / 构建产物

| Target | Primary outputs / 主要产物 |
| --- | --- |
| Windows x64 | `atracodec.exe`, `atracodec-converter.exe`, `atracodec.dll`, `atracodec-static.lib`, `atracodec-import.lib` |
| macOS arm64 / x64 | `atracodec`, `atracodec-converter`, `libatracodec.a`, `libatracodec.dylib`, `ATRACodec Converter.app` |
| All / 全部 | Three C11 consumer examples / 三个 C11 使用示例 |

已做三目标干净构建，静态库与验收构建逐字节一致；PE 时间戳及全部最终链接文件的可重复性不作保证。macOS 仅交叉构建和链接，原生执行待测试。

Clean builds completed for all three targets, with byte-identical static archives. PE timestamps and reproducibility of every final linked file are not guaranteed. Native macOS execution remains untested.
