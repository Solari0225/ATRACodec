#include "original_aal.h"
#include "original_fixed_predictor.hpp"
#include "../pcm132/recovered_atrac3_pcm132.h"
#include "reused/src/aal_format.hpp"
#include "reused/compression/enhancement.hpp"
#include <cstdlib>
#include <memory>
#include <string>
namespace {
using independent_aal::Bytes;
thread_local std::string last_error;
Bytes encode(const int16_t *pcm,uint32_t frames){
    if(!pcm||!frames)throw std::runtime_error("AAL3 needs nonempty signed16 stereo PCM");
    const uint32_t count=independent_aal::block_count(frames);
    std::vector<std::array<uint8_t,384>> cores;cores.reserve(count);
    sony_atrac3_independent_stereo_stream stream;
    if(sony_recovered_atrac3_stereo_stream_init(&stream,132000))throw std::runtime_error("Original ATRAC3/132 factory failed");
    for(uint32_t feed=0;cores.size()<count;++feed){
        std::array<int16_t,2048> input{};std::array<uint8_t,387> out{};uint32_t end_bits[2];
        const uint64_t offset=uint64_t(feed)*2048,total=uint64_t(frames)*2;
        if(offset<total)std::copy_n(pcm+size_t(offset),size_t(std::min<uint64_t>(2048,total-offset)),input.data());
        const int result=sony_recovered_atrac3_stereo_stream_block(&stream,input.data(),out.data(),uint32_t(out.size()),end_bits);
        if(result<0)throw std::runtime_error("Independent original ATRAC3/132 PCM block failed");
        if(result){std::array<uint8_t,384> core;std::copy_n(out.data(),384,core.data());cores.push_back(core);}
        if(feed>count+1)throw std::runtime_error("Original AAL3 core drain contract failed");
    }
    sony_aal_original::OriginalFixedPredictor predictor;
    std::vector<int16_t> predicted;predicted.reserve(size_t(count)*2048);
    for(const auto&core:cores){auto decoded=predictor.decode(core.data());predicted.insert(predicted.end(),decoded.begin(),decoded.end());}
    constexpr size_t priming=2*(1024+138);
    if(predicted.size()<size_t(frames)*2+priming)throw std::runtime_error("Original AAL3 predictor tail incomplete");
    std::vector<int16_t> residual(predicted.size()-priming);
    for(size_t i=0;i<residual.size();++i){
        uint16_t source=i<size_t(frames)*2?uint16_t(pcm[i]):0;
        uint16_t value=uint16_t(source-uint16_t(predicted[i+priming]));std::memcpy(&residual[i],&value,2);
    }
    Bytes output=independent_aal::aal3_header(frames,count);
    // Original pure AAL3 formatter derives its nominal duration from packet
    // count and unshifted tail, including the extra drain block near a frame
    // boundary. Retain that observed quirk; the exact PCM count is in BLK.
    const uint64_t declared_samples=uint64_t(count-2)*1024+independent_aal::final_samples(frames);
    independent_aal::be32put(output.data()+36,uint32_t(declared_samples*1000/44100));
    for(uint32_t ordinal=0;ordinal<count;++ordinal){
        independent_aal::compression::EnhancementPacket enhancement;
        if(ordinal){
            independent_aal::compression::StereoFrame errors{};
            for(unsigned ch=0;ch<2;++ch)for(unsigned j=0;j<1024;++j){
                int64_t n=(int64_t(ordinal)-1)*1024+j-138;
                if(n>=0&&uint64_t(n)<residual.size()/2)errors[ch][j]=residual[size_t(n)*2+ch];
            }
            enhancement=independent_aal::compression::encode_enhancement(errors);
        }
        const auto packet=independent_aal::block(cores[ordinal].data(),ordinal,enhancement.bytes,
            ordinal+1==count,independent_aal::final_samples(frames),enhancement.padding_bytes);
        output.insert(output.end(),packet.begin(),packet.end());
    }
    // Independent read-back covers the actual assembled syntax and enhancement
    // fields. External Sony decode and official-byte acceptance stay in tests.
    const auto parsed=independent_aal::parse_aal3(output);
    uint64_t checked=0;
    for(uint32_t ordinal=1;ordinal<count;++ordinal){
        independent_aal::compression::EnhancementPacket enhancement;
        enhancement.bytes=parsed.packets[ordinal].enhancement;enhancement.padding_bytes=parsed.packets[ordinal].padding;
        auto decoded=independent_aal::compression::decode_enhancement(enhancement);
        for(unsigned ch=0;ch<2;++ch)for(unsigned j=0;j<1024;++j){
            int64_t n=(int64_t(ordinal)-1)*1024+j-138;
            if(n<0||uint64_t(n)>=frames)continue;
            const size_t source_index=size_t(n)*2+ch;
            uint16_t restored=uint16_t(uint16_t(decoded[ch][j])+uint16_t(predicted[source_index+priming]));
            if(restored!=uint16_t(pcm[source_index]))throw std::runtime_error("Independent original AAL3 read-back mismatch");
            ++checked;
        }
    }
    if(checked!=uint64_t(frames)*2)throw std::runtime_error("Independent original AAL3 read-back sample count mismatch");
    return output;
}
}
extern "C" int sony_original_aal3_encode(const int16_t *pcm,uint32_t frames,uint8_t **output,size_t *bytes){
    if(!output||!bytes)return -1;*output=nullptr;*bytes=0;last_error.clear();
    try{
        const Bytes encoded=encode(pcm,frames);
        void *memory=std::malloc(encoded.size());if(!memory)throw std::bad_alloc();
        std::memcpy(memory,encoded.data(),encoded.size());*output=(uint8_t*)memory;*bytes=encoded.size();return 0;
    }catch(const std::exception&e){last_error=e.what();return -1;}
    catch(...){last_error="Unknown original AAL3 encoding error";return -1;}
}
extern "C" void sony_original_aal_free(void *p){std::free(p);}
extern "C" const char *sony_original_aal_last_error(void){return last_error.c_str();}
