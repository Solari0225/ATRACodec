#ifndef ATRACODEC_ATRAC3_CORE_H
#define ATRACODEC_ATRAC3_CORE_H
#include <stddef.h>
#include <stdint.h>
#include "atrac3_joint.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct atracodec_atrac3_channel_state {
    float signal[1162],overlap[4][128];
    int32_t gain_history[4][16];
    uint32_t active_bands;
} atracodec_atrac3_channel_state;
typedef struct atracodec_atrac3_core_state {
    atracodec_atrac3_channel_state channel[2];
    atracodec_atrac3_joint_config joint;
    uint32_t bitrate,frame_bytes,joint_stereo;
} atracodec_atrac3_core_state;
int atracodec_atrac3_core_initialize(atracodec_atrac3_core_state*,uint32_t bitrate);
/* One physical core frame produces1024samples/ch. Container timing/first-frame
 * suppression is performed by the whole-file interface, not this raw stage. */
int atracodec_atrac3_core_decode(atracodec_atrac3_core_state*,const uint8_t*,size_t,int16_t output[2048]);
#ifdef __cplusplus
}
#endif
#endif
