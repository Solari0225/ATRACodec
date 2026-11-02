#include "independent_plus_gain_cross.h"
#include <string.h>
static const float minimum_contrast[16]={10.f,10.f,10.f,10.f,12.f,16.f,16.f,16.f,16.f,16.f,16.f,16.f,16.f,16.f,16.f,16.f};
_Static_assert(sizeof(sony_plus_gain_record)==152,"Typed native gain record size");
int32_t sony_recovered_plus_gain_points_equal(const sony_plus_gain_points *a,
    const sony_plus_gain_points *b)
{
    if(a->count!=b->count)return 0;
    for(uint32_t i=0;i<a->count;++i)
        if(a->location[i]!=b->location[i]||a->level[i]!=b->level[i])return 0;
    return 1;
}
static void copy_simpler(sony_plus_gain_record *left,sony_plus_gain_record *right)
{
    if(right->points.count>=left->points.count)memcpy(right,left,sizeof(*left));
    else memcpy(left,right,sizeof(*left));
}
int sony_recovered_plus_unify_gain(sony_plus_gain_record current[2][16],
    const float previous_maximum[2][16],const float contrast[16],
    const float secondary_contrast[16],int32_t coherence[16],
    int32_t max_qu,int32_t coherence_start,uint32_t bands)
{
    if(!current||!previous_maximum||!contrast||!secondary_contrast||!coherence||bands>16)return -1;
    int eligible[16];
    for(uint32_t i=0;i<bands;++i){
        const sony_plus_gain_points *a=&current[0][i].points,*b=&current[1][i].points;
        if(a->count>7||b->count>7)return -2;
        const int32_t x=a->count?(int32_t)a->level[0]-6:0;
        const int32_t y=b->count?(int32_t)b->level[0]-6:0;
        eligible[i]=!(x-y>1&&x>0&&
            (previous_maximum[0][i]<previous_maximum[1][i]||previous_maximum[1][i]>=32768.f));
    }
    const float threshold=max_qu>=23?40.f:max_qu>=19?30.f:20.f;
    for(uint32_t i=0;i<bands;++i){
        if(!eligible[i])continue;
        int copy=contrast[i]>threshold;
        if(!copy&&contrast[i]>minimum_contrast[i]&&(int32_t)i>coherence_start){
            if(i+1<bands)copy=coherence[i-1]&&coherence[i]&&coherence[i+1];
            else if(bands>2)copy=coherence[i-2]&&coherence[i-1]&&coherence[i];
        }
        if(copy)copy_simpler(&current[0][i],&current[1][i]);
    }
    for(uint32_t i=0;i<bands;++i)
        coherence[i]=sony_recovered_plus_gain_points_equal(&current[0][i].points,&current[1][i].points);
    const double relaxed=(double)threshold*(double)0x1.333334p-1f;
    for(uint32_t i=0;i<bands;++i){
        if(!coherence[i]&&eligible[i]&&relaxed<=(double)contrast[i]&&relaxed<=(double)secondary_contrast[i]){
            copy_simpler(&current[0][i],&current[1][i]);
            coherence[i]=1;
        }
    }
    return 0;
}
