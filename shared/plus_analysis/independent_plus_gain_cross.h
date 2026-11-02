#ifndef SONY_INDEPENDENT_PLUS_GAIN_CROSS_H
#define SONY_INDEPENDENT_PLUS_GAIN_CROSS_H
#include "independent_plus_gain_band.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original405090 compares only count and active location/level pairs. */
int32_t sony_recovered_plus_gain_points_equal(const sony_plus_gain_points *a,
    const sony_plus_gain_points *b);
/* Original4070d0. Coherence is read by the first copy pass and rewritten by
 * pair comparison, then amended by a second copy pass. Copying transfers all
 * 38 typed gain record words. The lower point count wins; ties select left. */
int sony_recovered_plus_unify_gain(sony_plus_gain_record current[2][16],
    const float previous_maximum[2][16],const float contrast[16],
    const float secondary_contrast[16],int32_t coherence[16],
    int32_t max_qu,int32_t coherence_start,uint32_t bands);
#ifdef __cplusplus
}
#endif
#endif
