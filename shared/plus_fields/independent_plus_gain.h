#ifndef INDEPENDENT_PLUS_GAIN_H
#define INDEPENDENT_PLUS_GAIN_H
#include "../plus_packer/independent_plus_packer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct plus_given_gain_band {
    uint32_t points;
    uint32_t locations[7];
    uint32_t levels[7];
} plus_given_gain_band;
typedef struct plus_given_gain_channel {
    uint32_t channel_index,present,repeat_flag,repeat_bands,num_bands;
    uint32_t count_mode,count_width,count_base;
    uint32_t level_mode,level_width,level_base;
    uint32_t level_explicit[16];
    uint32_t location_mode;
    plus_given_gain_band bands[16];
} plus_given_gain_channel;
/* Already-decided modes and parameters. Valid gain bands1..16, points0..7,
 * levels0..15. Both channel-index parities and all four count/level modes.
 * The reference supplies the native left-channel values for stereo prediction.
 */
int32_t plus_pack_gain_counts(plus_bit_writer*,const plus_given_gain_channel*,const plus_given_gain_channel* reference);
int32_t plus_pack_gain_levels(plus_bit_writer*,const plus_given_gain_channel*,const plus_given_gain_channel* reference);
int32_t plus_pack_gain_locations_direct(plus_bit_writer*,const plus_given_gain_channel*);
/* Entire given-field gain stage when each active channel's location_mode==0.
 * Unsupported location modes are rejected, never silently replaced by0.
 * This routine does not choose gain points, modes, bases or replication flags.
 */
int32_t plus_pack_gain_direct_locations(plus_bit_writer*,const plus_given_gain_channel* channels,int32_t channel_count);
typedef struct plus_given_gain_location_parameters {
    uint32_t width,base;
    uint32_t explicit_values[16];
    uint32_t extra_values[16];
} plus_given_gain_location_parameters;
/* Complete given-field location serializer, all modes and both parities.
 * Non-direct location inputs have ordered locations0..31. Parameters supply
 * native width/base and already-decided per-band flags; no mode is selected.
 */
int32_t plus_pack_gain_locations(plus_bit_writer*,const plus_given_gain_channel*,
    const plus_given_gain_channel* reference,const plus_given_gain_location_parameters*);
/* Complete given-field gain stage for a mono/stereo channel unit. */
int32_t plus_pack_gain(plus_bit_writer*,const plus_given_gain_channel* channels,
    const plus_given_gain_location_parameters* parameters,int32_t channel_count);
#ifdef __cplusplus
}
#endif
#endif
