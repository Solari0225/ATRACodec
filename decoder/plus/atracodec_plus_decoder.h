#ifndef ATRACODEC_PLUS_DECODER_H
#define ATRACODEC_PLUS_DECODER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Native signed16 interleaved PCM. sample_frames counts samples per channel.
 * The independently recovered standard lossy graph is required; the AALP
 * integer residual predictor is not a substitute for this decoder. */
typedef struct atracodec_plus_pcm {
    int16_t *samples;
    uint64_t sample_frames;
    uint32_t sample_rate,channels,source_format,core_bitrate;
} atracodec_plus_pcm;

/* Decode an unencrypted EA3 codec1 OMA file. 0=success, negative=failure.
 * The output length follows Sony's actual pure stream contract, including
 * its frame priming/synthesis delay; no original-PCM-length guess is applied.
 * No Sony runtime, installed component or foreign decoder is invoked.
 * Always release successful output through the paired free function.
 * This header describes the integration ABI; implementation/acceptance is
 * in progress until the complete standard graph and whole-file tests pass. */
int atracodec_plus_decode_oma(const uint8_t *data,size_t bytes,
    atracodec_plus_pcm *output);
void atracodec_plus_free_pcm(atracodec_plus_pcm *output);
const char *atracodec_plus_last_error(void);

#ifdef __cplusplus
}
#endif
#endif
