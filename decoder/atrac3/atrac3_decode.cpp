#include "atracodec_atrac3_decode.h"
#include "atrac3_core.h"
#include "../../product/atracodec_fenv.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
thread_local std::string error;
struct Layout {size_t audio,frame_bytes;uint32_t bitrate;};
Layout layout(const uint8_t*data,size_t bytes) {
    if(!data||bytes<96)throw std::runtime_error("Incomplete ATRAC3 OMA input");
    size_t offset=0;
    if(!std::memcmp(data,"ea3",3)||!std::memcmp(data,"ID3",3)) {
        if(bytes<10)throw std::runtime_error("Truncated OMA metadata");
        for(unsigned i=6;i<10;++i){if(data[i]&128)throw std::runtime_error("Invalid OMA metadata size");offset=(offset<<7)|data[i];}
        offset+=10+((data[5]&16)?10:0);
    }
    if(offset>bytes||bytes-offset<96||std::memcmp(data+offset,"EA3",3)||data[offset+4]!=0||data[offset+5]!=96||data[offset+32]!=0)
        throw std::runtime_error("Expected ordinary ATRAC3 EA3 header");
    uint32_t params=(uint32_t(data[offset+33])<<16)|(uint32_t(data[offset+34])<<8)|data[offset+35];
    Layout result{offset+96,0,0};
    switch(params) {
    case 0x022018:result.frame_bytes=192;result.bitrate=66000;break;
    case 0x002026:result.frame_bytes=304;result.bitrate=105000;break;
    case 0x002030:result.frame_bytes=384;result.bitrate=132000;break;
    default:throw std::runtime_error("Unsupported ATRAC3 profile; expected44.1kHz stereo66/105/132kbps");
    }
    if((bytes-result.audio)%result.frame_bytes)throw std::runtime_error("Truncated ATRAC3 physical frame");
    return result;
}
}
extern "C" int atracodec_atrac3_decode_oma(const uint8_t*data,size_t bytes,atracodec_atrac3_pcm*out)
{
    error.clear();if(out)std::memset(out,0,sizeof(*out));
    if(!out){error="Output pointer is required";return -1;}
    int16_t *samples=nullptr;
    try {
        const atracodec_detail::FloatingEnvironment environment;
        const auto l=layout(data,bytes);const size_t count=(bytes-l.audio)/l.frame_bytes;
        const size_t emitted=count?count-1:0;
        if(emitted>std::numeric_limits<size_t>::max()/4096)throw std::runtime_error("ATRAC3 PCM size overflow");
        if(emitted){samples=static_cast<int16_t*>(std::malloc(emitted*4096));if(!samples)throw std::bad_alloc();}
        atracodec_atrac3_core_state state{};
        if(atracodec_atrac3_core_initialize(&state,l.bitrate))throw std::runtime_error("ATRAC3 initialization failed");
        int16_t priming[2048];
        for(size_t i=0;i<count;++i) {
            int16_t *destination=i?samples+(i-1)*2048:priming;
            if(atracodec_atrac3_core_decode(&state,data+l.audio+i*l.frame_bytes,l.frame_bytes,destination))
                throw std::runtime_error("Malformed ATRAC3 core at physical frame"+std::to_string(i));
        }
        out->samples=samples;out->sample_frames=uint64_t(emitted)*1024;
        out->sample_rate=44100;out->channels=2;out->source_format=1;out->core_bitrate=l.bitrate;
        return 0;
    } catch(const std::exception&e){std::free(samples);error=e.what();return -1;}
    catch(...){std::free(samples);error="Unknown ATRAC3 decoding failure";return -1;}
}
extern "C" void atracodec_atrac3_free_pcm(atracodec_atrac3_pcm*out)
{
    if(out){std::free(out->samples);std::memset(out,0,sizeof(*out));}
}
extern "C" const char *atracodec_atrac3_last_error(void){return error.c_str();}

