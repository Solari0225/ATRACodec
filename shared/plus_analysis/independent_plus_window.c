#include "independent_plus_window.h"
#include "independent_plus_fft.h"
#if !defined(__clang__) && !defined(__GNUC__)
#include <math.h>
#endif
#include <string.h>
#include "window_tables.inc"
static float spill(double x){volatile float y=(float)x;return y;}
static float number(uint32_t word){float f;memcpy(&f,&word,4);return f;}
static float root(float x){
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_sqrtf(x);
#else
    return sqrtf(x);
#endif
}
int sony_recovered_plus_select_windows(const sony_plus_window_config *config,
    const float *const input[16],uint32_t bands,const int32_t prior_gain_count[16],
    sony_plus_window_result result[16],uint32_t output_flags[16]){
    if(!config||!input||!result||!output_flags||bands>16)return -1;
    uint32_t force=config->policy_force!=0;
    uint32_t processed=force||config->mode==2?1u:(uint32_t)config->low_band_limit;
    const uint32_t evolving=config->evolving_flags&(config->mode==2?4u:16u);
    if(evolving){force=1;processed=bands;}
    if(processed>bands)return -1;
    const uint32_t *threshold=config->max_qu<19?window_threshold_low:window_threshold_high;
    for(uint32_t band=0;band<processed;++band){
        if(!input[band]||(evolving&&band&&!prior_gain_count))return -1;
        const uint32_t bins=band<2?64u:32u,first_count=bins/8,stride=band<2?1u:2u;
        float power[129];if(sony_recovered_plus_power_spectrum(input[band],stride,bins*2,power))return -1;
        for(uint32_t i=0;i<=bins;++i)power[i]=root(power[i]);
        float maximum=0.0f;uint32_t maximum_index=0;
        for(uint32_t i=0;i<bins;++i)if(maximum<power[i]){maximum=power[i];maximum_index=i;}
        double first=0.0,rest=0.0;
        for(uint32_t i=0;i<first_count;++i)first+=(double)power[i];
        const float first_sum=spill(first),first_mean=spill(first/(double)first_count);
        for(uint32_t i=first_count;i<bins;++i)rest+=(double)power[i];
        const float total=spill((double)first_sum+rest),rest_mean=spill(rest/(double)(bins-first_count));
        result[band].peak_ratio=maximum>0.0f?spill((double)maximum/(double)total*(double)bins):1.0f;
        result[band].low_band_scale=1.0f;
        if(!band){
            if((double)rest_mean*number(window_lowband_factor)<(double)first_mean){
                switch(maximum_index){case 0:result[band].low_band_scale=8.0f;break;
                    case 1:result[band].low_band_scale=4.0f;break;
                    case 2:result[band].low_band_scale=2.0f;break;
                    case 3:result[band].low_band_scale=1.5f;break;default:break;}
            }
        }else if(evolving&&prior_gain_count[band]>0){
            result[band].low_band_scale=result[band].peak_ratio;
            if(result[band].peak_ratio>number(window_scale_cap))result[band].low_band_scale=8.0f;
        }
        if(result[band].peak_ratio<number(threshold[band]))result[band].window_flag=1;
        if(force)result[band].window_flag=0;
    }
    for(uint32_t band=processed;band<bands;++band){
        result[band].low_band_scale=1.0f;result[band].peak_ratio=1.0f;result[band].window_flag=0;
    }
    if(config->max_qu>=27){
        uint32_t count=0;for(uint32_t i=0;i<bands;++i)if(result[i].window_flag)++count;
        if(count>=4){
            float margin=0.0f;
            if(count>=3*bands/4)margin=number(window_margin_three_quarters);
            else if(count>=bands/2)margin=number(window_margin_half);
            else if(count>=bands/4)margin=number(window_margin_quarter);
            for(uint32_t i=0;i<bands;++i)
                if((double)number(threshold[i])+(double)margin>(double)result[i].peak_ratio)result[i].window_flag=1;
        }
    }
    for(uint32_t i=0;i<bands;++i)output_flags[i]=result[i].window_flag;
    return 0;
}
