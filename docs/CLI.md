# Command line / 命令行

## Quick start / 快速使用

Windows 使用 `atracodec.exe`，macOS 使用 `./atracodec`。输入 WAV 必须为 44.1 kHz、16-bit signed integer、双声道；输出编码容器是未加密 OMA。

Use `atracodec.exe` on Windows or `./atracodec` on macOS. WAV input must contain 44.1 kHz, signed 16-bit, stereo PCM. Encoding produces unencrypted OMA.

```sh
atracodec --version
atracodec --help
atracodec --list-profiles --json
atracodec encode --profile aalp-352 input.wav output.oma
atracodec encode --profile aal3-132 input.wav output.oma
atracodec encode --profile atrac3-66 input.wav output.oma
atracodec encode --profile atrac3plus-352 input.wav output.oma
atracodec decode output.oma restored.wav
```

`--profile` 省略时默认 `aalp-352`。有损模式不会还原输入 PCM；AAL3/AALP 同时包含有损核心及补偿数据，可还原输入 PCM。

The default profile is `aalp-352`. Lossy modes do not reproduce source PCM. AAL3/AALP include the core and residual data needed for exact PCM reconstruction.

## Process contract / 进程接口

| Option / 选项 | Behavior / 行为 |
| --- | --- |
| `--overwrite` | 显式允许成功后替换已有目标 / Permit replacement after successful conversion |
| `--progress` | stderr 输出 JSON Lines 进度和错误 / Emit progress/errors as JSON Lines on stderr |
| `--cancel-file PATH` | 文件存在即合作取消 / Cancel cooperatively when the file exists |
| `--list-profiles --json` | 列出 22 个 profile 的机器可读规格 / List all 22 profiles as JSON |

stdout 用于最终 JSON 结果，stderr 用于进度或错误。退出码：成功 `0`、错误 `1`、取消 `2`。传给 CLI 的参数须使用宿主语言的参数数组，不拼接成 shell 命令。

stdout carries the final JSON result; stderr carries progress/errors. Exit codes are `0` success, `1` failure, and `2` cancellation. Pass arguments as a native argument array in a host application rather than concatenating a shell command.

```sh
atracodec encode --profile aalp-352 input.wav output.oma --progress --cancel-file cancel.flag
```

开始前应确保 `cancel.flag` 不存在；转换期间创建它即可取消，也可使用进程处理的 SIGINT/SIGTERM。已有目标在失败或取消时保留；默认禁止覆盖，并在最终提交时再次检查。输出先写入目标同目录的临时文件，成功后才提交。

Ensure `cancel.flag` is absent before starting; create it to cancel. Handled SIGINT/SIGTERM also cancel. Failed/cancelled conversions preserve an existing destination. Overwrite protection is checked again at commit. Output is staged in a sibling temporary file and committed only after success.

## Containers and lengths / 容器与长度

RIFF/WAVE 与 RF64/WAVE 均可输入；解码按实际长度输出 RIFF 或 RF64。WAV 预留 JUNK 区块，必要时改为 ds64，因此不要求 WAV 文件头与 Sony 相同，只比较 PCM。额外超大非 data 区块所需的 RF64 扩展表不在当前读取范围内。

RIFF/WAVE and RF64/WAVE are accepted; decoded output is RIFF or RF64 according to size. Reserved JUNK space becomes ds64 if needed. WAV headers can differ from Sony while PCM remains identical. RF64 tables for additional oversized non-data chunks are outside the current reader.

普通有损流保留 Sony 解码时序及尾部填充：ATRAC3 输出 `max(core_count - 1, 0) * 1024` 帧；ATRAC3plus 输出 `max(core_count * 2048 - 2416, 0)` 帧。不能对有损流套用 AAL 的源长度还原约定。

Lossy streams retain Sony timing/padding: ATRAC3 emits `max(core_count - 1, 0) * 1024` frames; ATRAC3plus emits `max(core_count * 2048 - 2416, 0)`. AAL source-length reconstruction is a separate contract.

## Native GUI / 原生界面

Windows 解压 GUI 包后运行 `atracodec-converter.exe`，保持同目录的 `atracodec.exe`。macOS 包内的 `ATRACodec Converter.app` 已内置相应 CLI。界面支持中文/English、编码/解码、profile 选择、进度、取消与覆盖选项；使用系统原生控件，未引入 Qt 或 Electron。

On Windows, run `atracodec-converter.exe` with its sibling `atracodec.exe`. The macOS `ATRACodec Converter.app` includes the matching CLI. Native controls provide Chinese/English, encode/decode, profile selection, progress, cancellation, and overwrite controls without Qt or Electron.

自动测试验证过参数构造和真实 CLI 工作进程，但未启动或操作 GUI；参见[手动测试](MANUAL-TESTING.md)。

Automation checked argument construction and real CLI workers, but did not launch or operate the GUI. See [manual testing](MANUAL-TESTING.md).
