# Streaming pipeline / 流式管线

## Sony observations / Sony 观察结果

本轮管线研究只使用已安装的 Sony/TraConv 原组件与自有合成 PCM。64 次未注入的原生控制运行全部成功，产物最终重新读取并校验哈希。以下结论区分已观察事实和本项目的适配接口，不把外层系统写入大小等同于编解码回调大小。

The pipeline study used installed Sony/TraConv artifacts and owned synthetic PCM. All 64 unhooked native controls succeeded; output bytes were reread and hashed at finalization. Observed facts are distinguished from the project's adapter contract. OS write sizes are not treated as codec callback sizes.

Sony 编码流程会创建临时文件、写入元数据和初始 EA3 头，以有限请求读取 PCM 并批量写入压缩数据，最后回写头字段、验证并提交。一次已跟踪的 AAL 解码在完整输入尚未读完时就开始写 PCM，结束时回写 WAV 长度，再移动临时文件到目标。这些记录支持已测试路径的增量处理结论。

The observed Sony encoder creates a temporary file, writes metadata and a provisional EA3 header, reads PCM through bounded requests, writes compressed batches, then rewrites header fields, validates, and commits. One tracked AAL decode began writing PCM before consuming the complete input, rewrote WAV lengths at EOF, and moved the temporary file to its destination. These observations establish incremental processing for the tested paths.

该次解码第一次 PCM 文件写入前，最大输入读取偏移为 248,928 / 1,036,888 字节。记录中的 4096 字节 PCM 写入等于 1024 个立体声采样帧，而 AALP 核心有 2048 帧，不能用系统写入粒度推断内部核心回调发布时刻。

Before the first PCM file write in that decode, the highest input offset read was 248,928 of 1,036,888 bytes. Observed 4096-byte PCM writes represent 1024 stereo frames, while an AALP core contains 2048. OS write granularity therefore does not identify the internal core callback publication point.

## Length authority / 长度权威字段

10 次受控原组件解码保留相同的官方核心和补偿数据，仅改变指定字段：EA3 tail/count 的改动不改变输出；最后 BLK 的 tail 从 1 改为 2 则让输出由 4097 帧变为 4098 帧。故本项目按物理 BLK 顺序、末标记和末 tail 还原 AAL 长度，不能仅靠 EA3 count=0 判断空流。

Ten controlled original-component decodes preserved the same official cores/residuals while changing selected fields. EA3 tail/count mutations did not change PCM; changing the last BLK tail from 1 to 2 changed 4097 frames to 4098. ATRACodec therefore follows physical BLK order, termination, and final tail rather than treating EA3 count=0 as authoritative emptiness.

空输入的 22 档官方编码均可完成。空 AAL 会包含特殊的全帧 tail 和负单块时长的无符号回绕值；这些数值不用于分配整段音频内存。本项目保留已测编码行为。

All 22 official profiles accepted empty input. Empty AAL uses a full-frame tail and an unsigned wrap of a negative single-block duration. These values are not used to allocate a complete audio buffer. The observed encoding behavior is retained.

## ATRACodec adapter / 本项目适配

| Stage / 阶段 | Contract / 约定 |
| --- | --- |
| Encoder / 编码 | 开始时提供已知 64 位总帧数，顺序输出最终头和音频块 / Known 64-bit length permits sequential final header and packets |
| Decoder / 解码 | 任意字节分块，增量跳过元数据，保留一个物理包和有界 AAL 末尾 / Arbitrary chunks, incremental metadata skipping, one packet and bounded AAL tail |
| CLI file I/O / 文件处理 | 64 位偏移，有限输入块，同目录临时输出，成功提交 / 64-bit offsets, bounded chunks, sibling temporary output, commit on success |
| Legacy API / 旧接口 | 完整缓冲区接口仍保留整段数据 / Whole-buffer APIs still retain complete data |

本项目 AAL 末尾会延迟少量有限块的 PCM 发布，以便正确裁切。它复现已验收的码流和 PCM 行为，但**未证明每个外层 Sony filter 的回调分块、tick 或发布延迟完全相同**。未知时长编码、流式元数据编辑、加密 OMA 不在当前接口范围。

A bounded number of AAL tail blocks delay PCM publication to permit correct trimming. The implementation reproduces validated bitstream/PCM behavior, but **does not establish identical callback chunks, ticks, or publication latency for every outer Sony filter**. Unknown-duration encoding, streaming metadata edits, and encrypted OMA are outside this API.

没有人为的 1 GiB 限制。已测 1 GiB + 4 字节 PCM 的完整 AAL3 路径；另测大于 4 GiB 的真实稀疏 RF64 文件头及偏移读取，后者不是完整 4 GiB 音频转码测试。AAL 格式字段溢出仍会拒绝。

There is no arbitrary 1 GiB cap. A full 1 GiB + 4 byte PCM AAL3 path was tested. A separate real sparse RF64 file above 4 GiB checked headers and offsets; it was not a complete 4 GiB audio conversion. Actual AAL field overflow is still rejected.
