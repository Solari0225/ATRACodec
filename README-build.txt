ATRACodec 0.5.0-test1 — 源码构建 / Source build

使用Python3.9+及Zig0.14.1。运行产物不需要Python、Sony组件或第三方ATRAC库。
Python3.9+ and Zig0.14.1 build the sources. Python and codec components are not runtime dependencies.

  python build.py --zig /path/to/zig --target all
  python build.py --target windows-x64
  python build.py --target macos-arm64
  python build.py --target macos-x64

输出build/<target>/，默认编译CLI、共享/静态库、C11示例和原生双语GUI。
--no-gui / --no-example关闭可选产物，--jobs设置并行数，--build-dir指定隔离目录。
Outputs include CLI, shared/static libraries, C11 examples and native bilingual GUI.
Use --no-gui, --no-example, --jobs N or --build-dir DIR as appropriate.
GUI构建从不启动界面；macOS在Windows上交叉编译不等于本机运行验证。
The builder never launches a GUI; a cross-build is not a native macOS runtime test.

Windows: atracodec.exe, atracodec-converter.exe, atracodec.dll,
atracodec-static.lib, atracodec-import.lib, 三个C11示例 / three C11 examples.
macOS: atracodec, atracodec-converter, libatracodec.a, libatracodec.dylib,
ATRACodec Converter.app, 三个C11示例 / three C11 examples.
GUI与CLI必须相邻；Mac .app内已包含匹配CLI。动态库示例与库放同目录。
Keep GUI and CLI together. Each app includes its matching CLI. Shared-library
examples require their matching library in the same directory.

  atracodec encode --profile aalp-352 INPUT.wav OUTPUT.oma --progress
  atracodec decode INPUT.oma OUTPUT.wav --progress
  atracodec --list-profiles --json

CMake3.20+及C11/C++17工具链也可构建。原生MSVC cl未验证，部分数学源依赖
Clang/GNU扩展；公共C ABI可供其他应用调用。Zig方式示例：
CMake3.20+ supports the same code. Native MSVC compiler support is unverified;
some recovered math uses Clang/GNU extensions. The public C ABI is independent.

  cmake -S . -B build-cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/zig-toolchain.cmake -DSONY_ZIG_EXECUTABLE=/path/to/zig -DSONY_ZIG_TARGET=windows-x64 -DCMAKE_BUILD_TYPE=Release
  cmake --build build-cmake --parallel 6
  cmake --install build-cmake --prefix install

SONY_ZIG_TARGET可选macos-arm64或macos-x64，此变量名沿用原构建工具约定。
CMake选项ATRACODEC_BUILD_GUI及ATRACODEC_BUILD_EXAMPLE控制可选目标。
SONY_ZIG_TARGET can also be macos-arm64 or macos-x64. It is an internal build
variable, not a runtime component dependency. GUI/example targets are optional.

public headers: product/atracodec.h, product/atracodec_stream.h.
examples/stream_roundtrip.c演示只用固定小PCM块的实时回调串联，不存整段文件。
The stream example connects an encoder to a separate decoder using callbacks
and checks lossless PCM without retaining either complete stream.

sources.json锁定源码/数表/构建输入的SHA256；修改代码后运行python record_sources.py。
source-manifest.json锁定交付树。无原版二进制、音频、研究捕获或工具包含在包内。
Sources and numeric tables are hash-bound. Run python record_sources.py after
intentional edits. The source package excludes original binaries, audio and captures.
保持-fno-fast-math及-ffp-contract=off，以维持已验证的Sony数学顺序。
Keep strict floating-point flags; enabling fast-math/FMA may change encoded bytes.
