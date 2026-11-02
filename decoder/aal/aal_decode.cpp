#include "aal_decode.h"
#include "../../shared/aal_original/original_fixed_predictor.hpp"
#include "../../shared/aal_original/reused/compression/independent_lpc.h"
#include "../../shared/aalp_original/original_aalp_reader.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
thread_local std::string last_error;
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
struct Packet {const uint8_t *core,*enhancement;size_t enhancement_bytes;uint32_t ordinal;uint16_t tail;uint8_t padding;bool last;};
struct Stream {uint32_t format,bitrate,F,delay,core_bytes,cap;uint64_t frames;std::vector<Packet> packets;};
Stream parse(const uint8_t *data,size_t bytes){
    size_t offset=0;if(bytes>=10&&(!std::memcmp(data,"ea3",3)||!std::memcmp(data,"ID3",3))){for(unsigned i=6;i<10;++i){if(data[i]&128)throw std::runtime_error("Invalid metadata length");offset=(offset<<7)|data[i];}offset+=10+((data[5]&16)?10:0);}
    if(offset>bytes||bytes-offset<96||std::memcmp(data+offset,"EA3",3)||data[offset+4]||data[offset+5]!=96)throw std::runtime_error("Invalid OMA/EA3 header");
    const uint8_t *h=data+offset;if(be16(h+6)!=0xff80&&be16(h+6)!=0xffff)throw std::runtime_error("Encrypted OMA is outside decoder scope");
    Stream s{};const uint32_t parameter=(uint32_t(h[49])<<16)|be16(h+50);
    if(h[32]==0x22&&parameter==0x2030){s.format=3;s.bitrate=132000;s.F=1024;s.delay=138;s.core_bytes=384;if(h[33]!=0x52||h[48])throw std::runtime_error("Unsupported AAL3 configuration");}
    else if(h[32]==0x21){static const uint32_t sizes[9]={280,376,560,744,936,1120,1488,1864,2048},rates[9]={48000,64000,96000,128000,160000,192000,256000,320000,352000};
        s.format=4;s.F=2048;s.delay=368;s.core_bytes=((parameter&1023)+1)*8;for(unsigned i=0;i<9;++i)if(s.core_bytes==sizes[i]&&parameter==(0x2800u|(sizes[i]/8-1)))s.bitrate=rates[i];
        if(!s.bitrate||h[33]!=0x91||h[48]!=1)throw std::runtime_error("Unsupported AALP configuration");s.cap=s.core_bytes<=744?8:s.core_bytes<=1488?12:16;
    }else throw std::runtime_error("Expected default AAL3/132 or AALP");
    offset+=96;while(offset<bytes){if(bytes-offset<24||std::memcmp(data+offset,"BLK",3))throw std::runtime_error("Invalid BLK header");const uint8_t *p=data+offset;const uint32_t size=be16(p+4),flags=be16(p+12);if(size<s.core_bytes||size>bytes-offset-24||p[3]>7)throw std::runtime_error("Truncated AAL block");Packet q{p+24,p+24+s.core_bytes,size-s.core_bytes,be32(p+8),uint16_t(flags&0xfff),p[3],(flags&0x8000)!=0};
        if(q.ordinal!=s.packets.size()||!q.tail||q.tail>s.F||q.padding>q.enhancement_bytes||(!q.ordinal&&q.enhancement_bytes))throw std::runtime_error("Invalid BLK ordinal/tail/padding");s.packets.push_back(q);offset+=24+size;}
    if(s.packets.size()<2||s.packets.size()!=be32(h+40)||!s.packets.back().last)throw std::runtime_error("Missing AAL final block");for(size_t i=0;i+1<s.packets.size();++i)if(s.packets[i].last||s.packets[i].tail!=s.F)throw std::runtime_error("Premature AAL tail marker");
    const uint64_t adjusted=s.packets.size()-2;const uint32_t tail=s.packets.back().tail;const bool extra=tail>s.F-s.delay;if(extra&&adjusted==0)throw std::runtime_error("Inconsistent AAL sample count");s.frames=(adjusted-uint64_t(extra))*s.F+tail;
    if(!s.frames||s.frames>(std::numeric_limits<size_t>::max()/2)/sizeof(int16_t))throw std::runtime_error("Invalid decoded sample count");return s;
}
}
extern "C" int atracodec_aal_decode_oma(const uint8_t *oma,size_t bytes,atracodec_aal_pcm *out){
    last_error.clear();if(!out){last_error="AAL PCM output pointer is null";return -1;}std::memset(out,0,sizeof(*out));
    try{if(!oma||!bytes)throw std::runtime_error("OMA input is empty");const Stream stream=parse(oma,bytes);std::vector<int16_t> pcm(size_t(stream.frames)*2);uint64_t written=0;
        auto a3=std::make_unique<sony_aal_original::OriginalFixedPredictor>();auto plus=std::make_unique<sony_aalp_predictor_state>();sony_original_aalp_predictor_reset(plus.get());
        for(const auto &packet:stream.packets){std::array<std::array<int16_t,2048>,2> predicted{};
            if(stream.format==3){const auto values=a3->decode(packet.core);for(uint32_t i=0;i<1024;++i)for(uint32_t ch=0;ch<2;++ch)predicted[ch][i]=values[2*i+ch];}
            else{sony_aalp_core_fields fields;plus_core_frame(packet.core,stream.core_bytes,fields);if(sony_original_aalp_predict(&fields,stream.cap,plus.get(),reinterpret_cast<int16_t(*)[2048]>(predicted.data())))throw std::runtime_error("AALP fixed prediction failed");}
            if(!packet.ordinal)continue;if(!packet.enhancement_bytes)throw std::runtime_error("Missing enhancement for output AAL block");
            const auto errors=enhancement(packet.enhancement,packet.enhancement_bytes,packet.padding,stream.F);
            for(uint32_t j=0;j<stream.F;++j){const int64_t index=int64_t(packet.ordinal-1)*stream.F+j-stream.delay;if(index<0||uint64_t(index)>=stream.frames)continue;for(uint32_t ch=0;ch<2;++ch)pcm[size_t(index)*2+ch]=sample16(uint32_t(uint16_t(predicted[ch][j]))+uint32_t(errors[ch][j]));++written;}
        }
        if(written!=stream.frames)throw std::runtime_error("AAL reconstruction coverage mismatch");void *memory=std::malloc(pcm.size()*sizeof(int16_t));if(!memory)throw std::bad_alloc();std::memcpy(memory,pcm.data(),pcm.size()*sizeof(int16_t));out->samples=(int16_t*)memory;out->sample_frames=stream.frames;out->sample_rate=44100;out->channels=2;out->source_format=stream.format;out->core_bitrate=stream.bitrate;return 0;
    }catch(const std::exception &e){last_error=e.what();return -1;}catch(...){last_error="Unknown AAL decoding failure";return -1;}
}
extern "C" void atracodec_aal_free_pcm(atracodec_aal_pcm *out){if(out){std::free(out->samples);std::memset(out,0,sizeof(*out));}}
extern "C" const char *atracodec_aal_last_error(void){return last_error.c_str();}
