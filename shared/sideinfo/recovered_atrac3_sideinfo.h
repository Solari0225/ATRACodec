#ifndef SONY_RECOVERED_ATRAC3_SIDEINFO_H
#define SONY_RECOVERED_ATRAC3_SIDEINFO_H
#include <stdint.h>
#if defined(_WIN32) && defined(SONY_SIDEINFO_TEST_EXPORTS)
#define SONY_META_API __declspec(dllexport)
#else
#define SONY_META_API
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_gain_band {
    uint32_t count,level[8],location[8];
} sony_atrac3_gain_band;
typedef struct sony_atrac3_tonal_component {
    uint32_t scale_factor,position,symbol[8];
} sony_atrac3_tonal_component;
typedef struct sony_atrac3_tonal_window {
    uint32_t count;
    const sony_atrac3_tonal_component* components;
} sony_atrac3_tonal_window;
typedef struct sony_atrac3_tonal_group {
    uint32_t band_present[4],word_length,coefficients_minus_one;
    sony_atrac3_tonal_window window[16];
} sony_atrac3_tonal_group;
typedef struct sony_atrac3_sideinfo {
    uint32_t extended_header,header_fields[5],subbands;
    sony_atrac3_gain_band gain[4];
    uint32_t tonal_mode,tonal_group_count;
    const sony_atrac3_tonal_group* tonal_groups;
    uint32_t quantization_units,word_length[32],scale_factor[32];
} sony_atrac3_sideinfo;
/* Recover the actual channel side-information writer +462ce0, its tonal
   writer +462f00, and their two byte insertion primitives. The supplied
   metadata are ALREADY decided by the encoder; this function does not make
   gain, tonal, quantization or allocation decisions. It stops exactly before
   the original fused spectral quantizer/packer callback.
   Original supported global tonal modes0/1 and word lengths2..7 are used;
   null codebook combinations fail, never substitute another codebook.
   Returns0 on success, -1 invalid metadata, -2 insufficient buffer, -3 absent
   codebook. end_bit is relative to output start. Valid codeword writes need
   one padding byte beyond the final partially occupied byte, as in original.
   On failure the caller must discard the output buffer. */
SONY_META_API int sony_recovered_atrac3_write_sideinfo(
    const sony_atrac3_sideinfo* info,uint8_t* output,uint32_t capacity,
    uint32_t byte_offset,uint32_t* end_bit);
#ifdef __cplusplus
}
#endif
#endif
