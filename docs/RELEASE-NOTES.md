# ATRACodec 0.5.0-test1

## 中文

ATRACodec 独立项目的首个 GitHub 预发布快照，采用 GPL-3.0-only。

ATRACodec 与 AtracDEnc 及其相关项目没有项目隶属关系，不是其分支、改名版本或调用封装；编解码主体依据 Sony 官方组件的原理与受控行为观察独立开发。无需安装 Sony 或 AtracDEnc 组件。早期自研辅助模块及有限源码比对的完整说明见仓库来源文档。

- 支持 22 档：ATRAC3 66/105/132、ATRAC3plus 九档、AAL3/132、AALP 九档；均为 44.1 kHz / 16-bit / 双声道、未加密 OMA。
- Windows x64 与 macOS Universal 的 CLI、原生中英文简易 GUI、C API SDK、完整对应源码及双语说明。
- CLI/GUI 使用有界增量处理；新 C API 使用 64 位已知总帧数。旧整缓冲区 API 仍保留完整缓冲区。
- 已记录的编码音频数据、AAL PCM 还原及 Sony 参考解码验收通过；完整 1 GiB + 4 字节 PCM 的 AAL3 路径一致。
- 发布整理补充 GPLv3、README 和验收说明；包内程序/库字节保持已验收构建不变。全部新附件以 `SHA256SUMS.txt` 为准。

本版本是测试版：macOS 仅交叉构建和链接，未原生执行；GUI 人工操作及 NW-X1050/X1060 实机播放待用户测试。不宣称全部输入上的普遍等价，也不宣称未规范化元数据的原始整文件与 Sony 一致。AAL profile 的码率指有损核心，实际总码率还包含补偿数据。

## English

The first GitHub prerelease snapshot of the independent ATRACodec project, under GPL-3.0-only.

ATRACodec has no project affiliation to AtracDEnc or its related projects and is not their fork, rebrand, or runtime wrapper. Its codec implementation was independently developed primarily from Sony component principles and controlled behavior observations. No Sony or AtracDEnc installation is required. Repository provenance notes explain retained own helper modules and finite audit limits.

- 22 profiles: ATRAC3 66/105/132, nine ATRAC3plus modes, AAL3/132, and nine AALP modes; 44.1 kHz / 16-bit / stereo, unencrypted OMA.
- Windows x64 and Universal macOS CLI, lightweight native Chinese/English GUI, C API SDK, complete corresponding source, and bilingual documentation.
- Bounded incremental CLI/GUI processing and a known-length 64-bit C stream API. Legacy whole-buffer APIs still retain full buffers.
- Recorded encoded-audio, AAL PCM, and Sony-reference decoding checks passed, including a complete AAL3 path with 1 GiB + 4 bytes of PCM.
- Publication adds GPLv3, READMEs, and verification documentation while preserving accepted program/library bytes. Use the new `SHA256SUMS.txt` for these assets.

This is a test release. macOS was cross-built and linked, not natively executed. Manual GUI and NW-X1050/X1060 playback tests remain. No universal equivalence or raw whole-file identity before metadata normalization is claimed. AAL profile bitrates describe the lossy core, not the total bitrate including residual data.

## Downloads / 下载选择

| Asset / 附件 | Purpose / 用途 |
| --- | --- |
| `Windows-x64.zip` / `macOS-Universal.zip` suffix | GUI + CLI |
| `CLI-Windows-x64.zip` / `CLI-macOS-Universal.zip` suffix | CLI only / 纯命令行 |
| `SDK.zip` suffix | C headers, libraries, examples / C 接口、库与示例 |
| `Source.zip` suffix | Complete corresponding sources + docs + license / 完整对应源码、文档及许可证 |
| `SHA256SUMS.txt` | Asset checksums / 附件校验 |

所有资产采用 `ATRACodec-0.5.0-test1-` 文件名前缀（校验、单独说明与许可证除外）。

Package names use the `ATRACodec-0.5.0-test1-` prefix, except checksums, standalone documentation, and license notices.
