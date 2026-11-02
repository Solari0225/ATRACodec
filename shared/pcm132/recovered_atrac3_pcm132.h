#ifndef SONY_RECOVERED_ATRAC3_PCM132_H
#define SONY_RECOVERED_ATRAC3_PCM132_H
#include "../mdct/recovered_atrac3_mdct.h"
#include "../gain_analysis/recovered_atrac3_gain.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_pcm132_channel {
    float qmf_history[138];
    sony_atrac3_mdct_state mdct;
    sony_atrac3_gain_channel gain;
} sony_atrac3_pcm132_channel;
typedef struct sony_atrac3_pcm132_state {
    sony_atrac3_pcm132_channel channel[2];
} sony_atrac3_pcm132_state;
/* Research-only PCM block composition for the original independent-stereo
 * 132kbps profile: signed16 stereo,44100Hz,1024 samples per channel. No Sony
 * runtime, upstream decision vectors, or substitute encoder are required.
 * init resets all history. block returns every384-byte DSP callback, INCLUDING
 * the initial priming callback. Container emission, final-input padding and
 * drain policy are outside this low-level API and are implemented/tested
 * separately by the pure-stream interface below. On failure reset state.
 */
void sony_recovered_atrac3_pcm132_init(sony_atrac3_pcm132_state *state);
int sony_recovered_atrac3_pcm132_block(sony_atrac3_pcm132_state *state,
    const int16_t pcm[2048],uint8_t output[387],uint32_t end_bits[2]);
typedef struct sony_atrac3_pcm132_stream {
    sony_atrac3_pcm132_state dsp;
    uint32_t primed,drain_step,nominal_bitrate;
} sony_atrac3_pcm132_stream;
/* Observed official pure-OMA profile emission contract: caller supplies full
 * blocks and zero pads any final short block, skips one initial callback,
 * then drains exactly3zero blocks. block/finish return1 for an emitted frame,
 *0 for priming/drain complete, negative for error. No empty-input contract.
 * finish may be called repeatedly and input is rejected once draining starts.
 * This profile API does not define AAL priming/tail/container policy.
 */
void sony_recovered_atrac3_pcm132_stream_init(sony_atrac3_pcm132_stream *stream);
int sony_recovered_atrac3_pcm132_stream_block(sony_atrac3_pcm132_stream *stream,
    const int16_t pcm[2048],uint8_t output[387],uint32_t end_bits[2]);
int sony_recovered_atrac3_pcm132_stream_finish(sony_atrac3_pcm132_stream *stream,
    uint8_t output[387],uint32_t end_bits[2]);
/* Original independent-stereo profile factory options105000/132000 only.
 *105: maximumQU28,1197bits per152-byte unit;132:QU30,1517bits/192-byte unit.
 * No66joint-stereo fallback. The legacy132 functions remain fixed-profile
 * convenience entry points. stream_init chooses the profile once per stream.
 */
typedef sony_atrac3_pcm132_state sony_atrac3_independent_stereo_state;
typedef sony_atrac3_pcm132_stream sony_atrac3_independent_stereo_stream;
int sony_recovered_atrac3_stereo_block(sony_atrac3_independent_stereo_state *state,
    const int16_t pcm[2048],uint32_t nominal_bitrate,uint8_t *output,
    uint32_t capacity,uint32_t end_bits[2]);
int sony_recovered_atrac3_stereo_stream_init(sony_atrac3_independent_stereo_stream *stream,
    uint32_t nominal_bitrate);
int sony_recovered_atrac3_stereo_stream_block(sony_atrac3_independent_stereo_stream *stream,
    const int16_t pcm[2048],uint8_t *output,uint32_t capacity,uint32_t end_bits[2]);
int sony_recovered_atrac3_stereo_stream_finish(sony_atrac3_independent_stereo_stream *stream,
    uint8_t *output,uint32_t capacity,uint32_t end_bits[2]);
#ifdef __cplusplus
}
#endif
#endif
