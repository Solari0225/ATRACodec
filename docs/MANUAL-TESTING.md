# Manual test checklist / 手动测试清单

自动测试未启动界面；此表用于用户自行操作，不标记为已完成。

Automation did not launch the GUI. This checklist is for user testing and is not marked complete.

| Test / 项目 | Expected / 预期 |
| --- | --- |
| Windows GUI + sibling CLI | 中/English 切换、profile、文件选择可用 / Language switch, profiles, file selection work |
| Native Intel / Apple Silicon Mac | `.app` 与独立 CLI 可运行，报告系统版本/架构 / Run app and CLI, record OS and CPU |
| Lossless roundtrip / 无损往返 | `aal3-132` / `aalp-352` 的解码 PCM 与源 PCM 一致 / Decoded PCM equals source |
| Lossy roundtrip / 有损往返 | 各核心可编码、解码和播放；长度遵守文档 / Encode/decode/play with documented timing |
| Progress/cancel / 进度取消 | 不冻结界面，取消不替换原目标 / Responsive UI; cancelled output preserves destination |
| Unicode/spaces / 路径 | 中文、空格路径及覆盖选项正常 / Unicode/spaces and overwrite behavior work |
| NW-X1050 / NW-X1060 | 使用现有正常传输流程测试 OMA 播放并记录固件/模式 / Transfer by the normal device workflow, test playback, record firmware/profile |

反馈至少包括版本、系统/架构、profile、CLI 命令或操作步骤、输入 PCM 规格、错误文字，以及能否用自有合成输入复现。比较无损还原应提取并比较 PCM，不直接比较 WAV 头。

Reports should include version, OS/CPU, profile, command or GUI steps, PCM configuration, exact error, and whether an owned synthetic input reproduces it. Compare PCM for lossless verification rather than whole WAV headers.
