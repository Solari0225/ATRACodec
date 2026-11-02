#ifndef INDEPENDENT_PLUS_FAST_WL_H
#define INDEPENDENT_PLUS_FAST_WL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Original fast WL allocation cost graph, region+3f0610. This chooses among
 * already supplied word lengths; it does not analyze PCM or choose the WLs.
 * Native scratch mapping is documented in README.txt. All cost slots, including
 * unused alternatives and their parameters, are retained across updates. */
typedef struct plus_fast_wl_method {
    int32_t huffman_id,range_mode,encoded_units;
} plus_fast_wl_method;
typedef struct plus_fast_wl_parameters {
    int32_t mode,cost[4];
    plus_fast_wl_method method[3]; /* Methods 1,2,3. */
    int32_t total_units,zero_trimmed_units,one_trimmed_units;
} plus_fast_wl_parameters;

/* Returns 0/-1 for domain validation. Prepare only updates the three bounds.
 * Valid units are 0..32. Channel 1 strips zero tails; channel 0 strips one tails. */
int plus_fast_wl_prepare(const uint32_t word_length[32],int32_t units,
    uint32_t channel_index,plus_fast_wl_parameters*);
/* Returns the candidate cost, or -1 for an invalid mode/bounds. Method 3 is the
 * left temporal difference; methods 1/2 are right interchannel differences.
 * Updates only method[mode-1]; the caller owns the cached cost[mode] slot. */
int32_t plus_fast_wl_candidate(const uint32_t word_length[32],
    const uint32_t reference[32],plus_fast_wl_parameters*,uint32_t mode);
/* Original full/incremental chooser. enabled==0 only resets mode and returns
 * direct cost. A nonzero full_recompute rebuilds both channels. Otherwise only
 * channel_index's bounds are refreshed, with dependent right costs recomputed.
 * Valid channel count is 1 or 2. A right-only update uses the cached left mode,
 * which must be 0..3; a left update overwrites it before use. */
int32_t plus_fast_wl_choose(const uint32_t word_length[2][32],int32_t units,
    int32_t channel_count,int32_t full_recompute,int32_t channel_index,
    int32_t enabled,plus_fast_wl_parameters parameters[2]);

#ifdef __cplusplus
}
#endif
#endif
