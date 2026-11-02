// Sony-derived frame mathematics copied without algorithm changes from the
// independently accepted0.4AAL decoder. The stream wrapper owns scheduling.
#ifndef ATRACODEC_STREAM_AAL_FRAME_HPP
#define ATRACODEC_STREAM_AAL_FRAME_HPP
#include "../shared/aal_original/original_fixed_predictor.hpp"
#include "../shared/aal_original/reused/compression/independent_lpc.h"
#include "../shared/aalp_original/original_aalp_reader.h"
#include <array>
#include <limits>
#include <string>
#include <vector>
namespace atracodec_stream_frame {
uint16_t be16(const uint8_t *p){return uint16_t((uint16_t(p[0])<<8)|p[1]);}
uint32_t be32(const uint8_t *p){return(uint32_t(be16(p))<<16)|be16(p+2);}
int16_t sample16(uint32_t value){const uint16_t u=uint16_t(value);return u<32768?int16_t(u):int16_t(int32_t(u)-65536);}
struct Bits {
    const uint8_t *data;size_t capacity,position=0;
    uint32_t get(unsigned n){if(n>32||position>capacity||n>capacity-position)throw std::runtime_error("Truncated AAL enhancement bit field");uint32_t value=0;while(n--){value=(value<<1)|((data[position>>3]>>(7-(position&7)))&1);++position;}return value;}
    // Native38d150 consumes alignment without testing its value.
    void align(){const unsigned n=unsigned((0-position)&7);(void)get(n);}
    void skip(size_t n){if(position>capacity||n>capacity-position)throw std::runtime_error("Truncated AAL core extension");position+=n;}
};
void plus_core_frame(const uint8_t *data,uint32_t bytes,sony_aalp_core_fields &fields){
    uint32_t positions[10];const int status=sony_original_aalp_read_core(data,bytes,&fields,positions);
    if((status&&status!=-10)||fields.channels!=2)throw std::runtime_error("AALP stereo audio unit parse failed at stage "+std::to_string(status));
    if(!status)return;
    // Native OMA delivery requires the first unit to be the default stereo
    // audio header. Leading type2 frames are rejected before37dd90 is called.
    // The immutable unit reader passed all fields at stage10; continue from
    // its exact end position to support native381560 trailing extensions.
    Bits bits{data,size_t(bytes)*8,positions[8]};
    for(;;){const unsigned type=bits.get(2);if(type==3)return;
        if(type!=2)throw std::runtime_error("Invalid default AALP audio unit layout");
        (void)bits.get(5);const uint32_t count=bits.get(11);if(count>2046)throw std::runtime_error("Invalid AALP extension byte count");bits.skip(size_t(count)*8);
    }
}
uint32_t quotient(Bits &bits){
    for(unsigned n=0;n<16;++n){const uint32_t value=bits.get(1);if(n==15){if(value)throw std::runtime_error("Invalid AAL Rice escape prefix");return bits.get(16);}if(value!=(n&1))return n;}
    throw std::runtime_error("Invalid Rice quotient");
}
void rice(Bits &bits,int32_t *output,size_t count){
    // Sony38cdd0 checks k<=declared PCM width; the default profiles use16.
    const unsigned k=bits.get(5);if(k>16)throw std::runtime_error("Invalid AAL Rice parameter for PCM16");
    std::vector<uint32_t> q(count);for(auto &v:q)v=quotient(bits);
    // Original38cdd0 reconstructs a full u32 folded value before its signed
    // unfold. Prediction/final PCM perform their own explicit width wrapping.
    for(size_t i=0;i<count;++i){const uint32_t u=(q[i]<<k)|bits.get(k);output[i]=(u&1)?-int32_t(u>>1)-1:int32_t(u>>1);}
}
void inverse_temporal(std::vector<int32_t> &samples,unsigned mode,const int32_t codes[8]){
    if(!mode)return;const int32_t history[8]={};std::vector<int32_t> output(samples.size());int status;
    if(mode<=7)status=aal_fixed_prediction(samples.data(),output.data(),history,mode,unsigned(samples.size()),16,1);
    else if(mode==8){int32_t reflection[8],direct[8];if(aal_lpc_reflection_from_codes(codes,reflection,8)||aal_lpc_direct_from_reflection(reflection,direct,8))throw std::runtime_error("Invalid AAL LPC coefficients");status=aal_lpc_inverse(samples.data(),output.data(),direct,history,8,unsigned(samples.size()),16);}
    else throw std::runtime_error("Invalid AAL temporal mode");
    if(status)throw std::runtime_error("AAL inverse prediction failed");samples.swap(output);
}
std::array<std::vector<int32_t>,2> enhancement(const uint8_t *data,size_t bytes,unsigned padding,uint32_t samples){
    if(padding>7||padding>bytes||bytes-padding>std::numeric_limits<size_t>::max()/8)throw std::runtime_error("Invalid enhancement size/padding");
    if(bytes-padding>size_t(samples)*4+8)throw std::runtime_error("Enhancement exceeds original PCM16 frame capacity");
    Bits bits{data,(bytes-padding)*8};
    // Native38c040 ignores the first flag and accepts either1or2 for this
    // two-bit syntax tag. Physical channel count comes from the OMA profile.
    (void)bits.get(1);const unsigned tag=bits.get(2);if(tag!=1&&tag!=2)throw std::runtime_error("Invalid AAL enhancement syntax tag");
    const unsigned raw=bits.get(2);std::array<std::vector<int32_t>,2> channels;for(auto &c:channels)c.resize(samples);
    if(raw==1){for(auto &c:channels)for(auto &v:c)v=sample16(bits.get(16));}
    else if(raw==0){const unsigned correlation=bits.get(2);if(correlation>2)throw std::runtime_error("Invalid AAL correlation mode");
        for(auto &c:channels){const unsigned mode=bits.get(4);int32_t codes[8]={};if(mode==8)for(unsigned i=0;i<8;++i)codes[i]=int32_t(bits.get(i<2?11:6));
            // Original38c040 uses field+1, and38c7c0 assigns floor(N/P) to
            // the first P-1 partitions and all remaining samples to the last.
            const unsigned partitions=bits.get(2)+1,normal=samples/partitions;unsigned at=0;
            for(unsigned p=0;p<partitions;++p){const unsigned n=p+1==partitions?samples-at:normal;rice(bits,c.data()+at,n);at+=n;}inverse_temporal(c,mode,codes);}
        for(uint32_t i=0;i<samples;++i){const int32_t a=channels[0][i],b=channels[1][i];if(correlation==1)channels[1][i]=sample16(uint32_t(a)-uint32_t(b));else if(correlation==2){channels[0][i]=sample16(uint32_t(a)+uint32_t(b));channels[1][i]=a;}}
    }else throw std::runtime_error("Invalid enhancement coding mode");
    if(bits.get(2))throw std::runtime_error("Nonzero enhancement end marker");bits.align();if(bits.position!=bits.capacity)throw std::runtime_error("Extra meaningful enhancement bytes");return channels;
}
}
#endif
