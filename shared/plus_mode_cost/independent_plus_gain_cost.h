#ifndef INDEPENDENT_PLUS_GAIN_COST_H
#define INDEPENDENT_PLUS_GAIN_COST_H
#include "../plus_fields/independent_plus_gain.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum plus_gain_cost_component {PLUS_GAIN_COST_COUNTS,PLUS_GAIN_COST_LEVELS,PLUS_GAIN_COST_LOCATIONS} plus_gain_cost_component;
/* Costs exclude each component's two mode bits. Candidates update the same
 * auxiliary fields as the original even when another candidate later wins.
 * A present gain channel has1..16 bands,0..7 points/band, ordered locations
 *0..31, levels0..15, and at least one point overall for range candidates. */
int32_t plus_gain_mode_candidate(plus_given_gain_channel*,const plus_given_gain_channel* reference,
    plus_given_gain_location_parameters*,plus_gain_cost_component,uint32_t mode);
int32_t plus_gain_choose_component(plus_given_gain_channel*,const plus_given_gain_channel* reference,
    plus_given_gain_location_parameters*,plus_gain_cost_component);
/* Original3e88c0: choose!=0 evaluates four alternatives; zero forces direct
 * modes. Includes present/repeat/band-count and all three mode-bit headers. */
int32_t plus_gain_choose_modes(plus_given_gain_channel* channels,
    plus_given_gain_location_parameters*,int32_t channel_count,int32_t choose);
#ifdef __cplusplus
}
#endif
#endif
