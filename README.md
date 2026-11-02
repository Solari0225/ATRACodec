# ATRACodec

[English](README.en.md) · 简体中文 · [下载 / Releases](https://github.com/Solari0225/ATRACodec/releases/tag/v0.5.0-test1)

独立的 ATRAC 有损与无损编解码器，提供命令行、C API，以及 Windows / macOS 原生轻量转换界面。

**ATRACodec 是一个全新、独立维护的项目，与 AtracDEnc 及其相关项目没有项目隶属关系。它不是这些项目的分支、改名发行版或调用封装，编解码实现不以 AtracDEnc 为基础。主要实现依据 Sony 官方组件的原理、静态分析与受控输入下的行为观察独立开发。** 运行时无需安装 Sony SMLE、TraConv 或 AtracDEnc。

ATRACodec is an independent new project, not a fork, rebranded distribution, or runtime wrapper of AtracDEnc or its related projects. Its codec implementation was independently developed primarily from analysis of Sony official components and their observed behavior. See [implementation provenance](docs/PROVENANCE.md) for the recorded source history and audit limits.

本项目不隶属于 Sony，也不是 Sony 官方软件。独立性说明不隐瞒开发历史：18 份自研 AAL 辅助文件沿用自早期实验；当前快照的源码比对未发现直接引入 AtracDEnc 代码的证据。详见[来源与核验范围](docs/PROVENANCE.md)。

## 当前版本

**0.5.0-test1 — 预发布测试版，GPL-3.0-only。**

支持未加密 OMA，输入/还原格式为 **44.1 kHz、16-bit、双声道 PCM**。目标兼容设备以 Sony Walkman NW-X1050 / NW-X1060 为代表；实机播放与手动 GUI 测试待用户完成，不将目标兼容性写成已完成的硬件认证。

| 模式 | CLI profile | 核心标称码率（kbps） | 还原性质 |
| --- | --- | --- | --- |
| ATRAC3 | `atrac3-66`、`atrac3-105`、`atrac3-132` | 66 / 105 / 132 | 有损；66 档为联合立体声 |
| ATRAC3plus | `atrac3plus-<码率>` | 48 / 64 / 96 / 128 / 160 / 192 / 256 / 320 / 352 | 有损 |
| AAL3 | `aal3-132` | ATRAC3 132 核心 | 无损：有损核心 + 无损补偿数据 |
| AALP | `aalp-<码率>` | ATRAC3plus 全部九档核心 | 无损：有损核心 + 无损补偿数据 |

共 **22 档**。AAL 表中的码率仅指有损核心，不是整个无损文件的总码率；总大小取决于音频内容及补偿数据，压缩后也可能比 PCM 更大。44.1 kHz / 16-bit / 双声道 PCM 的原始码率为 1411.2 kbps。

## 下载

从 [v0.5.0-test1 Release](https://github.com/Solari0225/ATRACodec/releases/tag/v0.5.0-test1) 选择：

| 文件 | 内容 |
| --- | --- |
| `ATRACodec-0.5.0-test1-Windows-x64.zip` | Windows x64 原生 GUI + CLI |
| `ATRACodec-0.5.0-test1-macOS-Universal.zip` | macOS Intel / Apple Silicon 通用 GUI + CLI |
| `ATRACodec-0.5.0-test1-CLI-Windows-x64.zip` | Windows x64 纯命令行 |
| `ATRACodec-0.5.0-test1-CLI-macOS-Universal.zip` | macOS 通用纯命令行 |
| `ATRACodec-0.5.0-test1-SDK.zip` | 公共 C 头文件、静态/动态库及 C11 示例 |
| `ATRACodec-0.5.0-test1-Source.zip` | 与仓库一致的源码、双语说明与完整 GPLv3 许可证 |
| `SHA256SUMS.txt` | 本次 Release 全部手动上传附件的 SHA-256 |

程序二进制沿用已验收构建；本次整理增加项目文档与许可证，没有改动编解码程序。macOS 已交叉构建、链接并检查通用二进制切片，**尚未在原生 macOS 执行**，也未进行 Apple 开发者签名或公证。Mac GUI 声明的最低系统版本为 macOS 11。

## 命令行

Windows PowerShell：

```powershell
.\atracodec.exe --list-profiles
.\atracodec.exe encode --profile aalp-352 input.wav output.oma
.\atracodec.exe decode output.oma restored.wav
```

macOS：

```sh
chmod +x ./atracodec
./atracodec --list-profiles
./atracodec encode --profile aalp-352 input.wav output.oma
./atracodec decode output.oma restored.wav
```

有损编码示例：`atracodec encode --profile atrac3plus-352 input.wav output.oma`。支持 RIFF/WAVE 和 RF64/WAVE；默认禁止覆盖，需显式加 `--overwrite`。`--progress` 在 stderr 输出 JSON Lines，stdout 输出最终 JSON，便于接入其他应用。完整说明见 [CLI](docs/CLI.md)。

## 流式处理与接入

CLI 和 GUI 使用增量编解码管线；新 C 流接口以固定有界状态处理分块输入，不缓存整首音频。编码开始前需要已知的 64 位总采样帧数，尚未提供未知时长实时录音接口。旧的整缓冲区 C API 保留兼容，仍会持有完整缓冲区。

没有人为的 1 GiB 文件上限；已实测 **1 GiB + 4 字节 PCM** 的完整 AAL3 编码、Sony 解码和本项目解码一致。格式自身的时长/包字段仍有边界，不能据此声称无限长度。[C API 接入](docs/INTEGRATION.md) · [流式管线说明](docs/PIPELINE.md)

## 一致性与已知范围

验收优先比较同一输入、同一设置下的音频编码数据，其次检查 PCM。既定测试集中，编码音频数据与 Sony 参考输出一致；AAL 解码还原源 PCM，解码测试也与 Sony 输出一致。

- 880 个非空编码用例覆盖 22 档、10 个长度边界、4 种分块方式；另验证 22 档原版空流行为。
- 1046 个 Sony 参考增量解码用例，以及 10 个原组件头字段/末块权威性测试。
- 已验证公开 API、并发独立上下文、回调错误、浮点环境恢复、固定内存、取消和事务输出。
- 公开源码已脱离研究工作目录，在 Windows x64、macOS arm64 / x64 三个目标完成构建；macOS 尚未原生运行，GUI 与 Walkman 实机待手动测试。

**不声称所有输入上的普遍等价，或 Sony 原始整文件完全相同。** OMA 的可变元数据不参与音频数据比较；规范化会省略 Sony ID3 前缀、将 EA3 头 `[12,32)` 归零。WAV 容器布局和外层回调发布粒度也不要求与 Sony 相同。详见[验收记录](docs/VALIDATION.md)及[待测项目](docs/MANUAL-TESTING.md)。

## 构建与源码布局

需要 Python 3.9+、Zig 0.14.1；也提供 CMake 3.20+ 构建方式。

```sh
python build.py --zig /path/to/zig --target all
```

输出在 `build/<target>/`，包括 CLI、库、C11 示例与原生 GUI。构建不启动 GUI。可用 `--no-gui`、`--no-example` 关闭可选目标。详见[构建说明](docs/BUILDING.md)。

| 路径 | 用途 |
| --- | --- |
| `atrac3/`、`shared/` | 独立有损编码与 AAL 数学、码流和容器模块 |
| `decoder/` | Sony 组件分析所依据的独立解码模块 |
| `product/` | C API、流接口、CLI、文件处理、原生 GUI |
| `examples/` | C11 内存、文件及流式回调示例 |
| `docs/` | 中英文构建、接入、来源和验收说明 |
| `sources.json`、`source-manifest.json` | 已验收源码输入及原始源码交付树的哈希清单 |
| `tools/verify_source.py` | 验证原始源码清单和 316 个构建输入 |

仓库不包含 Sony 原始组件、第三方 ATRAC 编解码项目、研究工具、研究捕获或音频样本。

## 许可证

本项目按 **GNU General Public License v3.0 only（GPL-3.0-only）** 发布；完整文本见 [LICENSE](LICENSE)。分发程序和源码时请保留相应许可及源码义务。有关项目名称与实现来源的说明见 [NOTICE.md](NOTICE.md) 和 [PROVENANCE](docs/PROVENANCE.md)。
