#include "stream_decode.hpp"
#include "stream_aal_frame.hpp"
#include "atracodec_fenv.hpp"
#include "../decoder/atrac3/atrac3_core.h"
#include "../decoder/plus/plus_core.h"
#include "../decoder/common/plus_float_pcm16.h"
#include <array>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
namespace atracodec_stream {
namespace {
using namespace atracodec_stream_frame;
class IncrementalDecoder final:public Decoder {
    enum class Phase{Prefix,Header,Pure,AALHeader,AALPacket};
    PCMSink sink_;void *user_;Phase phase_=Phase::Prefix;
    std::array<uint8_t,16384> packet_{};
    size_t have_=0,need_=10;uint64_t skip_=0,ordinal_=0;
    uint32_t F_=0,delay_=0,core_bytes_=0,cap_=0,declared_=0;
    atracodec_audio_info info_{};bool header_=false,finished_=false,last_=false;
    std::unique_ptr<atracodec_atrac3_core_state> a3_;
    std::unique_ptr<atracodec_plus_core_state> plus_;
    std::unique_ptr<sony_aal_original::OriginalFixedPredictor> fixed3_;
    std::unique_ptr<sony_aalp_predictor_state> fixedplus_;
    std::vector<int16_t> tail_;
    void emit(const int16_t *pcm,size_t frames){
        if(!frames)return;
        if(uint64_t(frames)>UINT64_MAX-info_.sample_frames)throw std::runtime_error("Decoded sample count overflow");
        // Protect the numerical core even when a host callback changes FENV.
        const atracodec_detail::FloatingEnvironment restore_core_environment;
        if(sink_(user_,pcm,frames))throw std::runtime_error("PCM output callback failed");
        info_.sample_frames+=frames;
    }
    void head(){
        const uint8_t *h=packet_.data();
        if(std::memcmp(h,"EA3",3)||h[4]||h[5]!=96)throw std::runtime_error("Invalid OMA/EA3 header");
        if(be16(h+6)!=0xff80&&be16(h+6)!=0xffff)throw std::runtime_error("Encrypted OMA is outside decoder scope");
        info_.sample_rate=44100;info_.channels=2;info_.sample_bits=16;
        const uint32_t sizes[9]={280,376,560,744,936,1120,1488,1864,2048};
        const uint32_t rates[9]={48000,64000,96000,128000,160000,192000,256000,320000,352000};
        if(h[32]==0){
            info_.source_format=1;F_=1024;delay_=1024;
            const uint32_t p=(uint32_t(h[33])<<16)|be16(h+34);
            if(p==0x022018){core_bytes_=192;info_.core_bitrate=66000;}
            else if(p==0x002026){core_bytes_=304;info_.core_bitrate=105000;}
            else if(p==0x002030){core_bytes_=384;info_.core_bitrate=132000;}
            else throw std::runtime_error("Unsupported ATRAC3 profile");
            a3_=std::make_unique<atracodec_atrac3_core_state>();
            if(atracodec_atrac3_core_initialize(a3_.get(),info_.core_bitrate))throw std::runtime_error("ATRAC3 initialization failed");
        }else if(h[32]==1||h[32]==0x21){
            const bool lossless=h[32]==0x21;
            info_.source_format=lossless?4:2;F_=2048;delay_=lossless?368:2416;
            const uint32_t p=lossless?((uint32_t(h[49])<<16)|be16(h+50)):((uint32_t(h[33])<<16)|be16(h+34));
            for(unsigned i=0;i<9;++i)if(p==(0x2800u|(sizes[i]/8-1))){core_bytes_=sizes[i];info_.core_bitrate=rates[i];}
            if(!info_.core_bitrate||(lossless&&(h[33]!=0x91||h[48]!=1)))throw std::runtime_error("Unsupported ATRAC3plus/AALP profile");
            if(lossless){cap_=core_bytes_<=744?8:core_bytes_<=1488?12:16;fixedplus_=std::make_unique<sony_aalp_predictor_state>();sony_original_aalp_predictor_reset(fixedplus_.get());}
            else{plus_=std::make_unique<atracodec_plus_core_state>();atracodec_plus_core_reset(plus_.get());}
        }else if(h[32]==0x22){
            if(((uint32_t(h[49])<<16)|be16(h+50))!=0x2030||h[33]!=0x52||h[48])throw std::runtime_error("Unsupported AAL3 profile");
            info_.source_format=3;info_.core_bitrate=132000;F_=1024;delay_=138;core_bytes_=384;
            fixed3_=std::make_unique<sony_aal_original::OriginalFixedPredictor>();
        }else throw std::runtime_error("Unsupported OMA codec");
        if(info_.source_format>=3){
            // Native sequential decoding does not use the EA3 packet-count
            //or tail hint as the source-length authority. Actual BLK ordinal,
            //last marker and last-tail determine the emitted PCM.
            declared_=be32(h+40);
            phase_=Phase::AALHeader;need_=24;tail_.reserve(size_t(F_)*6);
        }else{phase_=Phase::Pure;need_=core_bytes_;}
        have_=0;header_=true;
    }
    void pure(){
        std::array<int16_t,4096> pcm{};
        if(info_.source_format==1){
            if(atracodec_atrac3_core_decode(a3_.get(),packet_.data(),core_bytes_,pcm.data()))throw std::runtime_error("Malformed ATRAC3 core");
        }else{
            if((packet_[0]&0xe0)!=0x20)throw std::runtime_error("Invalid ATRAC3plus stereo audio unit");
            atracodec_plus_frame fields;uint32_t positions[10];float signal[2][2048];
            if(atracodec_plus_read_core(packet_.data(),core_bytes_,&fields,positions)||fields.channels!=2)throw std::runtime_error("Malformed ATRAC3plus core");
            if(atracodec_plus_core_render(&fields,plus_.get(),signal)||atracodec_plus_float_pcm16(signal[0],signal[1],pcm.data()))throw std::runtime_error("ATRAC3plus reconstruction failed");
        }
        if(ordinal_>UINT64_MAX/F_-1)throw std::runtime_error("Physical frame count overflow");
        const uint64_t start=ordinal_*F_;
        const size_t crop=start>=delay_?0:size_t(std::min<uint64_t>(F_,uint64_t(delay_)-start));
        emit(pcm.data()+crop*2,F_-crop);++ordinal_;have_=0;
    }
    void aal_header(){
        const uint8_t *p=packet_.data();
        if(last_)throw std::runtime_error("Trailing AAL data after final block");
        const uint32_t payload=be16(p+4),flags=be16(p+12),tail=flags&0xfff;
        if(std::memcmp(p,"BLK",3)||payload<core_bytes_||payload>core_bytes_+F_*4+8||p[3]>7||p[3]>payload-core_bytes_||be32(p+8)!=ordinal_||!tail||tail>F_||(!ordinal_&&payload!=core_bytes_))throw std::runtime_error("Invalid AAL block header");
        const bool last=(flags&0x8000)!=0;
        if((last&&ordinal_==0)||(!last&&tail!=F_))throw std::runtime_error("Invalid AAL final/tail marker");
        need_=24+payload;phase_=Phase::AALPacket;
    }
    void aal_packet(){
        const uint8_t *p=packet_.data();const uint32_t payload=be16(p+4),flags=be16(p+12),r=flags&0xfff;const bool final=(flags&0x8000)!=0;
        std::array<std::array<int16_t,2048>,2> predicted{};
        if(info_.source_format==3){const auto values=fixed3_->decode(p+24);for(unsigned j=0;j<F_;++j)for(unsigned ch=0;ch<2;++ch)predicted[ch][j]=values[j*2+ch];}
        else{sony_aalp_core_fields fields;plus_core_frame(p+24,core_bytes_,fields);if(sony_original_aalp_predict(&fields,cap_,fixedplus_.get(),reinterpret_cast<int16_t(*)[2048]>(predicted.data())))throw std::runtime_error("AALP fixed prediction failed");}
        if(ordinal_){
            if(payload==core_bytes_)throw std::runtime_error("Missing AAL enhancement");
            const auto errors=enhancement(p+24+core_bytes_,payload-core_bytes_,p[3],F_);
            for(unsigned j=0;j<F_;++j){
                if(ordinal_==1&&j<delay_)continue;
                for(unsigned ch=0;ch<2;++ch)tail_.push_back(sample16(uint32_t(uint16_t(predicted[ch][j]))+uint32_t(errors[ch][j])));
            }
        }
        ++ordinal_;
        if(final){
            const uint64_t adjusted=ordinal_-2;const bool extra=r>F_-delay_;
            if(extra&&!adjusted)throw std::runtime_error("Inconsistent AAL source length");
            const uint64_t total=(adjusted-uint64_t(extra))*F_+r;
            if(!total||total<info_.sample_frames||total-info_.sample_frames>tail_.size()/2)throw std::runtime_error("AAL reconstruction coverage mismatch");
            emit(tail_.data(),size_t(total-info_.sample_frames));tail_.clear();last_=true;
        }else if(tail_.size()/2>size_t(F_)*2){
            const size_t frames=tail_.size()/2-size_t(F_)*2;emit(tail_.data(),frames);tail_.erase(tail_.begin(),tail_.begin()+frames*2);
        }
        if(tail_.size()>size_t(F_)*6)throw std::runtime_error("AAL bounded tail invariant failed");
        have_=0;need_=24;phase_=Phase::AALHeader;
    }
public:
    IncrementalDecoder(PCMSink sink,void *user):sink_(sink),user_(user){if(!sink_)throw std::invalid_argument("PCM callback is required");}
    void write(const uint8_t *data,size_t bytes)override{
        if(finished_)throw std::runtime_error("Decoder is already finished");
        if(bytes&&!data)throw std::invalid_argument("OMA input pointer is null");
        while(bytes){
            if(skip_){const size_t n=size_t(std::min<uint64_t>(skip_,bytes));skip_-=n;data+=n;bytes-=n;continue;}
            if(header_&&info_.source_format>=3&&last_)throw std::runtime_error("Trailing data after AAL stream");
            const size_t n=std::min(bytes,need_-have_);std::memcpy(packet_.data()+have_,data,n);have_+=n;data+=n;bytes-=n;
            if(have_!=need_)continue;
            switch(phase_){
            case Phase::Prefix:
                if(!std::memcmp(packet_.data(),"ea3",3)||!std::memcmp(packet_.data(),"ID3",3)){
                    for(unsigned i=6;i<10;++i){if(packet_[i]&128)throw std::runtime_error("Invalid OMA metadata length");skip_=(skip_<<7)|packet_[i];}
                    if(packet_[5]&16)skip_+=10;have_=0;
                }
                phase_=Phase::Header;need_=96;break;
            case Phase::Header:head();break;
            case Phase::Pure:pure();break;
            case Phase::AALHeader:aal_header();break;
            case Phase::AALPacket:aal_packet();break;
            }
        }
    }
    void finish()override{
        if(finished_)throw std::runtime_error("Decoder is already finished");
        if(!header_||skip_||have_)throw std::runtime_error("Truncated OMA header or physical frame");
        if(info_.source_format>=3&&ordinal_&&(!last_||!tail_.empty()))throw std::runtime_error("Missing AAL final block");
        finished_=true;
    }
    bool info(atracodec_audio_info &out)const override{out=info_;return header_;}
};
}
std::unique_ptr<Decoder> make_decoder(PCMSink sink,void *user){return std::make_unique<IncrementalDecoder>(sink,user);}
}
