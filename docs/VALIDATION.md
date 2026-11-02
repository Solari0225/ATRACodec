# Validation / 验收

本记录描述 0.5.0-test1 的有限测试结果，不把测试通过写成全部可能输入上的证明。原始研究报告由 [evidence-hashes.json](evidence-hashes.json) 绑定；公开机器可读摘要见 [verification.json](verification.json)。本次 GitHub 整理只改文档与分发结构，没有改动已验收的编解码输入或程序字节。

This records finite results for 0.5.0-test1, not proof for every possible input. [evidence-hashes.json](evidence-hashes.json) binds the underlying research reports; [verification.json](verification.json) provides a public summary. GitHub preparation changes documentation/distribution structure without changing accepted codec inputs or program bytes.

## Acceptance contract / 验收约定

同一输入及设置优先比较编码音频数据；AAL 需要逐样本还原源 PCM，解码器另与 Sony PCM 输出比较。逐字节编码结论仅适用于记录的测试集及其参考绑定。

For identical inputs/settings, encoded audio data is compared first. AAL must restore source PCM sample-for-sample; decoder output is also compared with Sony PCM. Byte-identical encoding conclusions are scoped to the recorded corpus and reference bindings.

不将原始整个 OMA/WAV 文件都视为必须相同：规范化省略 Sony ID3 前缀，将 EA3 头 `[12,32)` 的可变部分归零；音频数据单独比较。WAV 容器预留 JUNK/ds64 区块，布局可能不同。

Raw whole OMA/WAV identity is not required. Normalization omits Sony's ID3 prefix and zeros variable EA3 bytes `[12,32)`; audio data is checked separately. WAV uses reserved JUNK/ds64 space and may have a different container layout.

## Completed checks / 已完成检查

| Area / 范围 | Recorded result / 结果 |
| --- | --- |
| Nonempty encoding / 非空编码 | 880 cases: 22 profiles × 10 boundaries × 4 chunk patterns; audio bytes matched the Sony-accepted independent baseline |
| Empty official streams / 官方空流 | All 22 profiles, including AAL tail/duration wrap; no unintended DSP/extra finish output |
| Incremental decoding / 增量解码 | 1046 same-input Sony-reference cases; all 22 profiles also tested byte-at-a-time |
| Original field authority / 原组件字段权威性 | 10 controlled original decodes for EA3 hints vs physical BLK tail |
| API protocol / API 协议 | 97 protocol checks |
| Floating environment / 浮点环境 | 176 positive checks, 8 error groups, 48 failing-callback checks |
| Concurrency / 并发 | 88 independent-context jobs over 8 workers; thread-local diagnostics checked |
| Callback reentry / 回调重入 | 6 same-context reentry/destruction fail-fast cases |
| Encoder heap / 编码堆 | 44 live runs; maximum tracked C++ heap 515,954 bytes |
| Decoder heap / 解码堆 | 44 runs, 32 vs 1024 packets for 22 profiles; peak growth 0; maximum 70,016 bytes |
| CLI | 22 encodes + 22 decodes, Unicode paths, RF64, cancellation, no-overwrite commit race |
| GUI helper/worker / GUI 辅助和工作进程 | 353 helper checks + 45 real CLI worker cases; 10 AAL roundtrips of 8193 frames |
| Clean source / 干净源码 | All 3 targets built with GUI; static archives reproduced byte-for-byte |
| Universal/C ABI | 7 universal binaries checked; 4 C11 static/shared architecture cross-links |
| CMake | Configure/build/install for all 3 targets; refreshed CLI propagated into GUI bundle |

堆统计只计跟踪到的 C++ 堆，不包含固定栈和 CRT。它说明所测轨道增长不导致堆增长，不是整个进程所有内存的上限。

Heap statistics cover tracked C++ allocations, excluding fixed stack and CRT. They demonstrate no track-length-driven heap growth in those cases, not an upper bound on every process allocation.

## Large input / 大文件

完整输入为 1,073,741,828 字节 PCM（268,435,457 个双声道采样帧），AAL3/132。源 PCM、Sony 解码与本项目解码 SHA-256 均为：

The complete AAL3/132 test used 1,073,741,828 PCM bytes (268,435,457 stereo frames). Source, Sony decode, and project decode shared SHA-256:

```text
fde2dd810211ff5ab289dc82b9f146d32d5ee9d81459bc500555b9c81fafefff
```

完整编码音频数据 SHA-256 / Complete encoded audio payload SHA-256:

```text
253cc5107f1fbea6240ba84b1e6b13c9029ff7285585e31a3138b38f2cd243c7
```

规范化 EA3 + 音频数据 SHA-256 / Normalized EA3 + payload SHA-256:

```text
5c91ec416b4e32adc5991b464abfa0d6868fe51ef22ad2ccd3caff8152e48a9a
```

本机单次项目编码约 83 秒、解码约 19.5 秒，峰值 private bytes 约 1.60 / 1.43 MB、工作集约 7 MiB。这是特定合成输入与机器上的观察，不是性能保证。另有 4,294,967,444 字节稀疏 RF64 文件测试，覆盖大偏移读取，没有完整转码 4 GiB 音频。

One local run took about 83 seconds to encode and 19.5 seconds to decode; peak private bytes were approximately 1.60 / 1.43 MB, with a working set around 7 MiB. These are observations on a specific synthetic input/machine, not performance guarantees. A separate 4,294,967,444-byte sparse RF64 case checked large-offset reads without converting 4 GiB of audio.

## Remaining scope / 尚未覆盖

- 原生 macOS 运行、GUI 的人工点击操作及 NW-X1050/X1060 实机播放。
- 除 PCM16 / 44.1 kHz / 双声道之外的输入、加密 OMA、未知总时长编码。
- 所有 Sony 外层 filter 的精确回调时刻；本项目使用有界末尾适配。
- 任意损坏/恶意文件的全面健壮性或独立安全审计。
- 全部最终链接程序的逐字节可重复构建，以及测试集之外的普遍编码等价。

- Native macOS execution, manual GUI operation, and NW-X1050/X1060 hardware playback.
- Input beyond PCM16 / 44.1 kHz / stereo, encrypted OMA, and unknown-length encoding.
- Exact callback timing for every Sony outer filter; this project uses bounded tail adaptation.
- Exhaustive corrupt/hostile-input robustness or an independent security audit.
- Byte-reproducibility of every linked executable and universal encoding equivalence outside the corpus.
