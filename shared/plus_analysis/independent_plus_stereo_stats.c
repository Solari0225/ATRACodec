#include "independent_plus_stereo_stats.h"
#include "independent_plus_math.h"
#include <string.h>
extern double sqrt(double);
static float spill(double x){volatile float result=(float)x;return result;}
static float energy(const float source[128]){
    float lane[4]={0.,0.,0.,0.};
    for(uint32_t i=0;i<128;i+=4)for(uint32_t j=0;j<4;++j){const float x=source[i+j],p=spill((double)x*(double)x);lane[j]=spill((double)lane[j]+(double)p);}
    const float a=spill((double)lane[0]+(double)lane[1]),b=spill((double)lane[2]+(double)lane[3]);return spill((double)a+(double)b);
}
static float contrast_db(float left,float right,float difference){
    float ratio;
    if((left==0.f&&right==0.f)||difference==0.f)ratio=0.001f;
    else if(left==0.f||right==0.f)ratio=1.f;
    else ratio=spill((double)difference/(double)(left<right?right:left));
    double result=ratio>0.f?-sony_plus_math_positive_ln(ratio)*8.68588924407958984375:160.;
    if(result>60.)result=60.;return spill(result);
}
static void measures(const float left[128],const float right[128],float output[3]){
    float difference[128];for(uint32_t i=0;i<128;++i)difference[i]=spill((double)left[i]-(double)right[i]);
    output[0]=energy(left);output[1]=energy(right);output[2]=energy(difference);
}
int sony_recovered_plus_stereo_contrast(const float left[16][128],const float right[16][128],float contrast[16],uint32_t bands){
    if(!left||!right||!contrast||bands>16)return -1;
    for(uint32_t b=0;b<bands;++b){float v[3];measures(left[b],right[b],v);contrast[b]=contrast_db(v[0],v[1],v[2]);}return 0;
}
int sony_recovered_plus_stereo_metrics(const float left[16][128],const float right[16][128],sony_plus_stereo_metrics *metrics,uint32_t bands){
    if(!left||!right||!metrics||bands>16)return -1;
    for(uint32_t b=0;b<bands;++b){
        float v[3];measures(left[b],right[b],v);
        const float l=spill((double)metrics->recent_left[b]+(double)v[0]),r=spill((double)metrics->recent_right[b]+(double)v[1]),d=spill((double)metrics->recent_difference[b]+(double)v[2]);
        metrics->recent_left[b]=v[0];metrics->recent_right[b]=v[1];metrics->recent_difference[b]=v[2];metrics->left_energy[b]=l;metrics->right_energy[b]=r;metrics->contrast[b]=contrast_db(l,r,d);
    }return 0;
}
int sony_recovered_plus_stereo_weight_ramp(float previous,float current,const float sum[128],float output[128]){
    if(!sum||!output)return -1;
    uint32_t count=8;
    if(previous!=0.f&&current!=0.f){const double ratio=current>previous?(double)current/(double)previous:(double)previous/(double)current;count=ratio>16.?8:ratio>8.?16:ratio>4.?32:64;}
    const float delta=spill(((double)current-(double)previous)/(double)count);double ramp=previous;
    for(uint32_t i=0;i<count;++i){const float factor=spill(ramp);output[i]=spill((double)sum[i]*(double)factor);ramp+=(double)delta;}
    for(uint32_t i=count;i<128;++i)output[i]=spill((double)sum[i]*(double)current);
    return 0;
}
int sony_recovered_plus_stereo_weights(float left[16][128],float right[16][128],const float contrast[16],sony_plus_stereo_weights *weights,int32_t coherence_boundary,uint32_t bands){
    if(!left||!right||!contrast||!weights||bands>16||coherence_boundary<0||coherence_boundary>16)return -1;
    for(uint32_t b=(uint32_t)coherence_boundary;b<bands;++b){
        if(contrast[b]>-11.f){
            const float l=energy(left[b]),r=energy(right[b]);float sum[128];for(uint32_t i=0;i<128;++i)sum[i]=spill((double)left[b][i]+(double)right[b][i]);
            const double average=(double)energy(sum)*.25;
            weights->new_weight[0][b]=average==0.?.5f:spill(sqrt((double)l/average)*.5);
            weights->new_weight[1][b]=average==0.?.5f:spill(sqrt((double)r/average)*.5);
            sony_recovered_plus_stereo_weight_ramp(weights->old_weight[0][b],weights->new_weight[0][b],sum,left[b]);
            sony_recovered_plus_stereo_weight_ramp(weights->old_weight[1][b],weights->new_weight[1][b],sum,right[b]);
        }else weights->new_weight[0][b]=weights->new_weight[1][b]=.5f;
    }
    memcpy(weights->old_weight,weights->new_weight,sizeof(weights->old_weight));return 0;
}
int sony_recovered_plus_select_source_flags(const float guard[16],const float previous_metric[16],const float current_metric[16],const int32_t previous_flags[16],int32_t flags[16],uint32_t bands){
    if(!guard||!previous_metric||!current_metric||!previous_flags||!flags||bands>16)return -1;
    for(uint32_t i=0;i<bands;++i){const double threshold=(double)previous_metric[i]*(previous_flags[i]?4.:8.);flags[i]=guard[i]<0.f?0:threshold<(double)current_metric[i];}return 0;
}
