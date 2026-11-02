#ifndef SONY_RECOVERED_ATRAC3_GAIN_H
#define SONY_RECOVERED_ATRAC3_GAIN_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_gain_record {
    uint32_t location[8];
    int32_t level[8];
    float envelope_history[32];
    float history_maximum,last_quarter_maximum;
    uint32_t point_count;
} sony_atrac3_gain_record;
typedef struct sony_atrac3_gain_channel {
    float previous_samples[1024];
    sony_atrac3_gain_record band[4];
    uint32_t previous_low_band_count;
} sony_atrac3_gain_channel;
typedef struct sony_atrac3_gain_config {
    uint32_t extended_header,current[4],previous[4];
} sony_atrac3_gain_config;
/* Original465070/465400 per-band gain decision and application. peaks[32]
 * are the actual band maxima from460a80, in the encoder's internal QMF scale.
 * Mutates previous interleaved samples, one gain record, and low-band history.
 * Returns new point count0..7, but preserves point_count until the caller
 * commits it, matching the original main. Finite nonnegative peak inputs.
 * Original x87 binary64 intermediates/binary32 stores; disable FMA contraction.
 */
int32_t sony_recovered_atrac3_gain_band(sony_atrac3_gain_channel *channel,
    uint32_t band,const sony_atrac3_gain_config *config,const float peaks[32]);
/* Original main order3,2,1,0, followed by point-count and level4 sentinel
 * commits. peaks is band-major4x32, as returned by recovered peak kernel. */
int sony_recovered_atrac3_gain_frame(sony_atrac3_gain_channel *channel,
    const sony_atrac3_gain_config *config,const float peaks[4][32]);
#ifdef __cplusplus
}
#endif
#endif
