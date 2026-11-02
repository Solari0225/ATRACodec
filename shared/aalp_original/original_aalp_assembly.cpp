#include "original_aalp_assembly.h"
#include "original_aalp_reader.h"
#include "../aal_original/reused/src/aal_format.hpp"
#include "../aal_original/reused/compression/enhancement.hpp"
#include <cstdlib>
#include <string>
namespace {
using independent_aal::Bytes;
thread_local std::string error;
Bytes assemble(const int16_t *pcm,uint32_t frames,const uint8_t *cores,
    uint32_t count,uint32_t core_bytes){
    if(!pcm||!frames||!cores)throw std::runtime_error("AALP needs nonempty signed16 stereo PCM and core frames");
    const independent_aal::EncodingProfile *profile=nullptr;
    for(const auto &p:independent_aal::kOfficialProfiles)if(p.codec==independent_aal::CoreCodec::Atrac3plus&&p.core_bytes==core_bytes)profile=&p;
    if(!profile)throw std::runtime_error("Unsupported original ATRAC3plus frame size");
    if(count!=independent_aal::profile_block_count(frames,*profile))throw std::runtime_error("AALP PCM/core count mismatch");
    const uint32_t cap=core_bytes<=744?8:(core_bytes<=1488?12:16);
    sony_aalp_predictor_state state;sony_original_aalp_predictor_reset(&state);
    std::vector<int16_t> predicted(size_t(count)*4096);
    for(uint32_t ordinal=0;ordinal<count;++ordinal){
        sony_aalp_core_fields fields;uint32_t positions[10];int16_t planar[2][2048];
        const int stage=sony_original_aalp_read_core(cores+size_t(ordinal)*core_bytes,core_bytes,&fields,positions);
        if(stage||fields.channels!=2)throw std::runtime_error("Original AALP physical core parser failed at block "+std::to_string(ordinal)+" stage "+std::to_string(stage));
        if(sony_original_aalp_predict(&fields,cap,&state,planar))throw std::runtime_error("Original AALP integer predictor failed");
        for(size_t j=0;j<2048;++j)for(size_t ch=0;ch<2;++ch)predicted[size_t(ordinal)*4096+j*2+ch]=planar[ch][j];
    }
    constexpr size_t priming=(2048+368)*2;
    if(predicted.size()<size_t(frames)*2+priming)throw std::runtime_error("AALP prediction tail incomplete");
    std::vector<int16_t> residual(predicted.size()-priming);
    for(size_t i=0;i<residual.size();++i){const uint16_t source=i<size_t(frames)*2?uint16_t(pcm[i]):0;
        const uint16_t value=uint16_t(source-uint16_t(predicted[i+priming]));std::memcpy(&residual[i],&value,2);}
    Bytes output=independent_aal::aal_header(frames,count,*profile);
    for(uint32_t ordinal=0;ordinal<count;++ordinal){independent_aal::compression::EnhancementPacket enhancement;
        if(ordinal){independent_aal::compression::PlusStereoFrame errors{};
            for(size_t j=0;j<2048;++j)for(size_t ch=0;ch<2;++ch){const int64_t index=(int64_t(ordinal)-1)*2048+j-368;
                if(index>=0&&uint64_t(index)<residual.size()/2)errors[ch][j]=residual[size_t(index)*2+ch];}
            enhancement=independent_aal::compression::encode_plus_enhancement(errors);
        }
        const Bytes packet=independent_aal::aal_block(cores+size_t(ordinal)*core_bytes,ordinal,enhancement.bytes,
            ordinal+1==count,independent_aal::profile_tail(frames,*profile),enhancement.padding_bytes,*profile);
        output.insert(output.end(),packet.begin(),packet.end());
    }
    const auto parsed=independent_aal::parse_aal(output);uint64_t checked=0;
    for(uint32_t ordinal=1;ordinal<count;++ordinal){independent_aal::compression::EnhancementPacket enhancement;
        enhancement.bytes=parsed.packets[ordinal].enhancement;enhancement.padding_bytes=parsed.packets[ordinal].padding;
        const auto errors=independent_aal::compression::decode_plus_enhancement(enhancement);
        for(size_t j=0;j<2048;++j)for(size_t ch=0;ch<2;++ch){const int64_t index=(int64_t(ordinal)-1)*2048+j-368;
            if(index<0||uint64_t(index)>=frames)continue;
            const size_t at=size_t(index)*2+ch;const uint16_t restored=uint16_t(uint16_t(errors[ch][j])+uint16_t(predicted[at+priming]));
            if(restored!=uint16_t(pcm[at]))throw std::runtime_error("AALP assembled enhancement read-back mismatch");++checked;}
    }
    if(checked!=uint64_t(frames)*2)throw std::runtime_error("AALP assembled PCM sample count mismatch");
    return output;
}
}
extern "C" int sony_original_aalp_assemble(const int16_t *pcm,uint32_t frames,
    const uint8_t *cores,uint32_t count,uint32_t core_bytes,uint8_t **output,size_t *bytes){
    if(!output||!bytes)return -1;*output=nullptr;*bytes=0;error.clear();
    try{const Bytes data=assemble(pcm,frames,cores,count,core_bytes);void *p=std::malloc(data.size());if(!p)throw std::bad_alloc();
        std::memcpy(p,data.data(),data.size());*output=(uint8_t*)p;*bytes=data.size();return 0;
    }catch(const std::exception &e){error=e.what();return -1;}catch(...){error="Unknown AALP assembly failure";return -1;}
}
extern "C" void sony_original_aalp_free(void *p){std::free(p);}
extern "C" const char *sony_original_aalp_last_error(void){return error.c_str();}
