#ifndef ATRACODEC_ATRAC3_DECODER_TABLES_H
#define ATRACODEC_ATRAC3_DECODER_TABLES_H
#include <stdint.h>
typedef struct atracodec_atrac3_decoder_book {
    uint32_t bits,lookup_count,width_count,value_count;
    const int8_t *lookup,*widths;
    const float *values;
} atracodec_atrac3_decoder_book;
typedef struct atracodec_atrac3_decoder_tables {
    float sf[64];
    uint32_t boundaries[33];
    atracodec_atrac3_decoder_book books[2][7];
} atracodec_atrac3_decoder_tables;
#ifdef __cplusplus
extern "C" {
#endif
const atracodec_atrac3_decoder_tables *atracodec_atrac3_decoder_default_tables(void);
#ifdef __cplusplus
}
#endif
#endif
