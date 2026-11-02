#ifndef SONY_INDEPENDENT_PLUS_GAIN_MATH_H
#define SONY_INDEPENDENT_PLUS_GAIN_MATH_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_gain_points {
    uint32_t count,location[7],level[7],window_flag;
} sony_plus_gain_points;
/* Original40ab10 gain interpolation/application, given selected0..7points.
 * Strictly increasing0..31locations represent groups of4time samples;
 * level0..15maps to gain exponent-6..9. Entire128sample array is in/out,
 * with unchanged unity tail. The point record is read-only. */
int sony_recovered_plus_apply_gain(float samples[128],const sony_plus_gain_points *points);
/* Complete actually selected SIMD path of original40aa80: apply the current
 * points to state[64..191], select exact overlap gain, window/fold and MDCT.
 * The native manager's other point-record argument is unused. This function
 * accepts only typed current gain fields and an already-decided direction;
 * it does not analyze input to choose gain points/window flags. */
int sony_recovered_plus_gain_mdct(float state[192],float output[128],
    const sony_plus_gain_points *points,uint32_t reverse_flag);
#ifdef __cplusplus
}
#endif
#endif
