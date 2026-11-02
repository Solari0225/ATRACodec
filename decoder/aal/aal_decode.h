#ifndef ATRACODEC_AAL_DECODE_H
#define ATRACODEC_AAL_DECODE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_aal_pcm {
    int16_t *samples; /* malloc-owned interleaved samples; free with this API */
    uint64_t sample_frames;
    uint32_t sample_rate,channels,source_format,core_bitrate;
} atracodec_aal_pcm;
/* Unencrypted Sony AAL3/132 or AALP nine default stereo44100/16 presets.
 * Sony-native default stereo16 grammar:1..4part layouts, Rice0..16/u32fold,
 * fixed temporal0..7, LPC8, raw PCM, correlation0..2. Nondefault mono/24bit
 * configurations are outside this API. Finite native/corpus validation is
 * recorded separately; it is not a proof for every possible input file.
 * AALP framefront accepts native bounded trailing type2 skipped extensions.
 * Success0, failure-1 with cleared output and thread-local diagnostic.
 * No source PCM, official core substitute, file, DLL or process is required. */
int atracodec_aal_decode_oma(const uint8_t *oma,size_t bytes,atracodec_aal_pcm *output);
void atracodec_aal_free_pcm(atracodec_aal_pcm *output);
const char *atracodec_aal_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
