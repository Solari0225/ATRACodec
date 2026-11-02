#include "independent_plus_gain_log.h"
#include <stdint.h>
#include <string.h>
typedef struct pair {double hi,lo;} pair;
static pair renormalize(double hi,double lo){const double s=hi+lo;pair r={s,lo-(s-hi)};return r;}
static pair add(pair a,pair b){
    const double s=a.hi+b.hi,v=s-a.hi;
    const double tail=((b.hi-v)+(a.hi-(s-v)))+a.lo+b.lo;
    return renormalize(s,tail);
}
static pair negate(pair a){a.hi=-a.hi;a.lo=-a.lo;return a;}
static pair multiply(pair a,pair b){
    const double p=a.hi*b.hi;
    const double c1=134217729.0*a.hi,c2=134217729.0*b.hi;
    const double a1=c1-(c1-a.hi),a2=a.hi-a1,b1=c2-(c2-b.hi),b2=b.hi-b1;
    double error=((a1*b1-p)+a1*b2+a2*b1)+a2*b2;
    error+=(a.hi*b.lo+a.lo*b.hi);
    return renormalize(p,error);
}
static pair divide(pair a,pair b){
    const double q=a.hi/b.hi;const pair initial={q,0.0};
    const pair remainder=add(a,negate(multiply(b,initial)));
    const pair correction={(remainder.hi+remainder.lo)/b.hi,0.0};
    return add(initial,correction);
}
static double number(uint64_t bits){double value;memcpy(&value,&bits,8);return value;}
double sony_recovered_plus_gain_log_measure(double ratio){
    uint64_t word;memcpy(&word,&ratio,8);
    if(word>>63||((word>>52)&2047)==2047)return number(UINT64_C(0x7ff8000000000000));
    if(!ratio)return number(UINT64_C(0xfff0000000000000));
    int exponent=(int)((word>>52)&2047)-1023;
    if(((word>>52)&2047)==0){ratio*=0x1p52;memcpy(&word,&ratio,8);exponent=(int)((word>>52)&2047)-1075;}
    word=(word&UINT64_C(0x000fffffffffffff))|UINT64_C(0x3ff0000000000000);
    double mantissa=number(word);
    if(mantissa>0x1.6a09e667f3bcdp0){mantissa*=0.5;++exponent;}
    const pair m={mantissa,0.0},one={1.0,0.0};
    const pair z=divide(add(m,negate(one)),add(m,one));
    const pair squared=multiply(z,z);pair term=z,sum=z;
    for(int odd=3;odd<=61;odd+=2){
        term=multiply(term,squared);const pair denominator={(double)odd,0.0};
        sum=add(sum,divide(term,denominator));
    }
    const pair two={2.0,0.0},ln2={0x1.62e42fefa39efp-1,0x1.abc9e3b39803fp-56},e={(double)exponent,0.0};
    const pair logarithm=add(multiply(sum,two),multiply(ln2,e));
    volatile double rounded=logarithm.hi+logarithm.lo;
    volatile double result=rounded*number(UINT64_C(0x3ff7154760000000));
    return result;
}
