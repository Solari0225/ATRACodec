#ifndef INDEPENDENT_PLUS_FLAGS_H
#define INDEPENDENT_PLUS_FLAGS_H
#include "../plus_packer/independent_plus_packer.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Already-decided flag coding: present==0 emits one zero bit. Otherwise the
 * explicit flag is emitted; when it is nonzero, count one-bit values follow.
 * A nonzero raw word is tested as a full word, but only its low bit is packed.
 * No coding-mode or window-analysis decision is performed here.
 */
typedef struct plus_given_flag_vector {
    uint32_t present;
    uint32_t explicit_values;
    const uint32_t* values;
} plus_given_flag_vector;
int32_t plus_pack_flag_vector(plus_bit_writer*,const plus_given_flag_vector*,int32_t count);
int32_t plus_pack_window_flags(plus_bit_writer*,const plus_given_flag_vector* channels,
                               int32_t channel_count,int32_t subband_count);
#ifdef __cplusplus
}
#endif
#endif
