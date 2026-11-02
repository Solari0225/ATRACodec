#ifndef SONY_RECOVERED_ATRAC3_JOINT_H
#define SONY_RECOVERED_ATRAC3_JOINT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_joint_state {
    int32_t cutoff,current[4],previous[4];
    uint32_t weight_previous,weight_current;
    float scale_previous[4],scale_current[4];
    /* Three ages, four bands, left/right absolute-sum pairs. No opaque slots. */
    float energy[3][4][2];
    /* Current QMF time samples or MDCT frequency samples, stage-dependent. */
    float samples[2][1024];
} sony_atrac3_joint_state;
typedef struct sony_atrac3_joint_budget_config {
    uint32_t unit_bytes,left_maximum_qu,right_maximum_qu;
    int32_t baseline_left_budget;
    uint32_t right_low_gain_count;
} sony_atrac3_joint_budget_config;
/* Reconstructed464e80 mode/weight analysis and4629a0 band mixing.
 * Original finite-domain binary64 x87 arithmetic/binary32 stores, no FMA.
 * These stages mutate only their documented numeric state fields. */
int sony_recovered_atrac3_joint_analyze(sony_atrac3_joint_state *state);
int sony_recovered_atrac3_joint_mix_band(sony_atrac3_joint_state *state,uint32_t band);
/* Original463110 adaptive left-channel budget, given current MDCT samples
 * and already computed low-band gain count. Returns bits or negative error. */
int32_t sony_recovered_atrac3_joint_left_budget(const sony_atrac3_joint_state *state,
    const sony_atrac3_joint_budget_config *config);
#ifdef __cplusplus
}
#endif
#endif
