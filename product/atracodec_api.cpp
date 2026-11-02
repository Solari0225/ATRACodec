#include "atracodec.h"
#include "sony_original_encoder.h"
#include "atracodec_fenv.hpp"
#include "../decoder/aal/aal_decode.h"
#if defined(ATRACODEC_ENABLE_ATRAC3_DECODE)
#include "../decoder/atrac3/atracodec_atrac3_decode.h"
#endif
#if defined(ATRACODEC_ENABLE_PLUS_DECODE)
#include "../decoder/plus/atracodec_plus_decoder.h"
#endif
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
namespace {
thread_local std::string error;
uint32_t source_format(const uint8_t *data,size_t bytes){
    if(!data||bytes<96)throw std::runtime_error("Input is not a complete OMA file");
    size_t offset=0;
    if(!std::memcmp(data,"ea3",3)||!std::memcmp(data,"ID3",3)){
        for(unsigned i=6;i<10;++i){if(data[i]&128)throw std::runtime_error("Invalid OMA metadata length");offset=(offset<<7)|data[i];}
        offset+=10+((data[5]&16)?10:0);
    }
    if(offset>bytes||bytes-offset<96||std::memcmp(data+offset,"EA3",3)||data[offset+4]||data[offset+5]!=96)
        throw std::runtime_error("Invalid OMA/EA3 header");
    switch(data[offset+32]){
    case 0:return ATRACODEC_ATRAC3;case 1:return ATRACODEC_ATRAC3PLUS;
    case 0x22:return ATRACODEC_AAL3;case 0x21:return ATRACODEC_AALP;
    default:throw std::runtime_error("Unsupported OMA codec");
    }
}
template<class PCM>void transfer(PCM &decoded,int16_t **samples,atracodec_audio_info *info){
    info->sample_rate=decoded.sample_rate;info->channels=decoded.channels;
    info->sample_bits=16;info->source_format=decoded.source_format;
    info->core_bitrate=decoded.core_bitrate;info->sample_frames=decoded.sample_frames;
    *samples=decoded.samples;decoded.samples=nullptr;
}
}
extern "C" const char *atracodec_version(void){return "0.5.0-test1";}
extern "C" uint32_t atracodec_profile_count(void){return sony_original_encoder_profile_count();}
extern "C" int atracodec_profile_at(uint32_t index,atracodec_profile *out){
    if(!out)return -1;sony_original_profile old{};
    if(sony_original_encoder_profile_at(index,&old))return -1;
    static_assert(sizeof(old)==sizeof(*out),"Public profile layout");
    std::memcpy(out,&old,sizeof(old));return 0;
}
extern "C" int atracodec_encode_pcm16(const int16_t *pcm,uint32_t frames,
    uint32_t format,uint32_t bitrate,uint8_t **out,size_t *bytes){
    error.clear();if(out)*out=nullptr;if(bytes)*bytes=0;
    if(!out||!bytes){error="Output pointers are required";return -1;}
    const int status=sony_original_encoder_encode_pcm16(pcm,frames,format,bitrate,out,bytes);
    if(status)error=sony_original_encoder_last_error();return status;
}
extern "C" int atracodec_decode_pcm16(const uint8_t *data,size_t bytes,
    int16_t **samples,atracodec_audio_info *info){
    error.clear();if(samples)*samples=nullptr;if(info)std::memset(info,0,sizeof(*info));
    if(!samples||!info){error="Output pointers are required";return -1;}
    try{
        const atracodec_detail::FloatingEnvironment environment;
        switch(source_format(data,bytes)){
        case ATRACODEC_AAL3:case ATRACODEC_AALP:{
            atracodec_aal_pcm decoded{};
            if(atracodec_aal_decode_oma(data,bytes,&decoded))throw std::runtime_error(atracodec_aal_last_error());
            transfer(decoded,samples,info);return 0;
        }
        case ATRACODEC_ATRAC3:
#if defined(ATRACODEC_ENABLE_ATRAC3_DECODE)
        {
            atracodec_atrac3_pcm decoded{};
            if(atracodec_atrac3_decode_oma(data,bytes,&decoded))throw std::runtime_error(atracodec_atrac3_last_error());
            transfer(decoded,samples,info);return 0;
        }
#else
            throw std::runtime_error("ATRAC3 decoder is still under independent Sony-based development");
#endif
        case ATRACODEC_ATRAC3PLUS:
#if defined(ATRACODEC_ENABLE_PLUS_DECODE)
        {
            atracodec_plus_pcm decoded{};
            if(atracodec_plus_decode_oma(data,bytes,&decoded))throw std::runtime_error(atracodec_plus_last_error());
            transfer(decoded,samples,info);return 0;
        }
#else
            throw std::runtime_error("ATRAC3plus decoder is still under independent Sony-based development");
#endif
        }
        throw std::runtime_error("Unsupported decoder format");
    }catch(const std::exception &e){error=e.what();return -1;}
    catch(...){error="Unknown decoding failure";return -1;}
}
extern "C" void atracodec_free(void *p){std::free(p);}
extern "C" const char *atracodec_last_error(void){return error.c_str();}
