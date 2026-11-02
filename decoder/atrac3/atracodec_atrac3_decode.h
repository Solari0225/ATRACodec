#ifndef ATRACODEC_ATRAC3_DECODE_H
#define ATRACODEC_ATRAC3_DECODE_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct atracodec_atrac3_pcm {
    int16_t *samples;
    uint64_t sample_frames;
    uint32_t sample_rate,channels,source_format,core_bitrate;
} atracodec_atrac3_pcm;

/* Sony-derived ordinary ATRAC3/66,105,132 lossy decoding,44.1kHz stereo.
 * Output count and rounding follow the observed Sony ordinary lossy path,
 * independently of AAL predictor timing. See the separate research acceptance.
 * Successful output is owned by this module's matching free. */
int atracodec_atrac3_decode_oma(const uint8_t *data,size_t bytes,
    atracodec_atrac3_pcm *output);
void atracodec_atrac3_free_pcm(atracodec_atrac3_pcm *output);
const char *atracodec_atrac3_last_error(void);

#ifdef __cplusplus
}
#endif
#endif
