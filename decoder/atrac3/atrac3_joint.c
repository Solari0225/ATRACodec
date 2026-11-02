#include "atrac3_joint.h"
/* Independent mathematical reconstruction of Sony normal ATRAC3 routine
 * image+4676b0. CW027f x87 operations stay binary64 until explicit FST f32. */
void atracodec_atrac3_joint_synthesize(float left[1162],float right[1162],
    const atracodec_atrac3_joint_config *c,const atracodec_atrac3_joint_tables *t)
{
    unsigned band;
    for(band=0;band<4;++band) {
        unsigned mode=c->matrix_modes[band],prior=c->previous_matrix_modes[band];
        double a=t->matrix[mode],b=t->matrix[mode+1];
        double wl=t->default_weight,wr=t->default_weight;
        float old_wl=t->default_weight,old_wr=t->default_weight;
        unsigned sample=0;
        if((int32_t)band>c->threshold) {
            wl=t->weights[c->weight_mode][0];wr=t->weights[c->weight_mode][1];
        }
        if((int32_t)band>c->previous_threshold) {
            old_wl=t->weights[c->previous_weight_mode][0];old_wr=t->weights[c->previous_weight_mode][1];
        }
        if((double)old_wl!=wl || mode!=prior) {
            double da=(a-(double)t->matrix[prior])*(double)t->interpolation_step;
            double db=(b-(double)t->matrix[prior+1])*(double)t->interpolation_step;
            float dwl=(float)((wl-(double)old_wl)*(double)t->interpolation_step);
            float dwr=(float)((wr-(double)old_wr)*(double)t->interpolation_step);
            for(sample=0;sample<8;++sample) {
                unsigned p=138+band+4*sample;
                double s=(double)((int)sample-8);
                double l=left[p],r=right[p];
                double ca=s*da+a,cb=s*db+b;
                double mix=cb*r+ca*l;
                double right_weight=s*(double)dwr+wr;
                double left_weight=s*(double)dwl+wl;
                right[p]=(float)((l-mix)*right_weight);
                left[p]=(float)(left_weight*mix);
            }
        }
        for(;sample<256;++sample) {
            unsigned p=138+band+4*sample;
            double l=left[p],r=right[p];
            double mix=b*r+a*l;
            left[p]=(float)(mix*wl);
            right[p]=(float)((l-mix)*wr);
        }
    }
}
