#include "sony_original_encoder.h"
#include "../shared/pcm132/recovered_atrac3_pcm132.h"
#include "../shared/joint66/recovered_atrac3_pcm66.h"
#include "../shared/aal_original/original_aal.h"
#include "../shared/aalp_original/original_aalp_assembly.h"
#include "../shared/plus_stream/recovered_plus_stream.h"
#include <algorithm>
#include <array>
#include <cfenv>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace {
using Bytes=std::vector<uint8_t>;
thread_local std::string last_error;
class FloatingEnvironment {
    std::fenv_t saved{};bool valid=false;
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    uint32_t saved_mxcsr=0;
#elif defined(__aarch64__)
    uint64_t saved_fpcr=0,saved_fpsr=0;
#endif
public:
    FloatingEnvironment(){
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        saved_mxcsr=_mm_getcsr();
#elif defined(__aarch64__)
        __asm__ volatile("mrs %0, fpcr" : "=r"(saved_fpcr));
        __asm__ volatile("mrs %0, fpsr" : "=r"(saved_fpsr));
#endif
        if(std::fegetenv(&saved))
            throw std::runtime_error("Cannot initialize IEEE floating-point environment");
        valid=true;
        if(std::fesetenv(FE_DFL_ENV)){
            std::fesetenv(&saved);valid=false;
            throw std::runtime_error("Cannot initialize IEEE floating-point environment");
        }
        /* Some Windows CRT fenv implementations cover x87 only. The core
         * also uses binary32 SIMD arithmetic and needs nearest-even, gradual
         * underflow and masked traps, independent of the embedding host. */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        _mm_setcsr(0x1f80u);
#elif defined(__aarch64__)
        const uint64_t zero=0;
        __asm__ volatile("msr fpcr, %0\n\tmsr fpsr, %0" : : "r"(zero) : "memory");
#endif
    }
    ~FloatingEnvironment(){
        if(!valid)return;
        std::fesetenv(&saved);
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        _mm_setcsr(saved_mxcsr);
#elif defined(__aarch64__)
        __asm__ volatile("msr fpcr, %0\n\tmsr fpsr, %1" : : "r"(saved_fpcr),"r"(saved_fpsr) : "memory");
#endif
    }
};
constexpr sony_original_profile profiles[]={
    {1,66000,192,1024,0,"atrac3-66"},{1,105000,304,1024,0,"atrac3-105"},
    {1,132000,384,1024,0,"atrac3-132"},
    {2,48000,280,2048,0,"atrac3plus-48"},{2,64000,376,2048,0,"atrac3plus-64"},
    {2,96000,560,2048,0,"atrac3plus-96"},{2,128000,744,2048,0,"atrac3plus-128"},
    {2,160000,936,2048,0,"atrac3plus-160"},{2,192000,1120,2048,0,"atrac3plus-192"},
    {2,256000,1488,2048,0,"atrac3plus-256"},{2,320000,1864,2048,0,"atrac3plus-320"},
    {2,352000,2048,2048,0,"atrac3plus-352"},
    {3,132000,384,1024,1,"aal3-132"},
    {4,48000,280,2048,1,"aalp-48"},{4,64000,376,2048,1,"aalp-64"},
    {4,96000,560,2048,1,"aalp-96"},{4,128000,744,2048,1,"aalp-128"},
    {4,160000,936,2048,1,"aalp-160"},{4,192000,1120,2048,1,"aalp-192"},
    {4,256000,1488,2048,1,"aalp-256"},{4,320000,1864,2048,1,"aalp-320"},
    {4,352000,2048,2048,1,"aalp-352"}
};
const sony_original_profile *find(uint32_t format,uint32_t rate){
    for(const auto &p:profiles)if(p.format==format&&p.nominal_bitrate==rate)return &p;
    return nullptr;
}
Bytes pure_header(const sony_original_profile &p){
    Bytes h(96);std::memcpy(h.data(),"EA3",3);h[3]=1;h[5]=96;h[6]=h[7]=0xff;
    uint32_t parameters;
    if(p.format==SONY_ORIGINAL_ATRAC3){
        h[32]=0;parameters=p.nominal_bitrate==66000?0x022018:
            p.nominal_bitrate==105000?0x002026:0x002030;
    }else{h[32]=1;parameters=0x002800|(p.core_frame_bytes/8-1);}
    h[33]=uint8_t(parameters>>16);h[34]=uint8_t(parameters>>8);h[35]=uint8_t(parameters);
    return h;
}
Bytes atrac3_cores(const int16_t *pcm,uint32_t frames,const sony_original_profile &p){
    const uint64_t blocks=(uint64_t(frames)+1023)/1024,expected=blocks+2;
    Bytes output;output.reserve(size_t(expected*p.core_frame_bytes));
    auto stereo=std::make_unique<sony_atrac3_independent_stereo_stream>();
    auto joint=std::make_unique<sony_atrac3_pcm66_stream>();
    if(p.nominal_bitrate==66000)sony_recovered_atrac3_pcm66_stream_init(joint.get());
    else if(sony_recovered_atrac3_stereo_stream_init(stereo.get(),p.nominal_bitrate))
        throw std::runtime_error("ATRAC3 factory failed");
    std::array<uint8_t,387> core{};uint32_t end_bits[2];
    auto save=[&](int status){
        if(status<0)throw std::runtime_error("ATRAC3 PCM encoding failed at block "+std::to_string(output.size()/p.core_frame_bytes));
        if(status)output.insert(output.end(),core.data(),core.data()+p.core_frame_bytes);
    };
    for(uint64_t block=0;block<blocks;++block){
        std::array<int16_t,2048> input{};
        const uint64_t offset=block*1024;
        const size_t count=size_t(std::min<uint64_t>(1024,uint64_t(frames)-offset));
        std::copy_n(pcm+size_t(offset)*2,count*2,input.data());
        save(p.nominal_bitrate==66000?
            sony_recovered_atrac3_pcm66_stream_block(joint.get(),input.data(),core.data(),end_bits):
            sony_recovered_atrac3_stereo_stream_block(stereo.get(),input.data(),core.data(),uint32_t(core.size()),end_bits));
    }
    for(unsigned j=0;j<3;++j)save(p.nominal_bitrate==66000?
        sony_recovered_atrac3_pcm66_stream_finish(joint.get(),core.data(),end_bits):
        sony_recovered_atrac3_stereo_stream_finish(stereo.get(),core.data(),uint32_t(core.size()),end_bits));
    if(output.size()!=expected*p.core_frame_bytes)throw std::runtime_error("ATRAC3 stream length mismatch");
    return output;
}
Bytes encode(const int16_t *pcm,uint32_t frames,const sony_original_profile &p){
    if(!pcm||!frames)throw std::invalid_argument("PCM must contain signed16 stereo44100Hz sample pairs");
    if(p.format==SONY_ORIGINAL_AAL3){
        uint8_t *data=nullptr;size_t size=0;
        if(sony_original_aal3_encode(pcm,frames,&data,&size))
            throw std::runtime_error(sony_original_aal_last_error());
        std::unique_ptr<uint8_t,decltype(&sony_original_aal_free)> owner(data,sony_original_aal_free);
        return Bytes(data,data+size);
    }
    if(p.format==SONY_ORIGINAL_ATRAC3){
        Bytes result=pure_header(p),core=atrac3_cores(pcm,frames,p);
        result.insert(result.end(),core.begin(),core.end());return result;
    }
    uint8_t *data=nullptr;size_t size=0;uint32_t count=0;
    const uint32_t pad_last=p.format==SONY_ORIGINAL_ATRAC3PLUS?1:0;
    if(sony_recovered_plus_encode_cores(pcm,frames,p.nominal_bitrate,pad_last,&data,&size,&count))
        throw std::runtime_error("ATRAC3plus PCM encoding failed");
    std::unique_ptr<uint8_t,decltype(&sony_recovered_plus_free_cores)> owner(data,sony_recovered_plus_free_cores);
    if(size!=uint64_t(count)*p.core_frame_bytes)throw std::runtime_error("ATRAC3plus stream size mismatch");
    if(p.format==SONY_ORIGINAL_ATRAC3PLUS){
        Bytes result=pure_header(p);result.insert(result.end(),data,data+size);return result;
    }
    uint8_t *aal=nullptr;size_t aal_bytes=0;
    if(sony_original_aalp_assemble(pcm,frames,data,count,p.core_frame_bytes,&aal,&aal_bytes))
        throw std::runtime_error(sony_original_aalp_last_error());
    std::unique_ptr<uint8_t,decltype(&sony_original_aalp_free)> aal_owner(aal,sony_original_aalp_free);
    return Bytes(aal,aal+aal_bytes);
}
}
extern "C" const char *sony_original_encoder_version(void){return "0.3.0-original-test1";}
extern "C" uint32_t sony_original_encoder_profile_count(void){return uint32_t(sizeof(profiles)/sizeof(profiles[0]));}
extern "C" int sony_original_encoder_profile_at(uint32_t index,sony_original_profile *out){
    if(!out||index>=sony_original_encoder_profile_count())return -1;*out=profiles[index];return 0;
}
extern "C" int sony_original_encoder_encode_pcm16(const int16_t *pcm,uint32_t frames,
    uint32_t format,uint32_t rate,uint8_t **output,size_t *bytes){
    if(!output||!bytes)return -1;*output=nullptr;*bytes=0;last_error.clear();
    try{
        const auto *profile=find(format,rate);
        if(!profile)throw std::invalid_argument("Unsupported format and bitrate; no fallback is used");
        const FloatingEnvironment floating_environment;
        const Bytes data=encode(pcm,frames,*profile);
        auto *p=static_cast<uint8_t*>(std::malloc(data.size()));if(!p)throw std::bad_alloc();
        std::memcpy(p,data.data(),data.size());*output=p;*bytes=data.size();return 0;
    }catch(const std::exception &e){last_error=e.what();return -1;}
    catch(...){last_error="Unknown encoding failure";return -1;}
}
extern "C" void sony_original_encoder_free(void *p){std::free(p);}
extern "C" const char *sony_original_encoder_last_error(void){return last_error.c_str();}
