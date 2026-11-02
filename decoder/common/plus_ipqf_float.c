/* Independent ordinarySony ATRAC3plus float synthesis.
 * Factorized16-point transform followed by twelve ordered synthesis sections.
 * Binary32 SIMD operations and binary64 scalar intermediates remain explicit.
 * Do not reassociate or enable FMA contraction. No original machine code,
 * component loading or CPU emulation is used in this implementation.
 */
#include "plus_ipqf_float.h"
#include <string.h>
#include <stddef.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24 && FLT_MAX_EXP==128,"IEEEbinary32 required");
_Static_assert(sizeof(double)==8 && DBL_MANT_DIG==53,"IEEEbinary64 required");
_Static_assert(offsetof(atracodec_plus_ipqf_float_state,ring)==0x900,"Synthesis state layout");
typedef struct quad {float v[4];} quad;
static quad load4(const float *p){quad x;memcpy(&x,p,sizeof(x));return x;}
static void store4(float *p,quad x){memcpy(p,&x,sizeof(x));}
static quad add4(quad a,quad b){quad c;for(unsigned i=0;i<4;++i)c.v[i]=a.v[i]+b.v[i];return c;}
static quad sub4(quad a,quad b){quad c;for(unsigned i=0;i<4;++i)c.v[i]=a.v[i]-b.v[i];return c;}
static quad mul4(quad a,quad b){quad c;for(unsigned i=0;i<4;++i)c.v[i]=a.v[i]*b.v[i];return c;}
static quad shuffle4(quad a,quad b,unsigned m){quad c={{a.v[m&3],a.v[(m>>2)&3],b.v[(m>>4)&3],b.v[(m>>6)&3]}};return c;}
static quad reverse4(quad a){return shuffle4(a,a,27);}
#include "plus_ipqf_float_tables.inc"
static void inverse16(const float in[16],float out[16],const float T[36],double norm){
    float w[92];memcpy(w+76,in,64);
    quad xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6;
    xmm4 = load4(w + 76); /* Sony +0x40dd27 */
    xmm5 = load4(w + 84); /* Sony +0x40dd36 */
    xmm1 = xmm4; /* Sony +0x40dd3e */
    xmm0 = load4(w + 80); /* Sony +0x40dd41 */
    xmm0 = shuffle4(xmm0, xmm0, 27); /* Sony +0x40dd4b */
    xmm2 = xmm0; /* Sony +0x40dd4f */
    xmm3 = load4(w + 88); /* Sony +0x40dd52 */
    xmm2 = add4(xmm2, xmm5); /* Sony +0x40dd55 */
    xmm3 = shuffle4(xmm3, xmm3, 27); /* Sony +0x40dd58 */
    xmm1 = add4(xmm1, xmm3); /* Sony +0x40dd5c */
    xmm6 = xmm1; /* Sony +0x40dd5f */
    xmm1 = sub4(xmm1, xmm2); /* Sony +0x40dd62 */
    xmm6 = add4(xmm6, xmm2); /* Sony +0x40dd65 */
    store4(w + 12, xmm6); /* Sony +0x40dd68 */
    const double d0 = (double)w[15]; /* Sony +0x40dd6d */
    const double d1 = d0 + (double)w[14]; /* Sony +0x40dd71 */
    xmm2 = xmm1; /* Sony +0x40dd75 */
    xmm1 = load4(T + 25); /* Sony +0x40dd78 */
    xmm6 = xmm1; /* Sony +0x40dd7f */
    const double d2 = d1 + (double)w[13]; /* Sony +0x40dd87 */
    xmm6 = mul4(xmm6, xmm2); /* Sony +0x40dd8b */
    store4(w + 16, xmm6); /* Sony +0x40dd8e */
    xmm0 = sub4(xmm0, xmm5); /* Sony +0x40dd93 */
    xmm2 = xmm0; /* Sony +0x40dd96 */
    const double d3 = d2 + (double)w[12]; /* Sony +0x40dd99 */
    xmm0 = load4(T + 21); /* Sony +0x40dd9d */
    xmm4 = sub4(xmm4, xmm3); /* Sony +0x40dda0 */
    xmm0 = shuffle4(xmm0, xmm0, 27); /* Sony +0x40dda3 */
    const double d4 = d3 * (double)norm; /* Sony +0x40dda7 */
    xmm0 = mul4(xmm0, xmm2); /* Sony +0x40ddad */
    xmm2 = xmm0; /* Sony +0x40ddb0 */
    xmm0 = load4(T + 17); /* Sony +0x40ddb3 */
    xmm0 = mul4(xmm0, xmm4); /* Sony +0x40ddba */
    xmm3 = xmm0; /* Sony +0x40ddbd */
    w[32] = (float)d4; /* Sony +0x40ddc0 */
    const double d5 = (double)w[12]; /* Sony +0x40ddc7 */
    const double d6 = d5 - (double)w[13]; /* Sony +0x40ddcb */
    xmm0 = sub4(xmm0, xmm2); /* Sony +0x40ddcf */
    xmm3 = add4(xmm3, xmm2); /* Sony +0x40ddd2 */
    xmm1 = mul4(xmm1, xmm0); /* Sony +0x40ddd5 */
    store4(w + 20, xmm3); /* Sony +0x40ddd8 */
    const double d7 = d6 - (double)w[14]; /* Sony +0x40dddd */
    store4(w + 24, xmm1); /* Sony +0x40dde1 */
    const double d8 = d7 + (double)w[15]; /* Sony +0x40dde6 */
    const double d9 = d8 * (double)T[33]; /* Sony +0x40ddea */
    const double d10 = (double)w[12]; /* Sony +0x40ddf0 */
    const double d11 = d10 - (double)w[15]; /* Sony +0x40ddf4 */
    const double d12 = d11 * (double)T[29]; /* Sony +0x40ddf8 */
    const double d13 = (double)w[13]; /* Sony +0x40ddfe */
    const double d14 = d13 - (double)w[14]; /* Sony +0x40de02 */
    const double d15 = d14 * (double)T[30]; /* Sony +0x40de06 */
    const double d16 = (double)T[34]; /* Sony +0x40de0c */
    const double d17 = d16 * d12; /* Sony +0x40de12 */
    const double d18 = (double)T[35]; /* Sony +0x40de14 */
    const double d19 = d18 * d15; /* Sony +0x40de1a */
    const double d20 = d17 - d19; /* Sony +0x40de1c */
    w[72] = (float)d20; /* Sony +0x40de1e */
    const double d21 = (double)w[16]; /* Sony +0x40de25 */
    const double d22 = d21 - (double)w[19]; /* Sony +0x40de29 */
    const double d23 = d22 * (double)T[31]; /* Sony +0x40de2d */
    const double d24 = (double)w[17]; /* Sony +0x40de33 */
    const double d25 = d24 - (double)w[18]; /* Sony +0x40de37 */
    const double d26 = d25 * (double)T[32]; /* Sony +0x40de3b */
    const double d27 = (double)w[18]; /* Sony +0x40de41 */
    const double d28 = d27 + (double)w[17]; /* Sony +0x40de45 */
    const double d29 = d28 + (double)w[19]; /* Sony +0x40de49 */
    const double d30 = d29 + (double)w[16]; /* Sony +0x40de4d */
    const double d31 = d30 * (double)norm; /* Sony +0x40de51 */
    w[62] = (float)d31; /* Sony +0x40de57 */
    const double d32 = d26; /* Sony +0x40de5e */
    const double d33 = d32 + d23; /* Sony +0x40de60 */
    const double d34 = d33 * (double)norm; /* Sony +0x40de62 */
    const double d35 = d34 - (double)w[62]; /* Sony +0x40de68 */
    w[66] = (float)d35; /* Sony +0x40de6f */
    const double d36 = (double)w[16]; /* Sony +0x40de76 */
    const double d37 = d36 - (double)w[17]; /* Sony +0x40de7a */
    const double d38 = d37 - (double)w[18]; /* Sony +0x40de7e */
    const double d39 = d38 + (double)w[19]; /* Sony +0x40de82 */
    const double d40 = d39 * (double)T[33]; /* Sony +0x40de86 */
    const double d41 = d40 - (double)w[66]; /* Sony +0x40de8c */
    w[70] = (float)d41; /* Sony +0x40de93 */
    const double d42 = (double)T[34]; /* Sony +0x40de9a */
    const double d43 = d42 * d23; /* Sony +0x40dea0 */
    const double d44 = d26 * (double)T[35]; /* Sony +0x40dea4 */
    const double d45 = d43 - d44; /* Sony +0x40deaa */
    const double d46 = d45 - (double)w[70]; /* Sony +0x40deac */
    w[74] = (float)d46; /* Sony +0x40deb3 */
    const double d47 = (double)w[23]; /* Sony +0x40debc */
    const double d48 = d47 + (double)w[22]; /* Sony +0x40dec0 */
    const double d49 = d48 + (double)w[21]; /* Sony +0x40dec4 */
    const double d50 = d49 + (double)w[20]; /* Sony +0x40dec8 */
    const double d51 = d50 * (double)norm; /* Sony +0x40decc */
    const double d52 = (double)w[32]; /* Sony +0x40ded2 */
    const double d53 = d51 - d52; /* Sony +0x40ded9 */
    w[33] = (float)d53; /* Sony +0x40dedb */
    const double d54 = (double)w[62]; /* Sony +0x40dee2 */
    const double d55 = d54 - (double)w[33]; /* Sony +0x40dee9 */
    w[34] = (float)d55; /* Sony +0x40def0 */
    const double d56 = (double)w[20]; /* Sony +0x40def7 */
    const double d57 = d56 - (double)w[21]; /* Sony +0x40defb */
    const double d58 = d57 - (double)w[22]; /* Sony +0x40deff */
    const double d59 = d58 + (double)w[23]; /* Sony +0x40df03 */
    const double d60 = d59 * (double)T[33]; /* Sony +0x40df07 */
    w[56] = (float)d60; /* Sony +0x40df0d */
    const double d61 = (double)w[20]; /* Sony +0x40df14 */
    const double d62 = d61 - (double)w[23]; /* Sony +0x40df18 */
    const double d63 = d62 * (double)T[29]; /* Sony +0x40df1c */
    const double d64 = (double)w[21]; /* Sony +0x40df22 */
    const double d65 = d64 - (double)w[22]; /* Sony +0x40df26 */
    const double d66 = d65 * (double)T[30]; /* Sony +0x40df2a */
    const double d67 = d63; /* Sony +0x40df30 */
    const double d68 = d67 * (double)T[34]; /* Sony +0x40df32 */
    const double d69 = d66; /* Sony +0x40df38 */
    const double d70 = d69 * (double)T[35]; /* Sony +0x40df3a */
    const double d71 = d68 - d70; /* Sony +0x40df40 */
    w[58] = (float)d71; /* Sony +0x40df42 */
    const double d72 = (double)w[27]; /* Sony +0x40df49 */
    const double d73 = d72 + (double)w[26]; /* Sony +0x40df4d */
    const double d74 = d73 + (double)w[25]; /* Sony +0x40df51 */
    const double d75 = d74 + (double)w[24]; /* Sony +0x40df55 */
    const double d76 = d75 * (double)norm; /* Sony +0x40df59 */
    const double d77 = d76 - d51; /* Sony +0x40df5f */
    const double d78 = (double)w[34]; /* Sony +0x40df61 */
    const double d79 = d77 - d78; /* Sony +0x40df68 */
    w[35] = (float)d79; /* Sony +0x40df6a */
    const double d80 = d66 + d63; /* Sony +0x40df73 */
    const double d81 = d80 * (double)norm; /* Sony +0x40df75 */
    const double d82 = d81 - d77; /* Sony +0x40df7b */
    w[65] = (float)d82; /* Sony +0x40df7d */
    const double d83 = d15 + d12; /* Sony +0x40df8a */
    const double d84 = d83 * (double)norm; /* Sony +0x40df8c */
    const double d85 = d84 - (double)w[35]; /* Sony +0x40df92 */
    w[36] = (float)d85; /* Sony +0x40df99 */
    const double d86 = (double)w[24]; /* Sony +0x40dfa2 */
    const double d87 = d86 - (double)w[27]; /* Sony +0x40dfa6 */
    const double d88 = d87 * (double)T[31]; /* Sony +0x40dfaa */
    const double d89 = (double)w[65]; /* Sony +0x40dfb0 */
    const double d90 = d89 - (double)w[36]; /* Sony +0x40dfb7 */
    w[37] = (float)d90; /* Sony +0x40dfcc */
    const double d91 = (double)w[25]; /* Sony +0x40dfd3 */
    const double d92 = d91 - (double)w[26]; /* Sony +0x40dfd7 */
    const double d93 = d92 * (double)T[32]; /* Sony +0x40dfdb */
    const double d94 = (double)w[66]; /* Sony +0x40dfe1 */
    const double d95 = d94 - (double)w[37]; /* Sony +0x40dfe8 */
    w[38] = (float)d95; /* Sony +0x40dfef */
    const double d96 = d93; /* Sony +0x40dff6 */
    const double d97 = d96 + d88; /* Sony +0x40dff8 */
    const double d98 = d97 - (double)w[24]; /* Sony +0x40dffa */
    const double d99 = d98 - (double)w[25]; /* Sony +0x40dffe */
    const double d100 = d99 - (double)w[26]; /* Sony +0x40e002 */
    const double d101 = d100 - (double)w[27]; /* Sony +0x40e006 */
    const double d102 = d101 * (double)norm; /* Sony +0x40e00a */
    w[55] = (float)d102; /* Sony +0x40e010 */
    const double d103 = (double)w[24]; /* Sony +0x40e017 */
    const double d104 = d103 - (double)w[25]; /* Sony +0x40e01b */
    const double d105 = d104 - (double)w[26]; /* Sony +0x40e01f */
    const double d106 = d105 + (double)w[27]; /* Sony +0x40e023 */
    const double d107 = d106 * (double)T[33]; /* Sony +0x40e027 */
    const double d108 = d107 - (double)w[55]; /* Sony +0x40e02d */
    w[57] = (float)d108; /* Sony +0x40e034 */
    const double d109 = d88 * (double)T[34]; /* Sony +0x40e03d */
    const double d110 = d93 * (double)T[35]; /* Sony +0x40e045 */
    const double d111 = d109 - d110; /* Sony +0x40e04b */
    const double d112 = d111 - (double)w[57]; /* Sony +0x40e04d */
    const double d113 = (double)w[55]; /* Sony +0x40e054 */
    const double d114 = d113 - (double)w[65]; /* Sony +0x40e05b */
    const double d115 = (double)w[38]; /* Sony +0x40e062 */
    const double d116 = d114 - d115; /* Sony +0x40e069 */
    w[39] = (float)d116; /* Sony +0x40e06b */
    const double d117 = d9 - (double)w[39]; /* Sony +0x40e074 */
    w[40] = (float)d117; /* Sony +0x40e07b */
    const double d118 = (double)w[56]; /* Sony +0x40e082 */
    const double d119 = d118 - d114; /* Sony +0x40e089 */
    const double d120 = (double)w[40]; /* Sony +0x40e08b */
    const double d121 = d119 - d120; /* Sony +0x40e092 */
    w[41] = (float)d121; /* Sony +0x40e094 */
    const double d122 = (double)w[70]; /* Sony +0x40e09b */
    const double d123 = d122 - (double)w[41]; /* Sony +0x40e0a2 */
    w[42] = (float)d123; /* Sony +0x40e0a9 */
    const double d124 = (double)w[57]; /* Sony +0x40e0b0 */
    const double d125 = d124 - d119; /* Sony +0x40e0b7 */
    const double d126 = (double)w[42]; /* Sony +0x40e0b9 */
    const double d127 = d125 - d126; /* Sony +0x40e0c0 */
    w[43] = (float)d127; /* Sony +0x40e0c2 */
    const double d128 = (double)w[72]; /* Sony +0x40e0c9 */
    const double d129 = d128 - (double)w[43]; /* Sony +0x40e0d0 */
    w[44] = (float)d129; /* Sony +0x40e0d7 */
    const double d130 = (double)w[58]; /* Sony +0x40e0de */
    const double d131 = d130 - d125; /* Sony +0x40e0e5 */
    const double d132 = (double)w[44]; /* Sony +0x40e0e7 */
    const double d133 = d131 - d132; /* Sony +0x40e0ee */
    w[45] = (float)d133; /* Sony +0x40e0f0 */
    const double d134 = (double)w[74]; /* Sony +0x40e0f7 */
    const double d135 = d134 - (double)w[45]; /* Sony +0x40e0fe */
    w[46] = (float)d135; /* Sony +0x40e105 */
    const double d136 = d112 - d131; /* Sony +0x40e10c */
    const double d137 = d136 - (double)w[46]; /* Sony +0x40e10e */
    w[47] = (float)d137; /* Sony +0x40e115 */
    memcpy(out,w+32,64);
}
int atracodec_plus_ipqf_float(const float *const bands[16],float *output,
    atracodec_plus_ipqf_float_state *state){
    if(!bands||!output||!state||state->ring<0||state->ring>3)return -1;
    for(unsigned i=0;i<16;++i)if(!bands[i])return -1;
    float T[36],C[192],norm32;memcpy(T,transform_bits,sizeof(T));
    memcpy(C,synthesis_bits,sizeof(C));memcpy(&norm32,&normalization_bits,4);
    const double norm=norm32;float *d=state->delay;int phase=0;
    memset(output,0,2048*sizeof(float));
    for(unsigned time=0;time<128;++time){
        phase=(state->ring+(int)time)%3;
        float scaled[16],transformed[16];
        for(unsigned i=0;i<16;++i)scaled[i]=(float)((double)bands[i][time]*(double)T[i+1]);
        inverse16(scaled,transformed,T,norm);
        const int i0=phase_indices[0][phase],i1=phase_indices[1][phase];
        const int i2=phase_indices[2][phase],i3=phase_indices[3][phase];
        const int i4=phase_indices[4][phase],i5=phase_indices[5][phase],i6=phase_indices[6][phase];
        for(unsigned i=0;i<16;++i)d[i5-15+(int)i]=transformed[15-i];
        store4(d+i0+48,add4(mul4(load4(C),load4(d+i1)),mul4(load4(C+96),reverse4(load4(d+i6+4)))));
        store4(d+i1+48,sub4(mul4(reverse4(load4(C+100)),reverse4(load4(d+i5-3))),mul4(reverse4(load4(C+4)),load4(d+i6))));
        store4(d+i0+52,add4(mul4(load4(C+4),load4(d+i1+4)),mul4(load4(C+100),reverse4(load4(d+i6)))));
        store4(d+i1+52,sub4(mul4(reverse4(load4(C+96)),reverse4(load4(d+i5-7))),mul4(reverse4(load4(C)),load4(d+i6+4))));
        for(int j=0;j<10;++j){
            const int current=(j+2)*48,previous=(j+1)*48,older=j*48;
            const float *c=C+104+j*8;
            store4(d+i0+current,add4(load4(d+i0+previous),mul4(load4(c),reverse4(load4(d+i4+older+45)))));
            store4(d+i1+current,sub4(mul4(reverse4(load4(c+4)),reverse4(load4(d+i3+older+45))),load4(d+i2+previous)));
            store4(d+i0+current+4,add4(load4(d+i0+previous+4),mul4(load4(c+4),reverse4(load4(d+i4+older+41)))));
            store4(d+i1+current+4,sub4(mul4(reverse4(load4(c)),reverse4(load4(d+i3+older+41))),load4(d+i2+previous+4)));
        }
        float *out=output+time*16;
        store4(out,add4(load4(d+i0+528),mul4(load4(C+184),reverse4(load4(d+i4+525)))));
        store4(out+8,sub4(mul4(reverse4(load4(C+188)),reverse4(load4(d+i3+525))),load4(d+i2+528)));
        store4(out+4,add4(load4(d+i0+532),mul4(load4(C+188),reverse4(load4(d+i4+521)))));
        store4(out+12,sub4(mul4(reverse4(load4(C+184)),reverse4(load4(d+i3+521))),load4(d+i2+532)));
    }
    state->ring=phase+1;return 0;
}
