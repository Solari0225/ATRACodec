# ATRACodec

English · [简体中文](README.md) · [Releases](https://github.com/Solari0225/ATRACodec/releases/tag/v0.5.0-test1)

An independent ATRAC lossy and lossless encoder/decoder with a CLI, a C API, and lightweight native Windows / macOS converters.

**ATRACodec is an independent new project, with no project affiliation to AtracDEnc or its related projects. It is not their fork, rebranded distribution, or runtime wrapper, and its codec implementation is not based on AtracDEnc. The implementation was independently developed primarily from the principles, static analysis, and controlled runtime observations of Sony official components.** No Sony SMLE, TraConv, or AtracDEnc installation is required at runtime.

This is not official Sony software and is not affiliated with Sony. Independence does not erase development history: 18 self-developed AAL helper files were retained from an earlier experiment. The recorded comparison found no direct incorporation of AtracDEnc source in the delivered snapshot. Read [provenance and audit limits](docs/PROVENANCE.md).

## Current version

**0.5.0-test1 — prerelease test build, GPL-3.0-only.**

Supports unencrypted OMA and **44.1 kHz, 16-bit, stereo PCM**. Sony Walkman NW-X1050 / NW-X1060 are representative target devices. Hardware playback and manual GUI testing remain for the user; target compatibility is not presented as completed hardware certification.

| Mode | CLI profile | Nominal core bitrate (kbps) | Reconstruction |
| --- | --- | --- | --- |
| ATRAC3 | `atrac3-66`, `atrac3-105`, `atrac3-132` | 66 / 105 / 132 | Lossy; 66 uses joint stereo |
| ATRAC3plus | `atrac3plus-<bitrate>` | 48 / 64 / 96 / 128 / 160 / 192 / 256 / 320 / 352 | Lossy |
| AAL3 | `aal3-132` | ATRAC3 132 core | Lossless core + residual |
| AALP | `aalp-<bitrate>` | All nine ATRAC3plus cores | Lossless core + residual |

There are **22 profiles**. The AAL bitrate is the lossy core bitrate, not the total lossless bitrate. Total size depends on the signal and its residual, and may exceed PCM size. Uncompressed 44.1 kHz / 16-bit stereo PCM is 1411.2 kbps.

## Downloads

Choose assets from the [v0.5.0-test1 release](https://github.com/Solari0225/ATRACodec/releases/tag/v0.5.0-test1):

| File | Contents |
| --- | --- |
| `ATRACodec-0.5.0-test1-Windows-x64.zip` | Native Windows x64 GUI + CLI |
| `ATRACodec-0.5.0-test1-macOS-Universal.zip` | Universal Intel / Apple Silicon GUI + CLI |
| `ATRACodec-0.5.0-test1-CLI-Windows-x64.zip` | Windows x64 CLI only |
| `ATRACodec-0.5.0-test1-CLI-macOS-Universal.zip` | Universal macOS CLI only |
| `ATRACodec-0.5.0-test1-SDK.zip` | Public C headers, static/shared libraries, C11 examples |
| `ATRACodec-0.5.0-test1-Source.zip` | Repository sources, bilingual documentation, full GPLv3 license |
| `SHA256SUMS.txt` | SHA-256 for all manually uploaded release assets |

Executable bytes are retained from the accepted build. This publication adds documentation and licensing without modifying the codec programs. macOS was cross-built, linked, and checked for both architecture slices, but **has not been run on native macOS**. No Apple Developer signing or notarization was performed. The GUI declares macOS 11 as its minimum version.

## Command line

Windows PowerShell:

```powershell
.\atracodec.exe --list-profiles
.\atracodec.exe encode --profile aalp-352 input.wav output.oma
.\atracodec.exe decode output.oma restored.wav
```

macOS:

```sh
chmod +x ./atracodec
./atracodec --list-profiles
./atracodec encode --profile aalp-352 input.wav output.oma
./atracodec decode output.oma restored.wav
```

For lossy encoding, use `atracodec encode --profile atrac3plus-352 input.wav output.oma`. RIFF/WAVE and RF64/WAVE are supported. Overwriting is disabled unless `--overwrite` is provided. `--progress` emits JSON Lines to stderr; the final JSON result goes to stdout for application integration. See [CLI documentation](docs/CLI.md).

## Streaming and integration

The CLI and GUI use incremental processing. The new C stream API operates with bounded state rather than holding a complete track. Encoding requires a known 64-bit total sample-frame count before starting; unknown-duration live capture is not exposed. The legacy whole-buffer C API remains available and still holds complete buffers.

There is no arbitrary 1 GiB file limit. A complete **1 GiB + 4 byte PCM** AAL3 encode and both Sony/project decodes were verified. Format duration/packet fields still impose real limits; this is not an unlimited-length claim. [C API integration](docs/INTEGRATION.md) · [Pipeline contract](docs/PIPELINE.md)

## Conformance and limits

Validation prioritizes identical encoded audio data for the same input/settings, then PCM reconstruction. Encoded audio data matched the Sony-accepted reference within the declared corpus; AAL restored source PCM, and decoder cases matched Sony PCM output.

- 880 nonempty encoder cases: 22 profiles × 10 length boundaries × 4 chunk patterns; 22 original empty-stream cases were also checked.
- 1046 Sony-reference incremental decoder cases and 10 original-component header/terminal-block authority tests.
- Public API protocol, concurrent independent contexts, callback errors, floating-point environment restoration, bounded memory, cancellation, and transactional file output were checked.
- Clean source builds completed for Windows x64 and macOS arm64 / x64. Native macOS, GUI operation, and Walkman hardware playback remain manual tests.

**This is not a claim of universal equivalence or raw whole-file identity with Sony output.** Variable metadata is excluded from audio comparisons: normalization omits Sony's ID3 prefix and zeros EA3 header bytes `[12,32)`. WAV container layout and outer callback publication granularity may also differ. See [validation](docs/VALIDATION.md) and [manual tests](docs/MANUAL-TESTING.md).

## Building and source layout

Use Python 3.9+ and Zig 0.14.1, or the provided CMake 3.20+ integration.

```sh
python build.py --zig /path/to/zig --target all
```

Outputs in `build/<target>/` include the CLI, libraries, C11 examples, and native GUI. Building never launches the GUI. Use `--no-gui` or `--no-example` to omit optional targets. See [building](docs/BUILDING.md).

| Path | Purpose |
| --- | --- |
| `atrac3/`, `shared/` | Independent lossy encoding, AAL math, bitstreams, containers |
| `decoder/` | Independent decoder modules derived from Sony component analysis |
| `product/` | C API, streaming, CLI, file handling, native GUIs |
| `examples/` | C11 memory, file, and streaming callback examples |
| `docs/` | Bilingual build, integration, provenance, and validation notes |
| `sources.json`, `source-manifest.json` | Accepted build-input hashes and original source-delivery inventory |
| `tools/verify_source.py` | Verify the original source inventory and 316 build inputs |

The repository excludes original Sony components, third-party ATRAC codec projects, research tools, runtime captures, and audio fixtures.

## License

Released under the **GNU General Public License v3.0 only (GPL-3.0-only)**. See the full [LICENSE](LICENSE). Preserve the applicable license and corresponding-source obligations when distributing the software. Project naming and provenance are described in [NOTICE.md](NOTICE.md) and [PROVENANCE](docs/PROVENANCE.md).
