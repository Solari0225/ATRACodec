#ifndef SONY_INDEPENDENT_PLUS_GAIN_HELPERS_H
#define SONY_INDEPENDENT_PLUS_GAIN_HELPERS_H
#include "independent_plus_gain_math.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual gain-search arithmetic leaves; these do not constitute full4073e0.
 * Max leaves retain the first operand on ties. For count<=1 the original
 * still reads values[0]; caller therefore always provides one element.
 * Envelope inputs are finite nonnegative amplitudes. */
float sony_recovered_plus_gain_maximum(const float *values,int32_t count);
float sony_recovered_plus_gain_absmaximum(const float *values,int32_t count);
int32_t sony_recovered_plus_gain_level_index(int32_t exponent);
int32_t sony_recovered_plus_gain_above_unity(const sony_plus_gain_points *points);
int32_t sony_recovered_plus_gain_envelope_turns(float initial_floor,const float envelope[32]);
/* Original4092f0 ratio for subsequent log analysis. Retains binary64 result;
 * only a denominator strictly above4is used, otherwise numerator/4. */
double sony_recovered_plus_gain_floor_ratio(float denominator,float numerator);
#ifdef __cplusplus
}
#endif
#endif
