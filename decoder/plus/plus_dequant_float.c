/* Sony ordinary403990, retained multiplication/store order, no external codec. */
#include "plus_dequant_float.h"
#include "plus_dequant_tables.h"
int atracodec_plus_dequant_float(const int16_t *input,float *output,
    uint32_t word_length,uint32_t scale_factor,uint32_t count)
{
    uint32_t i;
    volatile float factor;
    if(!input||!output||word_length<1||word_length>7||scale_factor>63||count>128)return -1;
    factor=(float)((double)plus_dequant_scale_factor64[scale_factor]*(double)plus_dequant_word_factor8[word_length]);
    for(i=0;i<count;++i)output[i]=(float)((double)input[i]*(double)factor);
    return 0;
}
