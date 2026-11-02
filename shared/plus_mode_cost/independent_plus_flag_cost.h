#ifndef INDEPENDENT_PLUS_FLAG_COST_H
#define INDEPENDENT_PLUS_FLAG_COST_H
#include "../plus_fields/independent_plus_flags.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original3ee400 decides present/explicit-values from the uint32 wrapped sum
 * of the given words and returns their exact bit cost. Values are unchanged.
 * Full-word sums are intentional, not a reduction of the words' low bits. */
int32_t plus_mode_choose_flags(plus_given_flag_vector* vector,int32_t count);
#ifdef __cplusplus
}
#endif
#endif
