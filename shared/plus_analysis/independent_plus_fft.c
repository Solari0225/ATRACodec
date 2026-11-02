#include "independent_plus_fft.h"
#include <string.h>
#include "fft_tables.inc"
/* The original x87 uses53bit precision with explicit binary32 spill points.
 * Double expressions keep register values; float local temporaries below mark
 * the actual original spills. Contraction must be disabled when compiling. */
static float spill(double x){volatile float y=(float)x;return y;}
static float number(uint32_t x){float f;memcpy(&f,&x,4);return f;}
static void swap_complex(float *a,uint32_t i,uint32_t j){
    const float r=a[i],v=a[i+1];a[i]=a[j];a[i+1]=a[j+1];a[j]=r;a[j+1]=v;
}
static void bit_reverse(float *a,uint32_t n){
    uint32_t j=0;const uint32_t count=n/2;
    for(uint32_t i=0;i<count;++i){
        if(i<j)swap_complex(a,2*i,2*j);
        uint32_t bit=count/2;while(bit&&(j&bit)){j^=bit;bit/=2;}j^=bit;
    }
}
/* Four radix outputs. Modes0and1have no general rotations; mode1is the
 * fixed pi/4 branch. Modes2and3rotate the regular and conjugate blocks. */
static void radix4(float *a,uint32_t j,uint32_t l,uint32_t mode,
    double w1r,double w1i,double w2r,double w2i,double w3r,double w3i){
    const uint32_t k=j+l,h=k+l,t=h+l;
    const double x0r=(double)a[j]+a[k],x0i=(double)a[j+1]+a[k+1];
    const float x1r=spill((double)a[j]-a[k]),x1i=spill((double)a[j+1]-a[k+1]);
    double x2r=(double)a[h]+a[t],x2i=(double)a[h+1]+a[t+1];
    if(mode==1)x2r=spill(x2r);else x2i=spill(x2i);
    const float x3r=spill((double)a[h]-a[t]),x3i=spill((double)a[h+1]-a[t+1]);
    a[j]=spill(x0r+x2r);a[j+1]=spill(x0i+x2i);
    const float y2r=spill(x0r-x2r);const double y2i=x0i-x2i;
    const double y1r=(double)x1r-x3i,y1i=(double)x3r+x1i;
    const double y3r=(double)x3i+x1r,y3i=(double)x1i-x3r;
    if(mode==0){
        a[h]=y2r;a[h+1]=spill(y2i);
        a[k]=spill(y1r);a[k+1]=spill(y1i);
        a[t]=spill(y3r);a[t+1]=spill(y3i);
    }else if(mode==1){
        a[h]=spill(x2i-x0i);a[h+1]=spill(x0r-x2r);
        a[k]=spill(w1r*(y1r-y1i));a[k+1]=spill(w1r*(y1r+y1i));
        a[t]=spill(w1r*(-y3i-y3r));a[t+1]=spill(w1r*(y3r-y3i));
    }else{
        if(mode==2){a[h]=spill(w2r*y2r-w2i*y2i);a[h+1]=spill(w2r*y2i+w2i*y2r);}
        else{a[h]=spill(-w2i*y2r-w2r*y2i);a[h+1]=spill(w2r*y2r-w2i*y2i);}
        a[k]=spill(w1r*y1r-w1i*y1i);a[k+1]=spill(w1i*y1r+w1r*y1i);
        a[t]=spill(w3r*y3r-w3i*y3i);a[t+1]=spill(w3r*y3i+w3i*y3r);
    }
}
static void first_stage(float *a,uint32_t n,const uint32_t *w){
    radix4(a,0,2,0,0,0,0,0,0,0);
    radix4(a,8,2,1,number(w[2]),0,0,0,0,0);
    uint32_t k1=0;
    for(uint32_t j=16;j<n;j+=16){
        const double w2r=number(w[k1+2]),w2i=number(w[k1+3]);k1+=2;
        double w1r=number(w[2*k1]),w1i=number(w[2*k1+1]);
        float w3r=spill(w1r-2.0*w2i*w1i),w3i=spill(2.0*w2i*w1r-w1i);
        radix4(a,j,2,2,w1r,w1i,w2r,w2i,w3r,w3i);
        w1r=number(w[2*k1+2]);w1i=number(w[2*k1+3]);
        w3r=spill(w1r-2.0*w2r*w1i);w3i=spill(2.0*w2r*w1r-w1i);
        radix4(a,j+8,2,3,w1r,w1i,w2r,w2i,w3r,w3i);
    }
}
static void middle_stage(float *a,uint32_t n,uint32_t l,const uint32_t *w){
    const uint32_t m=l*4;
    for(uint32_t j=0;j<l;j+=2)radix4(a,j,l,0,0,0,0,0,0,0);
    for(uint32_t j=m;j<m+l;j+=2)radix4(a,j,l,1,number(w[2]),0,0,0,0,0);
    uint32_t k1=0;
    for(uint32_t k=2*m;k<n;k+=2*m){
        k1+=2;const double w2r=number(w[k1]),w2i=number(w[k1+1]);
        double w1r=number(w[2*k1]),w1i=number(w[2*k1+1]);
        float w3r=spill(w1r-2.0*w2i*w1i),w3i=spill(2.0*w2i*w1r-w1i);
        for(uint32_t j=k;j<k+l;j+=2)radix4(a,j,l,2,w1r,w1i,w2r,w2i,w3r,w3i);
        w1r=number(w[2*k1+2]);w1i=number(w[2*k1+3]);
        w3r=spill(w1r-2.0*w2r*w1i);w3i=spill(2.0*w2r*w1r-w1i);
        for(uint32_t j=k+m;j<k+m+l;j+=2)radix4(a,j,l,3,w1r,w1i,w2r,w2i,w3r,w3i);
    }
}
int sony_recovered_plus_real_fft(float *a,uint32_t n){
    if(!a)return -1;const uint32_t *w;
    switch(n){case 16:w=fft_twiddle16;break;case 32:w=fft_twiddle32;break;
        case 64:w=fft_twiddle64;break;case 128:w=fft_twiddle128;break;
        case 256:w=fft_twiddle256;break;default:return -1;}
    bit_reverse(a,n);first_stage(a,n,w);
    uint32_t l=8;while(l*4<n){middle_stage(a,n,l,w);l*=4;}
    if(l*4==n){for(uint32_t j=0;j<l;j+=2)radix4(a,j,l,0,0,0,0,0,0,0);}
    else for(uint32_t j=0;j<l;j+=2){
        const float r=spill((double)a[j]-a[j+l]),i=spill((double)a[j+1]-a[j+l+1]);
        a[j]=spill((double)a[j]+a[j+l]);a[j+1]=spill((double)a[j+l+1]+a[j+1]);
        a[j+l]=r;a[j+l+1]=i;
    }
    for(uint32_t j=2;j<n/2;j+=2){
        const uint32_t k=n-j;
        const double wr=0.5-number(w[n/2-j/2]),wi=number(w[n/4+j/2]);
        const double xr=(double)a[j]-a[k],xi=(double)a[k+1]+a[j+1];
        const float yr=spill(wr*xr-wi*xi);const double yi=wr*xi+wi*xr;
        a[j]=spill((double)a[j]-yr);a[j+1]=spill((double)a[j+1]-yi);
        a[k]=spill((double)a[k]+yr);a[k+1]=spill((double)a[k+1]-yi);
    }
    const float nyquist=spill((double)a[0]-a[1]);a[0]=spill((double)a[1]+a[0]);a[1]=nyquist;
    return 0;
}
int sony_recovered_plus_power_spectrum(const float *input,uint32_t stride,uint32_t n,float *power){
    if(!input||!power||!stride||n>256)return -1;
    float a[256];for(uint32_t i=0;i<n;++i)a[i]=input[i*stride];
    if(sony_recovered_plus_real_fft(a,n))return -1;
    const double nyquist=a[1];a[1]=0.0f;
    for(uint32_t i=0;i<n/2;++i){const double r=a[2*i],v=a[2*i+1];power[i]=spill(r*r+v*v);}
    power[n/2]=spill(nyquist*nyquist);return 0;
}
