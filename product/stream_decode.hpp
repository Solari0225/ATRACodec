#ifndef ATRACODEC_STREAM_DECODE_HPP
#define ATRACODEC_STREAM_DECODE_HPP
#include "atracodec.h"
#include <memory>
namespace atracodec_stream {
using PCMSink=int(*)(void*,const int16_t*,size_t);
class Decoder {
public:
    virtual ~Decoder()=default;
    virtual void write(const uint8_t*,size_t)=0;
    virtual void finish()=0;
    virtual bool info(atracodec_audio_info&)const=0;
};
std::unique_ptr<Decoder> make_decoder(PCMSink,void*);
}
#endif
