#include "recovered_atrac3_spectrum.h"
#include "../../atrac3/put_words.h"
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 &&
               FLT_MAX_EXP==128, "IEEE binary32 required");
typedef struct { uint32_t word, length; } codeword;
#include "spectrum_tables.inc"

static float number(uint32_t bits)
{
    union { uint32_t bits; float value; } data;
    data.bits=bits; return data.value;
}
static uint32_t quantize(float input, float scale, uint32_t mask)
{
    union { uint32_t bits; float value; } data;
    /* Separate binary32 operations implement the executed MULPS then ADDPS.
     * No FMA or host integer rounding conversion is permitted here. */
    volatile float product=input*scale;
    volatile float rounded=product+number(UINT32_C(0x4b400000));
    data.value=rounded;return data.bits&mask;
}
int sony_recovered_atrac3_write_spectrum(const float spectrum[1024],
    const uint32_t word_lengths[32], const uint32_t scale_factors[32],
    uint32_t quantization_units, uint8_t *output, uint32_t capacity,
    uint32_t start_bit, uint32_t *end_bit)
{
    if(!spectrum || !word_lengths || !scale_factors || !output || !end_bit ||
       !quantization_units || quantization_units>32 || capacity>UINT32_MAX/8 ||
       start_bit>capacity*8) return -1;
    for(uint32_t unit=0;unit<quantization_units;++unit)
        if(word_lengths[unit]>7 || (word_lengths[unit] && scale_factors[unit]>63))
            return -1;
    uint32_t bit=start_bit;
    for(uint32_t unit=0;unit<quantization_units;++unit){
        const uint32_t wl=word_lengths[unit],sf=scale_factors[unit];
        if(!wl)continue;
        const uint32_t mask=spectrum_masks[wl];
        const float scale=number(spectrum_scales[3*wl-sf%3]-((sf/3)<<23));
        const codeword *book=spectrum_books[wl];
        if(wl==1){
            for(uint32_t i=spectrum_starts[unit];i<spectrum_starts[unit+1];i+=8){
                uint32_t word=0,length=0;
                for(uint32_t j=0;j<8;j+=2){
                    const uint32_t a=quantize(spectrum[i+j],scale,mask);
                    const uint32_t b=quantize(spectrum[i+j+1],scale,mask);
                    const codeword code=book[a*4+b];
                    word |= code.word>>length;length+=code.length;
                }
                if((bit>>3)>capacity || capacity-(bit>>3)<4 ||
                   length>capacity*8-bit) return -2;
                sony_recovered_atrac3_put32(word,bit,output);bit+=length;
            }
        }else{
            for(uint32_t i=spectrum_starts[unit];i<spectrum_starts[unit+1];i+=2){
                const codeword a=book[quantize(spectrum[i],scale,mask)];
                const codeword b=book[quantize(spectrum[i+1],scale,mask)];
                const uint32_t word=a.word|(b.word>>a.length),length=a.length+b.length;
                if((bit>>3)>capacity || capacity-(bit>>3)<3 ||
                   length>capacity*8-bit) return -2;
                sony_recovered_atrac3_put24(word,bit,output);bit+=length;
            }
        }
    }
    *end_bit=bit;return 0;
}
