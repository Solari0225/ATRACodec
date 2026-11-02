#include "atracodec_plus_decoder.h"
#include "plus_core.h"
#include "../common/plus_float_pcm16.h"
#include "../../product/atracodec_fenv.hpp"
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
thread_local std::string error;
struct Layout {size_t audio,frame_bytes;uint32_t bitrate;};
Layout layout(const uint8_t *data,size_t bytes)
{
    if(!data||bytes<96)throw std::runtime_error("Incomplete ATRAC3plus OMA input");
    size_t offset=0;
    if(!std::memcmp(data,"ea3",3)||!std::memcmp(data,"ID3",3)){
        if(bytes<10)throw std::runtime_error("Truncated OMA metadata");
        for(unsigned i=6;i<10;++i){if(data[i]&128)throw std::runtime_error("Invalid OMA metadata size");offset=(offset<<7)|data[i];}
        offset+=10+((data[5]&16)?10:0);
    }
    if(offset>bytes||bytes-offset<96||std::memcmp(data+offset,"EA3",3)||data[offset+4]||data[offset+5]!=96||data[offset+32]!=1)
        throw std::runtime_error("Expected ordinary ATRAC3plus EA3 header");
    const uint8_t *h=data+offset;
    const uint16_t marker=uint16_t((uint16_t(h[6])<<8)|h[7]);
    if(marker!=0xff80&&marker!=0xffff)throw std::runtime_error("Encrypted OMA is outside decoder scope");
    const uint32_t parameter=(uint32_t(h[33])<<16)|(uint32_t(h[34])<<8)|h[35];
    const uint32_t sizes[9]={280,376,560,744,936,1120,1488,1864,2048};
    const uint32_t rates[9]={48000,64000,96000,128000,160000,192000,256000,320000,352000};
    Layout result{offset+96,0,0};
    for(unsigned i=0;i<9;++i)if(parameter==(0x2800u|(sizes[i]/8-1))){result.frame_bytes=sizes[i];result.bitrate=rates[i];break;}
    if(!result.bitrate)throw std::runtime_error("Unsupported ATRAC3plus profile; expected44.1kHz stereo48..352kbps");
    if((bytes-result.audio)%result.frame_bytes)throw std::runtime_error("Truncated ATRAC3plus physical frame");
    return result;
}
}
extern "C" int atracodec_plus_decode_oma(const uint8_t *data,size_t bytes,atracodec_plus_pcm *out)
{
    error.clear();if(out)std::memset(out,0,sizeof(*out));
    if(!out){error="Output pointer is required";return -1;}
    int16_t *samples=nullptr;
    try{
        const atracodec_detail::FloatingEnvironment environment;
        const auto l=layout(data,bytes);const size_t packets=(bytes-l.audio)/l.frame_bytes;
        if(packets>std::numeric_limits<uint64_t>::max()/2048)throw std::runtime_error("ATRAC3plus sample count overflow");
        const uint64_t decoded=uint64_t(packets)*2048;
        const uint64_t frames=decoded>2416?decoded-2416:0;
        if(frames>std::numeric_limits<size_t>::max()/(2*sizeof(int16_t)))throw std::runtime_error("ATRAC3plus PCM size overflow");
        if(frames){samples=static_cast<int16_t*>(std::malloc(size_t(frames)*2*sizeof(int16_t)));if(!samples)throw std::bad_alloc();}
        atracodec_plus_core_state state;atracodec_plus_core_reset(&state);
        uint64_t written=0;
        for(size_t i=0;i<packets;++i){
            const uint8_t *packet=data+l.audio+i*l.frame_bytes;
            // Original OMA delivery requires the first unit to be the default
            // stereo audio header. The physical DSP additionally has a unit2
            // skipper; OMA accepts such extensions only after the audio unit.
            if((packet[0]&0xe0)!=0x20)throw std::runtime_error("Invalid default ATRAC3plus audio unit layout");
            atracodec_plus_frame fields;uint32_t positions[10];
            if(atracodec_plus_read_core(packet,uint32_t(l.frame_bytes),&fields,positions)||fields.channels!=2)
                throw std::runtime_error("Malformed ATRAC3plus audio data at physical frame"+std::to_string(i));
            float signal[2][2048];
            if(atracodec_plus_core_render(&fields,&state,signal))throw std::runtime_error("Invalid ATRAC3plus reconstruction data");
            if(uint64_t(i+1)*2048<=2416)continue;
            int16_t interleaved[4096];
            if(atracodec_plus_float_pcm16(signal[0],signal[1],interleaved))throw std::runtime_error("ATRAC3plus PCM conversion failed");
            for(unsigned j=0;j<2048;++j){const uint64_t absolute=uint64_t(i)*2048+j;if(absolute<2416)continue;
                const size_t destination=size_t(absolute-2416)*2;samples[destination]=interleaved[j*2];samples[destination+1]=interleaved[j*2+1];++written;}
        }
        if(written!=frames)throw std::runtime_error("ATRAC3plus PCM reconstruction coverage mismatch");
        out->samples=samples;out->sample_frames=frames;out->sample_rate=44100;out->channels=2;out->source_format=2;out->core_bitrate=l.bitrate;
        return 0;
    }catch(const std::exception &e){std::free(samples);error=e.what();return -1;}
    catch(...){std::free(samples);error="Unknown ATRAC3plus decoding failure";return -1;}
}
extern "C" void atracodec_plus_free_pcm(atracodec_plus_pcm *out)
{
    if(out){std::free(out->samples);std::memset(out,0,sizeof(*out));}
}
extern "C" const char *atracodec_plus_last_error(void){return error.c_str();}
