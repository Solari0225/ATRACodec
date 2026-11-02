#ifndef SONY_INDEPENDENT_PLUS_GAIN_LOG_H
#define SONY_INDEPENDENT_PLUS_GAIN_LOG_H
#ifdef __cplusplus
extern "C" {
#endif
/* Positive finite gain ratio -> original natural-log based gain measure.
 * Deterministic portable arithmetic, no platform libm or Sony runtime. */
double sony_recovered_plus_gain_log_measure(double ratio);
#ifdef __cplusplus
}
#endif
#endif
