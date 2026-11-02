# Implementation provenance / 实现来源

## Independent project / 独立项目

ATRACodec 是独立的新项目。它不是 AtracDEnc、AtracDEnc-AAL 或其他相关项目的分支、改名发行版或运行封装；当前有损及无损编解码实现不以这些项目作为上游编解码实现。主体依据 Sony 官方组件的原理、静态分析与自有合成输入下的有界运行观察独立开发。

ATRACodec is an independent new project. It is not a fork, rebranded distribution, or runtime wrapper of AtracDEnc, AtracDEnc-AAL, or related projects. These are not the upstream codec implementation for the current lossy/lossless encoder and decoder. The implementation was developed primarily from Sony official component principles, static analysis, and bounded observations using owned synthetic inputs.

Sony SMLE / TraConv 只作为研究与验收参考。发布源码不包含官方 DLL、EXE、研究捕获、解编译数据库或研究工具；程序不加载它们，也不需要模拟器、Python 或第三方 ATRAC 运行库。解码器开发未使用 AtracDEnc、FFmpeg 或其他第三方 ATRAC 解码实现。

Sony SMLE / TraConv served as research and validation references. Published source excludes official DLLs/EXEs, captures, decompiler databases, and research tools. Programs do not load them or require an emulator, Python, or a third-party ATRAC runtime. Decoder development did not use AtracDEnc, FFmpeg, or other third-party ATRAC decoder implementations.

参考 TraConv.exe SHA-256 / Reference TraConv.exe SHA-256:

```text
88622173b1a7c91fc38ceedc846a59c8f7593515183550d897811f53c293d993
```

## Retained own work / 保留的自研成果

早期有一份明确采用 AtracDEnc 有损核心的混合备用方案，作为另一个项目单独保留，没有把它包装成此项目。当前源码仍保留其中 **18 份自研 AAL 算术/容器辅助文件**；这些不是当年的 AtracDEnc 有损核心，已核对来源记录，并纳入本次源码指纹比对。因此“独立项目”不能被扩张为“全部代码均从空白重写、没有沿用任何早期自研成果”。

An earlier mixed fallback explicitly used an AtracDEnc lossy core and remains a separate project; it is not republished as ATRACodec. The current tree retains **18 self-developed AAL arithmetic/container helpers** from that experiment. These are distinct from its AtracDEnc lossy core, were checked against their provenance records, and were included in the fingerprint comparison. Independence does not mean every line was rewritten from an empty tree or that no earlier own work was retained.

231 份既有编码构建输入与已验收的独立 0.4 快照一致，其中 230 份也与独立 0.3 快照同路径逐字节一致；另 1 份是后加的浮点环境保护头。0.5 新增流接口复用这些已验收的自有数学模块，不转调 Sony 组件。

All 231 prior encoder build inputs match the accepted independent 0.4 snapshot; 230 also match the independent 0.3 snapshot at the same paths. The other file is the later floating-point environment guard. The 0.5 streaming layer reuses these accepted own math modules rather than delegating to Sony components.

## Recorded source comparison / 已记录的源码比对

核验对象为原始实际交付源码包的 328 个文件，其中 303 个 C/C++、头文件和数表进入指纹比对。参考集合包括固定上游提交、早期备用方案的原始上游压缩包，以及旧 ATRAC3 / ATRAC3plus vendor 树；每份参考有 117 个代码文件。

The audit verified all 328 files of the actual original source package and fingerprinted 303 C/C++, header, and table files. Reference sets were a pinned upstream revision, the original upstream archive used for the fallback, and the old ATRAC3/ATRAC3plus vendor trees, each with 117 code files.

Pinned reference / 固定参考:

[dcherednik/atracdenc @ cb170709f836cb43dc0312455188c791159d81dc](https://github.com/dcherednik/atracdenc/tree/cb170709f836cb43dc0312455188c791159d81dc)

| Check / 检查 | Matches / 命中 |
| --- | --- |
| 完全相同的原始文件 / Identical raw files | 0 |
| 去注释和空白后相同 / Identical after comment/whitespace removal | 0 |
| 至少 80 token 的连续精确片段 / Contiguous exact runs of at least 80 tokens | 0 |

片段比较保留标识符与数值字面量，过滤低信息重复窗口。参考代码仅作为只读审计数据，未编译、执行、链接或据此改写编解码算法。未随本项目分发参考代码。

Token comparisons retained identifiers and numeric literals and filtered low-information repeated windows. Reference code was read-only audit data, not compiled, executed, linked, or used to rewrite codec algorithms. It is not distributed with this project.

**准确结论是“当前快照未发现直接引入 AtracDEnc 源码的证据”。** 检测不能穷尽任意改名、改写、全部历史版本或曾参考的思路；不把有限指纹比对描述为数学证明。

**The precise result is that no direct incorporation of AtracDEnc source was found in this snapshot.** This cannot exhaust arbitrary renaming, rewriting, all historical revisions, or previously consulted ideas. A finite fingerprint audit is not a mathematical proof.

## Name strings / 名称字符串

原始交付源码中有 3 个文件、4 处名称字符串：CLI JSON 的 `atracdenc_used:false`、一处否定性注释，以及 `PROVENANCE.txt` 的中英文来源说明。它们不是编解码选择开关、函数调用或库依赖。Windows CLI 有一处该 JSON ASCII 字段，Mac Universal CLI 的两个切片各有一处。

The original delivery contains four name occurrences in three source files: the CLI JSON field `atracdenc_used:false`, one negative-use comment, and the Chinese/English statements in `PROVENANCE.txt`. They are not codec selectors, calls, or dependencies. The Windows CLI has one ASCII occurrence of the JSON field, and each Universal macOS CLI slice has one.

本次新增 README/核验说明会继续出现被说明的项目名，所以从未宣称“没有 AtracDEnc 字符串”。GUI、共享/静态/导入库及示例在原始包扫描中未检出该名称，公开 API 与系统依赖检查也未显示其运行依赖。

The new READMEs/audit notes necessarily name the compared project; no zero-string claim is made. The original package scan found no such name in GUIs, shared/static/import libraries, or examples. Public API and system dependency checks also showed no AtracDEnc runtime dependency.

## Public evidence / 公开证据

[验证摘要](VALIDATION.md)、[机器可读摘要](verification.json)和[证据文件哈希](evidence-hashes.json)保留测试范围与证据绑定。原始详细记录保存在项目研究归档中；公开摘要不包含本地账号路径、PID、原组件、原始捕获或音频。`source-manifest.json` 保持原始交付记录，新增发布文档不计入原始命中数量。

[Validation](VALIDATION.md), the [machine-readable summary](verification.json), and [evidence hashes](evidence-hashes.json) record scope and bind the underlying reports. Detailed records remain in the research archive. Public summaries omit local account paths, PIDs, original components, raw captures, and audio. `source-manifest.json` preserves the original delivery inventory; publication documents do not change the original occurrence counts.
