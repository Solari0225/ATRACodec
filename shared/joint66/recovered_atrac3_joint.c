#include "recovered_atrac3_joint.h"
#include <string.h>
#include "joint_tables.inc"
static float f32(uint32_t value){float x;memcpy(&x,&value,4);return x;}
static double f64(const uint32_t *value){double x;memcpy(&x,value,8);return x;}
static double mul(double a,double b){volatile double r=a*b;return r;}
static double add(double a,double b){volatile double r=a+b;return r;}
static double sub(double a,double b){volatile double r=a-b;return r;}
static double divide(double a,double b){volatile double r=a/b;return r;}
static float store(double value){volatile float r=(float)value;return r;}
static double absolute(float value){return value<0?-(double)value:(double)value;}
static int mode(int32_t value){return value<0?-value:value;}
int sony_recovered_atrac3_joint_analyze(sony_atrac3_joint_state *s){
    if(!s||s->cutoff<0||s->cutoff>4)return -1;
    double accumulated_left=f32(joint_46f414[0]),accumulated_right=f32(0x3dcccccdu);
    for(uint32_t b=0;b<4;++b){
        double left=0,right=0;
        for(uint32_t t=0;t<256;++t){left=add(left,absolute(s->samples[0][4*t+b]));right=add(right,absolute(s->samples[1][4*t+b]));}
        const float saved_left=store(left),saved_right=store(right);
        s->energy[2][b][0]=s->energy[1][b][0];s->energy[2][b][1]=s->energy[1][b][1];
        s->energy[1][b][0]=s->energy[0][b][0];s->energy[1][b][1]=s->energy[0][b][1];
        s->energy[0][b][0]=saved_left;s->energy[0][b][1]=saved_right;
        const int32_t previous=s->previous[b];
        double limit=f32(joint_46f410[0]);float upper=f32(0x419d94ecu);
        if(b<(uint32_t)s->cutoff){limit=f32(joint_46f40c[0]);upper=f32(0x45100000u);
            if(!previous)limit=f32(joint_46f408[0]);else if(previous==1)upper=f32(0x42c00000u);}
        int32_t next=3;
        if(mul(limit,left)<right)next=0;
        else if(mul(upper,right)<left)next=1;
        else if(add(right,left)<f32(joint_46f404[0]))next=-3;
        s->current[b]=mode(previous);s->previous[b]=next;
        if(b>=(uint32_t)s->cutoff&&(next==3||next==-3)){
            accumulated_left=add(mul(accumulated_left,f32(joint_46f400[0])),saved_left);
            accumulated_right=add(mul(accumulated_right,f32(joint_46f400[0])),saved_right);
        }
    }
    const float right=store(accumulated_right),sum=store(add(accumulated_right,accumulated_left));
    uint32_t offset=0;double selected=accumulated_left;
    if(accumulated_left>accumulated_right){selected=right;offset=8;}
    const float rounded=store(add(mul(divide(selected,sum),f32(joint_46f3fc[0])),f32(joint_46f380[0])));
    uint32_t word;memcpy(&word,&rounded,4);uint32_t weight=(word&7)+offset;
    if(!weight||weight==8)++weight;
    s->weight_previous=s->weight_current;s->weight_current=weight;return 0;
}
int sony_recovered_atrac3_joint_mix_band(sony_atrac3_joint_state *s,uint32_t b){
    if(!s||b>3||s->cutoff<0||s->cutoff>4||s->weight_current>15||s->weight_previous>15)return -1;
    const int next=mode(s->previous[b]),old=s->current[b];
    if(next>3||old<0||old>3)return -1;
    float *left=s->samples[0]+b,*right=s->samples[1]+b;
    if(b>=(uint32_t)s->cutoff){
        uint32_t n=s->weight_current,o=s->weight_previous;
        if(!next){n^=8;o^=8;n+=8;o+=8;}else if(next==1){n+=8;o+=8;}else if(next==3){n&=7;o&=7;}
        const double new_weight=f32(joint_46c9a0[n]),old_weight=f32(joint_46c9a0[o]);
        if(!new_weight)return -2;
        const double delta=mul(sub(old_weight,new_weight),f32(joint_46f3d0[0]));
        for(int32_t i=-8;i<0;++i){const uint32_t p=(uint32_t)(i+8)*4;const double sum=add(right[p],left[p]);left[p]=store(divide(sum,sub(new_weight,mul(i,delta))));}
        const double inverse=divide(f32(joint_46f3cc[0]),new_weight);
        for(uint32_t t=8;t<256;++t){const uint32_t p=t*4;left[p]=store(mul(add(right[p],left[p]),inverse));}
        return 0;
    }
    const float energy_left=s->energy[0][b][0],energy_right=s->energy[0][b][1];
    const double a_left=f32(joint_46c988[next]),a_right=f32(joint_46c988[next+1]);
    const float b_left=f32(joint_46c988[old]),b_right=f32(joint_46c988[old+1]);
    float target=f32(0x3f800000u);
    if(next==3){
        double difference=0;int ratio=0;
        if(energy_right>mul(energy_left,f64(joint_46f3c4))&&mul(energy_left,f32(joint_46f3c0[0]))>energy_right){difference=sub(energy_right,energy_left);ratio=1;}
        else if(energy_left>mul(energy_right,f64(joint_46f3c4))&&mul(energy_right,f32(joint_46f3c0[0]))>energy_left){difference=sub(energy_left,energy_right);ratio=1;}
        if(ratio)target=store(divide(mul(difference,f32(joint_46f3bc[0])),add(energy_right,energy_left)));
        else if(energy_right!=f64(joint_46f3b4)&&energy_left!=f64(joint_46f3b4)&&
            (mul(energy_right,f32(joint_46f3c0[0]))<=energy_left||mul(energy_left,f32(joint_46f3c0[0]))<=energy_right))target=f32(0x3fd55555u);
    }
    const float previous=s->scale_previous[b];const double step=f64(joint_46f3ac);
    if(sub(target,previous)>=step)target=store(add(previous,step));
    if(sub(previous,target)>=step)target=store(sub(previous,step));
    s->scale_current[b]=target;
    const double new_right=mul(a_right,target),old_right=mul(b_right,previous);
    const float delta_left=store(mul(sub(b_left,a_left),f32(joint_46f3d0[0])));
    const double delta_right=mul(sub(old_right,new_right),f32(joint_46f3d0[0]));
    if(!new_right)return -2;s->scale_previous[b]=target;
    for(int32_t i=-8;i<0;++i){const uint32_t p=(uint32_t)(i+8)*4;
        const double original_left=left[p],mid=mul(add(right[p],original_left),f32(joint_46f3a8[0]));
        const double numerator=sub(original_left,mul(sub(a_left,mul(delta_left,i)),mid));
        right[p]=store(divide(numerator,sub(new_right,mul(i,delta_right))));left[p]=store(mid);
    }
    const double inverse=divide(f32(joint_46f3cc[0]),new_right);
    for(uint32_t t=8;t<256;++t){const uint32_t p=t*4;const double original_left=left[p],mid=mul(add(right[p],original_left),f32(joint_46f3a8[0]));right[p]=store(mul(sub(original_left,mul(mid,a_left)),inverse));left[p]=store(mid);}
    return 0;
}
static float maximum(const float *samples,uint32_t first,uint32_t end){float result=0;for(uint32_t i=first;i<end;++i){const float value=(float)absolute(samples[i]);if(result<value)result=value;}return result;}
int32_t sony_recovered_atrac3_joint_left_budget(const sony_atrac3_joint_state *s,
    const sony_atrac3_joint_budget_config *c){
    if(!s||!c||!c->left_maximum_qu||c->left_maximum_qu>32||!c->right_maximum_qu||c->right_maximum_qu>32||c->right_low_gain_count>7)return -1;
    double left=0,right=0;uint32_t count=c->left_maximum_qu;if(count>19)count=19;
    for(uint32_t q=count;q-->0;)left=add(left,mul(joint_qu_sizes[q],maximum(s->samples[0],joint_qu_starts[q],joint_qu_starts[q+1])));
    const float left_energy=store(left);
    for(uint32_t q=0;q<c->right_maximum_qu;++q)right=add(right,mul(joint_qu_sizes[q],maximum(s->samples[1],joint_qu_starts[q],joint_qu_starts[q+1])));
    float right_energy=store(add(right,right));const int32_t mode=s->current[0];
    if(mode!=3)right_energy=store(add(right_energy,right_energy));
    const int32_t total=(int32_t)(c->unit_bytes*16)-59,base=c->baseline_left_budget;
    if(right_energy<f32(joint_46f3cc[0])||(mode!=3&&right_energy<f32(joint_46f3f0[0])))return total;
    int32_t increment=0;
    if(!c->right_low_gain_count&&
       ((mul(right_energy,f32(joint_46f3ec[0]))<left_energy&&mode==3)||mul(right_energy,f32(joint_46f3e8[0]))<left_energy)){
        increment=total-base;
        if(mul(right_energy,f32(joint_46f3e4[0]))>left_energy){
            if(mul(right_energy,f32(joint_46f3e0[0]))<left_energy)increment=(int32_t)mul(increment,f32(joint_46f3dc[0]));
            else if(mul(right_energy,f32(joint_46f3d8[0]))<left_energy)increment=(int32_t)mul(increment,f32(joint_46f3d4[0]));
            increment=(int32_t)mul(divide(sub(left_energy,right_energy),add(left_energy,left_energy)),increment);
        }
    }else if(right_energy<left_energy&&mode==3){
        increment=(int32_t)mul(divide(sub(left_energy,right_energy),mul(left_energy,f32(joint_46f3e0[0]))),total-base);
    }
    return base+(increment&~7);
}
