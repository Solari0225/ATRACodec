# C API integration / C API 接入

公共头文件为 [atracodec.h](../product/atracodec.h) 和 [atracodec_stream.h](../product/atracodec_stream.h)，共 17 个导出符号。完整约定见 [API.txt](../API.txt)。所有 PCM 都是交错 signed 16-bit、双声道、44100 Hz；一帧表示一对 L/R 样本，不是一个字节或一个声道样本。

The public headers are [atracodec.h](../product/atracodec.h) and [atracodec_stream.h](../product/atracodec_stream.h), exposing 17 symbols. See [API.txt](../API.txt) for the complete contract. PCM is interleaved signed 16-bit stereo at 44100 Hz; one frame is an L/R pair.

## Incremental API / 增量接口

编码：`atracodec_encoder_create` → 多次 `atracodec_encoder_write` → `atracodec_encoder_finish` → `atracodec_encoder_destroy`。创建时指定格式、核心码率和 `uint64_t expected_frames`，必须准确提交这些帧数。编码字节通过回调输出，不保留完整文件。

Encode: `atracodec_encoder_create` → repeated `atracodec_encoder_write` → `atracodec_encoder_finish` → `atracodec_encoder_destroy`. Creation takes the format, core bitrate, and `uint64_t expected_frames`; feed exactly this count. Encoded bytes are delivered through a callback without retaining the complete file.

解码：`atracodec_decoder_create` → 多次 `atracodec_decoder_write` → `atracodec_decoder_finish` → `atracodec_decoder_destroy`。输入可任意字节分块，PCM 通过回调输出；`atracodec_decoder_info` 可查询已识别的格式及累计输出帧数。

Decode: `atracodec_decoder_create` → repeated `atracodec_decoder_write` → `atracodec_decoder_finish` → `atracodec_decoder_destroy`. Input accepts arbitrary byte chunks; PCM is delivered through a callback. `atracodec_decoder_info` reports the recognized format and frames emitted so far.

- 回调数据只在回调期间有效，必须消费整个数据块；返回 `0` 表示成功，非零使该流永久失败。
- 同一上下文不支持并行或回调重入，也不能在其活动回调中销毁自己；独立上下文可并行，也可由一个上下文回调驱动另一个独立上下文。
- 成功的变更操作返回 `0`，错误返回 `-1`；`decoder_info` 在完整 EA3 头到达前返回 `1`。错误文字从 `atracodec_stream_last_error()` 获取，为线程私有 UTF-8。
- 成功流需恰好执行一次 `finish`；`destroy` 不负责 flush。错误后仅销毁该上下文，不继续使用。
- 解码可能在输出部分 PCM 后才发现尾部错误；文件应用应采用临时文件，成功后提交。

- Callback data is valid only during the callback. Consume the entire block and return `0`; nonzero permanently fails the stream.
- A context cannot be used concurrently, reentered, or destroyed from its active callback. Independent contexts may run concurrently, or one callback may drive a separate context.
- Successful mutations return `0`, errors `-1`; `decoder_info` returns `1` before a complete EA3 header. Obtain thread-local UTF-8 diagnostics from `atracodec_stream_last_error()`.
- Finish a successful stream exactly once. `destroy` does not flush. Destroy after an error rather than continuing.
- A decoder may report a late error after emitting PCM; stage file output and commit only on success.

## Examples / 示例

| Source | Demonstrates / 用途 |
| --- | --- |
| [stream_roundtrip.c](../examples/stream_roundtrip.c) | 编码回调驱动独立解码器，用固定小块检查无损还原 / Bounded streaming encoder-to-decoder loopback |
| [encode_pcm16.c](../examples/encode_pcm16.c) | PCM 文件编码接口 / PCM file encoding |
| [roundtrip_memory.c](../examples/roundtrip_memory.c) | 旧整缓冲区 API 无损往返 / Legacy whole-buffer lossless roundtrip |

SDK 提供 Windows x64 与 macOS Universal 静态/动态库。Windows 动态链接使用 `atracodec-import.lib`，静态链接使用 `atracodec-static.lib`；动态库示例需与 `atracodec.dll` 同目录。macOS 提供 `libatracodec.a` 与 `libatracodec.dylib`。

The SDK provides Windows x64 and Universal macOS static/shared libraries. Use `atracodec-import.lib` for Windows DLL linkage or `atracodec-static.lib` for static linkage; DLL examples need the matching `atracodec.dll` beside them. macOS provides `libatracodec.a` and `libatracodec.dylib`.

旧的 `atracodec_encode_pcm16` / `atracodec_decode_pcm16` 仍缓冲完整输入输出；旧编码参数为 `uint32_t` 帧数，不代表新流接口的长度限制。旧接口分配的结果使用 `atracodec_free` 释放。

Legacy `atracodec_encode_pcm16` / `atracodec_decode_pcm16` still buffer whole inputs/outputs. The legacy encoder's `uint32_t` frame parameter is not the new stream API limit. Release legacy allocated results with `atracodec_free`.

对外分发集成产物时，项目采用 GPL-3.0-only；本仓库没有额外的链接例外或替代商业许可证。

The project is GPL-3.0-only for distribution, without an additional linking exception or alternative commercial license in this repository.
