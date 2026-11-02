/* Sony original3f1540 mathematics, independently expressed in portable C. */
#include "plus_gain_float.h"
#include "plus_gain_tables.h"

static float power_of_two(int n)
{
    return n<0?1.0f/(float)(1u<<(unsigned)-n):(float)(1u<<(unsigned)n);
}
float atracodec_plus_gain_scale_float(const atracodec_plus_gain *gain)
{
    if(!gain||!gain->count)return 1.0f;
    if(gain->count>7||gain->levels[0]>15)return 0.0f;
    return power_of_two(-plus_gain_exponents[gain->levels[0]]);
}
int atracodec_plus_apply_gain_float(const atracodec_plus_gain *gain,float samples[128])
{
    unsigned point,at=0;
    int current;
    if(!gain||!samples||gain->count>7)return -1;
    for(point=0;point<gain->count;++point)
        if(gain->locations[point]>31||gain->levels[point]>15||
           (point&&gain->locations[point]<=gain->locations[point-1]))return -1;
    if(!gain->count)return 0;
    current=plus_gain_exponents[gain->levels[0]];
    for(point=0;point<gain->count;++point) {
        int next=point+1<gain->count?plus_gain_exponents[gain->levels[point+1]]:0;
        float scale=power_of_two(-current);
        while(at<gain->locations[point]) {
            unsigned j;
            for(j=0;j<4;++j)samples[at*4+j]=(float)((double)samples[at*4+j]*(double)scale);
            ++at;
        }
        samples[at*4]=(float)((double)samples[at*4]*(double)scale);
        if(current!=next) {
            unsigned j;
            unsigned difference=(unsigned)(current<next?next-current:current-next);
            float base=current<next?scale:power_of_two(-next);
            const float *interpolation=plus_gain_interpolation45+(difference-1)*3;
            for(j=0;j<3;++j) {
                unsigned order=current<next?2-j:j;
                volatile double product=(double)interpolation[order]*(double)samples[at*4+1+j];
                samples[at*4+1+j]=(float)(product*(double)base);
            }
        } else {
            unsigned j;
            for(j=1;j<4;++j)samples[at*4+j]=(float)((double)samples[at*4+j]*(double)scale);
        }
        ++at;current=next;
    }
    return 0;
}
