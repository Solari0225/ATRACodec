#ifndef SONY_ORIGINAL_AAL_RESEARCH_H
#define SONY_ORIGINAL_AAL_RESEARCH_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Research AAL3/132 encoder: PCM signed16 stereo44100Hz -> complete
 * unencrypted OMA/AAL3 bytes. Original independently reconstructed lossy core,
 * integer predictor and AAL enhancement method. No installed codec/process.
 * Nonempty input; frame_count counts L/R pairs. On success returns0 and owns
 * output until sony_original_aal_free. Negative error leaves output null.
 * Research interface; complete all-profile release acceptance remains separate.
 */
int sony_original_aal3_encode(const int16_t *pcm,uint32_t frame_count,
    uint8_t **output,size_t *output_bytes);
void sony_original_aal_free(void *output);
const char *sony_original_aal_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
