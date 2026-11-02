#ifndef SONY_RECOVERED_PLUS_ALLOCATED_FRAME_H
#define SONY_RECOVERED_PLUS_ALLOCATED_FRAME_H
#include "recovered_plus_allocator.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_allocated_frame_parameters {uint32_t unit_id,header_flag,frame_bytes,noise_level,noise_table;} sony_plus_allocated_frame_parameters;
/* Complete frame packing from the independent allocator's own output. Only
 * upstream channel/header parameters and fixed physical frame size are supplied.
 * Returns the physical byte count or a negative packing error. */
int32_t sony_recovered_plus_pack_allocated_frame(const sony_plus_allocator_config*,
    const sony_plus_allocator_state*,const sony_plus_allocated_frame_parameters*,
    uint8_t *output,uint32_t capacity,uint32_t *codec_status);
#ifdef __cplusplus
}
#endif
#endif
