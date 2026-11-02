#ifndef ATRACODEC_PUBLIC_H
#define ATRACODEC_PUBLIC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum atracodec_format {
    ATRACODEC_ATRAC3=1,ATRACODEC_ATRAC3PLUS=2,
    ATRACODEC_AAL3=3,ATRACODEC_AALP=4
};
typedef struct atracodec_profile {
    uint32_t format,nominal_bitrate,core_frame_bytes,core_frame_samples,lossless;
    char id[16];
} atracodec_profile;
typedef struct atracodec_audio_info {
    uint32_t sample_rate,channels,sample_bits,source_format,core_bitrate,reserved;
    uint64_t sample_frames;
} atracodec_audio_info;
const char *atracodec_version(void);
uint32_t atracodec_profile_count(void);
int atracodec_profile_at(uint32_t index,atracodec_profile *profile);
/* PCM is interleaved signed16 stereo44100Hz. sample_frames counts L/R pairs.
 * Successful output is a complete unencrypted OMA, owned until atracodec_free.
 * Error: -1 with output=null,bytes=0 and a thread-local UTF8 diagnostic. */
int atracodec_encode_pcm16(const int16_t *pcm,uint32_t sample_frames,
    uint32_t format,uint32_t nominal_bitrate,uint8_t **output,size_t *bytes);
/* Development interface. Complete format coverage is reported separately.
 * Input is an unencrypted OMA. Successful output is interleaved signed16 PCM;
 * info.sample_frames is the actual output length, including the original
 * lossy decoder's documented timing policy. AAL restores source length.
 * Caller frees samples with atracodec_free. The encoder/decoder never reads
 * external files, calls Sony components or invokes a substitute decoder. */
int atracodec_decode_pcm16(const uint8_t *input,size_t bytes,int16_t **samples,
    atracodec_audio_info *info);
void atracodec_free(void *data);
const char *atracodec_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
