#ifndef INDEPENDENT_PLUS_SPECTRUM_STATS_H
#define INDEPENDENT_PLUS_SPECTRUM_STATS_H
#include "independent_plus_coarse.h"
#include "../plus_fields/independent_plus_gain.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original3ee300/40b89d, given first256 spectral values. Scalar uses binary64
 * accumulation; original SSE uses four binary32 lanes and its exact reduction.
 * Outputs are ratios of mean absolute first16 bins to16..127 and128..255. */
int plus_spectrum_low_ratios(const float spectrum[256],uint32_t backend,float ratios[2]);
/* Original3ecc30/40b37d. Per-QU N*peak/sum(abs(values)); nonpositive supplied
 * peaks give1. The original fills every unused output QU with1. */
int plus_spectrum_peakiness(const float spectrum[2048],const float peak[32],
    uint32_t units,uint32_t backend,float ratio[32]);
/* Original3e8ce0 /3e8b30: baseline6 level spread, activity across current/prior
 * first2 gain bands, and exact trailing/repeated gain-band header shortening.
 * Given gain points/locations/levels, not gain analysis from PCM. */
int32_t plus_spectrum_gain_spread(const plus_given_gain_band*);
int plus_spectrum_gain_header(plus_given_gain_channel*,
    const plus_given_gain_band previous[16],uint32_t bands,int16_t* activity);
#ifdef __cplusplus
}
#endif
#endif
