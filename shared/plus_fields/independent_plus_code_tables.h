#ifndef INDEPENDENT_PLUS_CODE_TABLES_H
#define INDEPENDENT_PLUS_CODE_TABLES_H
#include "../plus_packer/independent_plus_packer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct plus_given_code_table_channel {
    uint32_t channel_index,table_flag,mode,partial_flag,partial_count;
    uint32_t selectors[32];
    uint32_t indices[32];
} plus_given_code_table_channel;
/* Given selector words, indices, mode and optional reduced range only.
 * Selectors1 use code-index coding; selectors2 emit one low bit; other values
 * emit nothing. Selection of these fields is not performed by this module.
 * The values helper excludes the table_flag/mode header, matching3f9e70.
 */
int32_t plus_pack_code_table_values(plus_bit_writer*,uint32_t full_table_flag,
    int32_t active_units,const plus_given_code_table_channel*,const plus_given_code_table_channel* reference);
int32_t plus_pack_code_tables(plus_bit_writer*,uint32_t full_table_flag,
    int32_t active_units,const plus_given_code_table_channel* channels,int32_t channel_count);
#ifdef __cplusplus
}
#endif
#endif
