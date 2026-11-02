#ifndef SONY_RECONSTRUCTION_ATRAC3_PUT_WORDS_H
#define SONY_RECONSTRUCTION_ATRAC3_PUT_WORDS_H
#include <stdint.h>
#if defined(_WIN32) && defined(SONY_MATH_TEST_EXPORTS)
#define SONY_MATH_API __declspec(dllexport)
#else
#define SONY_MATH_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
/* Recovered narrow ATRAC3 packing primitives, not a complete frame writer.
 * word already contains the caller's aligned code bits. The current byte is
 * ORed; following bytes are overwritten, exactly as in the observed helpers.
 * The caller must provide at least (bit_position >> 3) + 3 / + 4 writable
 * bytes respectively. No data-dependent tables or Sony code are linked.
 */
SONY_MATH_API void sony_recovered_atrac3_put24(uint32_t word,
                                            uint32_t bit_position,
                                            uint8_t *output);
SONY_MATH_API void sony_recovered_atrac3_put32(uint32_t word,
                                            uint32_t bit_position,
                                            uint8_t *output);
#ifdef __cplusplus
}
#endif
#endif
