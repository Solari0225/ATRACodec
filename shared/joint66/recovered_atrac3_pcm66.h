#ifndef SONY_RECOVERED_ATRAC3_PCM66_H
#define SONY_RECOVERED_ATRAC3_PCM66_H
#include "recovered_atrac3_joint.h"
#include "../pcm132/recovered_atrac3_pcm132.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_pcm66_state {
    sony_atrac3_pcm132_state dsp;
    sony_atrac3_joint_state joint;
} sony_atrac3_pcm66_state;
typedef struct sony_atrac3_pcm66_stream {
    sony_atrac3_pcm66_state state;
    uint32_t primed,drain_step;
} sony_atrac3_pcm66_stream;
void sony_recovered_atrac3_pcm66_init(sony_atrac3_pcm66_state *state);
/* Each full1024-sample stereo signed16 block emits one192-byte raw DSP
 * callback, including the first priming callback. output requires195 bytes
 * for original writer word-store padding. Finite standard44100Hz profile. */
int sony_recovered_atrac3_pcm66_block(sony_atrac3_pcm66_state *state,
    const int16_t pcm[2048],uint8_t output[195],uint32_t end_bits[2]);
void sony_recovered_atrac3_pcm66_stream_init(sony_atrac3_pcm66_stream *stream);
int sony_recovered_atrac3_pcm66_stream_block(sony_atrac3_pcm66_stream *stream,
    const int16_t pcm[2048],uint8_t output[195],uint32_t end_bits[2]);
int sony_recovered_atrac3_pcm66_stream_finish(sony_atrac3_pcm66_stream *stream,
    uint8_t output[195],uint32_t end_bits[2]);
#ifdef __cplusplus
}
#endif
#endif
