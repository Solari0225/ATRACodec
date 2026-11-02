#ifndef ATRACODEC_ATRAC3_FLOAT_TRANSFORM_H
#define ATRACODEC_ATRAC3_FLOAT_TRANSFORM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Internal Sony-derived mathematical stage; coefficient values have their own
 * runtime provenance. This is not the AAL integer transform. */
typedef struct atracodec_atrac3_float_transform_tables {
    float scale[16];
    float fraction[8];
    float rotation[128][4];
    float butterfly[128];
    int32_t permutation[128];
} atracodec_atrac3_float_transform_tables;
const atracodec_atrac3_float_transform_tables *
atracodec_atrac3_float_transform_default_tables(void);

void atracodec_atrac3_float_transform(
    float signal[1162], float overlap[4][128],
    const int32_t gain_history[4][16], unsigned bands,
    const int32_t first_gain[4], const float spectrum[1024],
    const atracodec_atrac3_float_transform_tables *tables);

#ifdef __cplusplus
}
#endif
#endif
