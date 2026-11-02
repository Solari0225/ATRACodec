#ifndef INDEPENDENT_ATRAC3_QMF_H
#define INDEPENDENT_ATRAC3_QMF_H
#ifdef __cplusplus
extern "C" {
#endif
/* Recovered active ATRAC3 analysis kernel, region-relative entry 0x460c20.
 * input: 1024 finite binary32 PCM values; output: 1024 binary32 band values.
 * history: 138 binary32 values, updated in place. Reset all words to +0.
 * Exact operation order requires round-to-nearest binary32, gradual underflow,
 * no fast-math reassociation and no multiply/add contraction.
 * input==output is supported. Other overlapping arrays are not specified.
 * The history layout is 92 previous intermediate values + 46 previous PCM.
 */
void independent_atrac3_qmf(const float input[1024], float history[138], float output[1024]);
#ifdef __cplusplus
}
#endif
#endif
