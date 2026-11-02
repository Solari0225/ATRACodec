#ifndef SONY_RECOVERED_PEAK_LANES_H
#define SONY_RECOVERED_PEAK_LANES_H
#ifdef __cplusplus
extern "C" {
#endif
/* Original active ATRAC3 math leaf: 32 windows of 32 input float samples,
   four independent sample lanes. output[lane*32+window] is the absolute peak
   among input[window*32 + lane + 4*k], k=0..7. IEEE binary32 is required.
   Exact SSE MAXPS operand order is retained, including its NaN selection,
   under DAZ-disabled semantics. Normal finite PCM values do not depend on DAZ.
   This is a single analysis primitive, not the entire gain/tonal encoder. */
int sony_recovered_peak_lanes_f32(const float input[1024],float output[128]);
#ifdef __cplusplus
}
#endif
#endif
