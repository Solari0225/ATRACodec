#include "independent_plus_math.h"
#include <stdint.h>
#include <string.h>
typedef struct pair {double hi,lo;} pair;
static pair norm(double hi,double lo){const double s=hi+lo;pair r={s,lo-(s-hi)};return r;}
static pair add(pair a,pair b){const double s=a.hi+b.hi,v=s-a.hi;return norm(s,((b.hi-v)+(a.hi-(s-v)))+a.lo+b.lo);}
static pair neg(pair a){a.hi=-a.hi;a.lo=-a.lo;return a;}
static pair mul(pair a,pair b){
    const double p=a.hi*b.hi,c1=134217729.0*a.hi,c2=134217729.0*b.hi;
    const double a1=c1-(c1-a.hi),a2=a.hi-a1,b1=c2-(c2-b.hi),b2=b.hi-b1;
    return norm(p,(((a1*b1-p)+a1*b2+a2*b1)+a2*b2)+(a.hi*b.lo+a.lo*b.hi));
}
static pair divp(pair a,pair b){const pair initial={a.hi/b.hi,0.};const pair rem=add(a,neg(mul(b,initial)));const pair correction={(rem.hi+rem.lo)/b.hi,0.};return add(initial,correction);}
static double value(uint64_t word){double result;memcpy(&result,&word,8);return result;}
double sony_plus_math_atan(double input){
    int negative=input<0.;if(negative)input=-input;
    pair x={input,0.},offset={0.,0.};const pair one={1.,0.};int inverse=0;
    const pair halfpi={0x1.921fb54442d18p0,0x1.1a62633145c07p-54};
    if(input>1.){x=divp(one,x);inverse=1;}
    if(x.hi>0x1.a827999fcef32p-2){x=divp(add(x,neg(one)),add(x,one));offset.hi=halfpi.hi*.5;offset.lo=halfpi.lo*.5;}
    const pair square=neg(mul(x,x));pair term=x,sum=x;
    for(int odd=3;odd<=101;odd+=2){term=mul(term,square);const pair denominator={(double)odd,0.};sum=add(sum,divp(term,denominator));}
    sum=add(sum,offset);if(inverse)sum=add(halfpi,neg(sum));
    volatile double result=sum.hi+sum.lo;return negative?-result:result;
}
double sony_plus_math_positive_ln(double input){
    uint64_t word;memcpy(&word,&input,8);
    if(word>>63||((word>>52)&2047)==2047)return value(UINT64_C(0x7ff8000000000000));
    if(!input)return value(UINT64_C(0xfff0000000000000));
    int exponent=(int)((word>>52)&2047)-1023;
    if(((word>>52)&2047)==0){input*=0x1p52;memcpy(&word,&input,8);exponent=(int)((word>>52)&2047)-1075;}
    word=(word&UINT64_C(0x000fffffffffffff))|UINT64_C(0x3ff0000000000000);double mantissa=value(word);
    if(mantissa>0x1.6a09e667f3bcdp0){mantissa*=.5;++exponent;}
    const pair m={mantissa,0.},one={1.,0.};const pair z=divp(add(m,neg(one)),add(m,one));
    const pair square=mul(z,z);pair term=z,sum=z;
    for(int odd=3;odd<=61;odd+=2){term=mul(term,square);const pair denominator={(double)odd,0.};sum=add(sum,divp(term,denominator));}
    const pair two={2.,0.},ln2={0x1.62e42fefa39efp-1,0x1.abc9e3b39803fp-56},e={(double)exponent,0.};
    const pair result=add(mul(sum,two),mul(ln2,e));volatile double rounded=result.hi+result.lo;return rounded;
}
