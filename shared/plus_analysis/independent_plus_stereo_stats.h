#ifndef SONY_INDEPENDENT_PLUS_STEREO_STATS_H
#define SONY_INDEPENDENT_PLUS_STEREO_STATS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_stereo_metrics {
    float contrast[16],left_energy[16],right_energy[16];
    float recent_left[16],recent_right[16],recent_difference[16];
} sony_plus_stereo_metrics;
typedef struct sony_plus_stereo_weights {float old_weight[2][16],new_weight[2][16];} sony_plus_stereo_weights;
int sony_recovered_plus_stereo_contrast(const float left[16][128],
    const float right[16][128],float contrast[16],uint32_t bands);
int sony_recovered_plus_stereo_metrics(const float left[16][128],
    const float right[16][128],sony_plus_stereo_metrics *metrics,uint32_t bands);
int sony_recovered_plus_stereo_weights(float left[16][128],float right[16][128],
    const float contrast[16],sony_plus_stereo_weights *weights,
    int32_t coherence_boundary,uint32_t bands);
int sony_recovered_plus_stereo_weight_ramp(float previous,float current,
    const float sum[128],float output[128]);
int sony_recovered_plus_select_source_flags(const float guard[16],
    const float previous_metric[16],const float current_metric[16],
    const int32_t previous_flags[16],int32_t flags[16],uint32_t bands);
#ifdef __cplusplus
}
#endif
#endif
