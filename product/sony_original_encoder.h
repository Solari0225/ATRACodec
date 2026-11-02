#ifndef SONY_ORIGINAL_ENCODER_H
#define SONY_ORIGINAL_ENCODER_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum sony_original_format {
    SONY_ORIGINAL_ATRAC3=1,SONY_ORIGINAL_ATRAC3PLUS=2,
    SONY_ORIGINAL_AAL3=3,SONY_ORIGINAL_AALP=4
};
typedef struct sony_original_profile {
    uint32_t format,nominal_bitrate,core_frame_bytes,core_frame_samples,lossless;
    char id[16];
} sony_original_profile;
const char *sony_original_encoder_version(void);
uint32_t sony_original_encoder_profile_count(void);
int sony_original_encoder_profile_at(uint32_t index,sony_original_profile *profile);
/* Nonempty interleaved signed16 stereo PCM,44100Hz. sample_frames counts
 * L/R pairs, not individual int16 values. No files, external processes or
 * installed codec components are accessed by this API. Unknown format/rate
 * combinations fail; no substitute codec is selected. Output is a complete
 * unencrypted OMA file and belongs to the caller until encoder_free().
 * On failure output is null, bytes is0, and last_error describes the failure.
 * Separate calls own separate encoder history and may run concurrently.
 * Encoding temporarily uses the default IEEE floating-point environment and
 * restores the caller's environment before returning.
 * Supported input format and finite verification scope are documented in the
 * accompanying test-release report.
 */
int sony_original_encoder_encode_pcm16(const int16_t *pcm,uint32_t sample_frames,
    uint32_t format,uint32_t nominal_bitrate,uint8_t **output,size_t *bytes);
void sony_original_encoder_free(void *output);
/* Thread-local UTF8 message, valid until the next encoding call on that thread. */
const char *sony_original_encoder_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
