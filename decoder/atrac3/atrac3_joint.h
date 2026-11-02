#ifndef ATRACODEC_ATRAC3_JOINT_H
#define ATRACODEC_ATRAC3_JOINT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_atrac3_joint_tables {
    float matrix[6];
    float weights[16][2];
    float interpolation_step,default_weight;
} atracodec_atrac3_joint_tables;
const atracodec_atrac3_joint_tables *atracodec_atrac3_joint_default_tables(void);
typedef struct atracodec_atrac3_joint_config {
    uint32_t matrix_modes[4],previous_matrix_modes[4];
    uint32_t weight_mode,previous_weight_mode;
    int32_t threshold,previous_threshold;
} atracodec_atrac3_joint_config;
void atracodec_atrac3_joint_synthesize(float left[1162],float right[1162],
    const atracodec_atrac3_joint_config *config,
    const atracodec_atrac3_joint_tables *tables);
#ifdef __cplusplus
}
#endif
#endif
