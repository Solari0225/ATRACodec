#ifndef INDEPENDENT_PLUS_PACKER_H
#define INDEPENDENT_PLUS_PACKER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Recovered ATRAC3plus packing primitives and given-stage orchestration.
 * Buffer writes OR new bits into existing bytes. Initialize an empty output
 * buffer to zero yourself. Existing bits are deliberately not cleared.
 * A failed bounded write sets error and leaves that individual write intact.
 */
typedef struct plus_bit_writer {
    uint8_t* data;
    size_t capacity;
    uint32_t bit_position;
    int error;
} plus_bit_writer;

enum { PLUS_PACK_INVALID=-1, PLUS_PACK_NO_SPACE=-2, PLUS_PACK_STAGE_FAILED=-3 };
int plus_pack_init(plus_bit_writer*,uint8_t* data,size_t capacity,uint32_t bit_position);
int plus_pack_bits(plus_bit_writer*,uint32_t value,unsigned width);
/* Actual unit-id primitive returns the number2; body helpers return final
 * absolute bit position. Negative results are bounded-API errors.
 */
int32_t plus_pack_unit_id(plus_bit_writer*,uint32_t raw_unit_id);
int32_t plus_pack_header(plus_bit_writer*,uint32_t raw_quant_unit_count,uint32_t raw_header_flag);
int32_t plus_pack_noise(plus_bit_writer*,uint32_t raw_flag,uint32_t raw_level,uint32_t raw_table);
int32_t plus_pack_finish(plus_bit_writer*,uint32_t target_total_bits);

typedef enum plus_pack_stage {
    PLUS_PACK_HEADER=0, PLUS_PACK_WORD_LENGTH=1, PLUS_PACK_SCALE_FACTOR=2,
    PLUS_PACK_CODE_TABLE=3, PLUS_PACK_SPECTRUM=4, PLUS_PACK_WINDOW=5,
    PLUS_PACK_GAIN=6, PLUS_PACK_TONES=7, PLUS_PACK_NOISE=8,
    PLUS_PACK_STAGE_COUNT=9
} plus_pack_stage;
/* Caller supplies the already-decided fields for each stage. The callback
 * must emit that stage's fields and return0. This does not choose compression
 * modes, codebooks, gain, tones, quantization or any encoder analysis decision.
 */
typedef int (*plus_pack_emit_stage)(void* user,plus_pack_stage stage,
                                   uint32_t channel_count,plus_bit_writer*);
int32_t plus_pack_body(plus_bit_writer*,uint32_t channel_count,
                      uint32_t expected_body_bits,plus_pack_emit_stage,
                      void* user,uint32_t* codec_status);
int32_t plus_pack_unit(plus_bit_writer*,uint32_t raw_unit_id,
                      uint32_t channel_count,uint32_t expected_body_bits,
                      plus_pack_emit_stage,void* user,uint32_t* codec_status);

#ifdef __cplusplus
}
#endif
#endif
