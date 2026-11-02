#ifndef SONY_ORIGINAL_AALP_ASSEMBLY_H
#define SONY_ORIGINAL_AALP_ASSEMBLY_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Codec-stage boundary: the caller supplies independently encoded physical
 * ATRAC3plus frames. This function parses the actual core, computes the exact
 * original fixed predictor, lossless residuals, enhancement and OMA packets.
 * It has no file/process/component/foreign-codec calls. This is not itself a
 * PCM-to-lossy-core encoder. Official core inputs are permitted in tests only.
 */
int sony_original_aalp_assemble(const int16_t *pcm,uint32_t sample_frames,
    const uint8_t *cores,uint32_t core_count,uint32_t core_bytes,
    uint8_t **output,size_t *bytes);
void sony_original_aalp_free(void *output);
const char *sony_original_aalp_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
