# ATRACodec native converter

The GUI is a separate small launcher for the sibling ATRACodec command-line
program. It contains no codecs and requires no Sony components. Windows uses
Win32/common controls; macOS uses native AppKit controls. C++17 is sufficient.

The automated work compiles and links these programs but **does not launch or
control the GUI**. Native visual testing is reserved for the user. In particular,
macOS cross-linking on Windows does not establish macOS runtime behavior,
distribution signing, notarization, or a tested visual layout.

## Build and package

From the source root, with Zig 0.14.1:

```text
python product/gui/build_gui.py --zig PATH/zig --out build/gui \
  --cli-windows-x64 build/windows-x64/atracodec.exe \
  --cli-macos-x64 build/macos-x64/atracodec \
  --cli-macos-arm64 build/macos-arm64/atracodec
```

`--target windows-x64`, `macos-x64`, or `macos-arm64` builds one target. CLI options
are optional for building; a runnable application package must contain its
matching CLI. The helper never edits the main CMake, build script, or codec API.

Windows entry: `converter_windows.cpp`, resource `converter_windows.rc` (resource
compiler working directory is this directory). Link `user32`, `gdi32`,
`comdlg32`, `comctl32`; use Unicode startup, Windows subsystem, C++17. With Zig:

```text
zig rc /fo converter.res converter_windows.rc
zig c++ -target x86_64-windows-gnu -std=c++17 -O2 -municode \
  -Wl,--subsystem,windows -Wl,-e,WinMainCRTStartup \
  converter_windows.cpp converter.res -luser32 -lgdi32 -lcomdlg32 -lcomctl32 \
  -o atracodec-converter.exe
```

The Windows delivery layout is `atracodec-converter.exe` and `atracodec.exe` in
the same directory. Windows 10 or later is the intended GUI host; the runtime
imports are Windows system libraries/UCRT. No external C++ runtime DLL is used.

macOS entry: `converter_macos.cpp`. Target `x86_64-macos.11.0` or
`aarch64-macos.11.0`. Link flags:

```text
-std=c++17 -O2
-Fproduct/gui/macos-stubs/System/Library/Frameworks
-Lproduct/gui/macos-stubs/usr/lib
-framework AppKit -framework Foundation -lobjc
```

The two `.tbd` framework descriptions and `libobjc.tbd` contain only system
install names and the few imports this source uses. They are build files, not
substitute runtime libraries, and are never included in the `.app`. Zig's real
Darwin libc headers and `libSystem.tbd` supply the POSIX interfaces. The source
declares the small Objective-C ABI surface itself; no Apple SDK is redistributed.

The generated macOS layout is:

```text
ATRACodec Converter.app/Contents/
  Info.plist
  MacOS/atracodec-converter
  MacOS/atracodec
  Resources/{en,zh-Hans,zh-Hant}.lproj/InfoPlist.strings
```

The parent build can combine the two matching thin GUI binaries into a universal
Mach-O, do the same for the two CLIs, then call `package_app(folder, gui, cli)`
from `build_gui.py`. `zip_app(app, zip_path)` assigns Unix mode 0755 to both
executables even on Windows; use it or preserve those modes in the final package.
The helper's thin `.app.zip` files are build evidence, not the final universal
release. Any release signing/notarization remains a separate native macOS step.

## CMake integration

The main project includes `product/gui/cmake-native.cmake` after creating its
`atracodec` CLI target. `ATRACODEC_BUILD_GUI=ON` adds `atracodec-converter` on
Windows/macOS; other hosts are skipped. The fragment uses the existing global
runtime output path. Windows compiles the RC next to a copied manifest, supports
`SONY_ZIG_EXECUTABLE` or a native resource compiler, and installs GUI/CLI in `bin`.

macOS creates `atracodec-converter.app` with `Info.plist.in`, minimum macOS 11,
three localization directories, and the actual CLI copied into `Contents/MacOS`.
The native SDK is preferred on Apple hosts. Windows-to-Darwin Zig builds default
to the local build-only import descriptions when an SDK is absent;
`ATRACODEC_GUI_USE_APPLE_STUBS` selects this explicitly. `LINK_DEPENDS` on the CLI
refreshes the bundle after that executable changes, even without GUI source
changes. CMake installs the whole `.app` at the install prefix; the existing main
install rules also install the separate SDK and CLI. CMake's bundle directory name
differs from the standalone helper's display-name directory; the internal GUI/CLI
executable names and native application behavior are identical.

## Window and localization

One resizable window contains operation and language selectors; input/output
paths and native pickers; a 22-item encoding format selector; replacement opt-in;
Start and Cancel; a determinate/indeterminate progress bar; a readable status;
and a selectable, bounded activity log. Decode disables the encoding selector.
The default encoding is AALP with a 352 kbps core. AAL labels explain that these
are lossless formats and that their core rate is not the final file bitrate.

The Chinese/English selector translates all application-owned controls, hints,
profile descriptions, validation messages, status, completion and cancellation
messages, close confirmation, and macOS Edit/Quit menus. Initial selection follows
the system preferred UI language. Source paths, external CLI diagnostic details,
and OS-owned file picker/message-box buttons remain native data/system text.
Language and path/profile controls are disabled for the active task, then restored
after the worker has reaped its child.

Windows uses the system message font, per-monitor DPI awareness, common-controls
v6, keyboard tab navigation, and Enter/Escape for Start/Cancel. macOS uses native
fields/buttons/popups, system fonts, an ordinary menu bar, Command-Q/Edit
shortcuts, and Return/Escape actions. View sizes are updated by native resizing.
No screen capture or visual/layout assertion is claimed without user testing.

## Child lifetime and progress

Every launch uses an explicit absolute sibling executable. Windows calls
`CreateProcessW` with a correctly quoted CRT command line and a handle allowlist;
macOS calls `posix_spawn` with a null-terminated argument array and Darwin
close-by-default descriptors, retaining the explicit standard-stream actions. Neither invokes
a command shell. Output paths are passed as data, including Unicode, spaces, and
shell metacharacters.

Both workers pass `--progress --cancel-file UNIQUE_PATH`, merge stdout/stderr,
parse bounded JSON lines, and forward UI updates to the main thread. Progress
uses processed/total PCM frames; an unknown total animates without a false
percentage. Pending progress is flushed before a later result/error line, so
throttling preserves the event order. The bounded log also accepts ordinary
`Error: ...` text for compatibility.

Cancel first creates only the task's unique flag. The CLI owns conversion cleanup
and atomic output commit. After a three-second grace interval the GUI may terminate
its own child: Windows uses the assigned job object (with kill-on-close); macOS
signals only the unreaped PID returned by its own successful spawn, with TERM then
KILL. It never searches for or kills a process by name. Handles/descriptors and
the flag reservation are scoped to the worker; the child is reaped before the
controls are enabled or the application exits. Close while converting offers to
cancel and wait. No independent background conversion is left behind.

## Apple ABI choices

`macos_abi.h` matches Apple's opaque Class/id/SEL/IMP declarations and chooses
BOOL according to the compiler's Objective-C BOOL macro, otherwise macOS's signed
char definition. All Objective-C calls cast `objc_msgSend` to the actual return
and argument types. The code never requests aggregate returns: Intel's
`objc_msgSend_stret` is therefore unnecessary. NSRect/NSSize arguments use the
64-bit CGRect/CGSize layout; selector callbacks use the compiler's corresponding
struct argument ABI. Window construction uses the documented
`windowWithContentViewController:` convenience constructor, avoiding guessed
window-style bit fields. AppKit control operations stay on the main thread;
worker events use `performSelectorOnMainThread:withObject:waitUntilDone:`.
Before the first C++ worker, an immediately returning NSThread initializes Cocoa
threaded mode if needed, following Apple's documented POSIX-thread guidance.

Primary Apple/Microsoft references are listed in `references.json` and the
research record. Only declarations, public documented behavior, and system
imports are used; no third-party UI runtime or copied SDK implementation is used.

## Automated and manual checks

`research/gui/run_qa.py` is a console-only harness. It tests all profile IDs,
both language tables, argument construction, Unicode/escaping, chunked and
malformed JSON, large counters, log bounds, session transitions, and the production
Windows process helper with a fake console child. It additionally invokes the
real CLI through that same helper for all 22 encode/decode profiles and real
cooperative cancellation. Ten AAL roundtrips compare PCM exactly. It audits PE
imports and both Mach-O load commands and binds the source/artifact hashes.

User manual checks remain: native startup on each OS/architecture; complete
Chinese/English display and keyboard focus; resizing/high DPI; file pickers;
successful encode/decode; invalid input; existing-output opt-in; cancellation;
and closing while converting. These are explicitly not executed by the agent.
