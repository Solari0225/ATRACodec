#include "independent_plus_window_stereo.h"
#include <string.h>
#include "window_stereo_tables.inc"
static float number(uint32_t word){float f;memcpy(&f,&word,4);return f;}
int sony_recovered_plus_unify_windows(sony_plus_window_result result[2][16],
    uint32_t output_flags[2][16],const float contrast[16],uint32_t coherence_start,
    const int32_t channel_source_flags[16],uint32_t bands){
    if(!result||!output_flags||!contrast||!channel_source_flags||bands>16)return -1;
    for(uint32_t i=0;i<bands;++i){
        if(result[0][i].window_flag==result[1][i].window_flag)continue;
        const uint32_t source=result[0][i].peak_ratio>result[1][i].peak_ratio?0u:1u;
        const uint32_t other=1u-source;
        const double difference=(double)result[source][i].peak_ratio-(double)result[other][i].peak_ratio;
        if(difference<(double)number(window_stereo_min_separation)||
                contrast[i]>number(window_stereo_contrast_limit)){
            output_flags[other][i]=output_flags[source][i];
            result[other][i].window_flag=result[source][i].window_flag;
        }
    }
    for(uint32_t i=coherence_start;i<bands;++i){
        const uint32_t source=channel_source_flags[i]!=0?1u:0u,other=1u-source;
        output_flags[other][i]=output_flags[source][i];
        result[other][i].window_flag=result[source][i].window_flag;
    }
    return 0;
}
