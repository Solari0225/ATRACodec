#include "plus_noise_float.h"
#include "plus_noise_tables.h"
int atracodec_plus_add_noise_float(float *const bands[16],uint32_t level,uint32_t start_index)
{
    unsigned band,i;
    double amplitude;
    if(!bands||level>15||start_index>31)return -1;
    for(band=0;band<16;++band)if(!bands[band])return -1;
    amplitude=(double)(1u<<level);
    for(band=0;band<16;++band) {
        const float *noise=plus_noise_vector1148+plus_noise_selector47[start_index+band];
        for(i=0;i<128;++i) {
            volatile double product=amplitude*(double)noise[i];
            bands[band][i]=(float)(product+(double)bands[band][i]);
        }
    }
    return 0;
}
