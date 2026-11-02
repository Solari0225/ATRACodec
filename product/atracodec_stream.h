#ifndef ATRACODEC_PUBLIC_STREAM_H
#define ATRACODEC_PUBLIC_STREAM_H
#include "atracodec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_stream_encoder atracodec_stream_encoder;
typedef struct atracodec_stream_decoder atracodec_stream_decoder;
/* A callback consumes the entire supplied block before returning0. A nonzero
 * result fails the stream. Data remains valid only during that callback.
 * No callbacks are made concurrently by one context. Calls that mutate a
 * context must not be reentered or run concurrently on that same context. */
typedef int(*atracodec_bytes_sink)(void *user,const uint8_t *data,size_t bytes);
typedef int(*atracodec_pcm_sink)(void *user,const int16_t *pcm,size_t sample_frames);
/* Known source length permits sequential OMA output with a final header at
 * creation. Length is64-bit stereo sample-pair count, not bytes. No complete
 * source or result buffer is retained. Exactly expected_frames must be fed.
 * This API does not yet accept an unknown source length or metadata edits. */
int atracodec_encoder_create(uint32_t format,uint32_t bitrate,
    uint64_t expected_frames,atracodec_bytes_sink,void *user,
    atracodec_stream_encoder **output);
int atracodec_encoder_write(atracodec_stream_encoder*,const int16_t*,size_t frames);
int atracodec_encoder_finish(atracodec_stream_encoder*);
void atracodec_encoder_destroy(atracodec_stream_encoder*);
/* Arbitrary input chunk boundaries are accepted, including byte-at-a-time.
 * Header metadata is skipped incrementally. Decoder history, one physical
 * packet and a bounded AAL tail are retained. A late error can occur after
 * PCM has been emitted; file callers should commit output only on success. */
int atracodec_decoder_create(atracodec_pcm_sink,void *user,
    atracodec_stream_decoder **output);
int atracodec_decoder_write(atracodec_stream_decoder*,const uint8_t*,size_t bytes);
int atracodec_decoder_finish(atracodec_stream_decoder*);
/* Returns0 when an EA3 header is available,1 when not yet available,-1 for
 * an invalid argument. sample_frames reports PCM emitted so far. */
int atracodec_decoder_info(const atracodec_stream_decoder*,atracodec_audio_info*);
void atracodec_decoder_destroy(atracodec_stream_decoder*);
/* Error text is thread-local and distinct from the legacy whole-buffer API.
 * Every state-mutating successful stream call clears the current thread's
 * diagnostic. After an error a context is permanently failed; destroy it.
 * finish is required exactly once. Destroy acceptsNULL and does not flush. */
const char *atracodec_stream_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
