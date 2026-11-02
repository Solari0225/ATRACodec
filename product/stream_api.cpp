#include "atracodec_stream.h"
#include "stream_decode.hpp"
#ifndef ATRACODEC_STREAM_DECODER_ONLY
#include "stream_encode.hpp"
#endif
#include "atracodec_fenv.hpp"
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
namespace {
thread_local std::string stream_error;
template<class T>struct State {
    std::unique_ptr<T> codec;
    bool busy=false,failed=false,finished=false;
};
struct Busy {
    bool &flag;explicit Busy(bool &f):flag(f){flag=true;}~Busy(){flag=false;}
};
template<class Context,class Operation>int mutate(Context *context,Operation operation,bool finish=false){
    stream_error.clear();
    if(!context){stream_error="Stream context is null";return -1;}
    try{
        if(context->busy)throw std::runtime_error("Stream context cannot be reentered");
        if(context->failed)throw std::runtime_error("Stream context has failed; destroy it");
        if(context->finished)throw std::runtime_error("Stream context is already finished");
        Busy active(context->busy);const atracodec_detail::FloatingEnvironment environment;
        operation(*context->codec);
        if(context->failed)throw std::runtime_error("Stream context was reentered from a callback");
        context->finished=finish;stream_error.clear();return 0;
    }catch(const std::exception &e){context->failed=true;stream_error=e.what();return -1;}
    catch(...){context->failed=true;stream_error="Unknown streaming failure";return -1;}
}
template<class Context>void destroy(Context *context){
    if(!context)return;
    if(context->busy){context->failed=true;stream_error="Cannot destroy an active stream context";return;}
    delete context;
}
}
struct atracodec_stream_decoder:State<atracodec_stream::Decoder>{
    atracodec_pcm_sink sink=nullptr;void *user=nullptr;
};
#ifndef ATRACODEC_STREAM_DECODER_ONLY
struct atracodec_stream_encoder:State<atracodec_stream::Encoder>{
    atracodec_bytes_sink sink=nullptr;void *user=nullptr;
};
extern "C" int atracodec_encoder_create(uint32_t format,uint32_t bitrate,uint64_t frames,
    atracodec_bytes_sink sink,void *user,atracodec_stream_encoder **output){
    stream_error.clear();if(output)*output=nullptr;
    if(!output||!sink){stream_error="Output context pointer and byte callback are required";return -1;}
    try{
        const atracodec_detail::FloatingEnvironment environment;
        auto context=std::make_unique<atracodec_stream_encoder>();
        context->sink=sink;context->user=user;context->busy=true;
        context->codec=atracodec_stream::make_encoder(format,bitrate,frames,
            [](void *opaque,const uint8_t *data,size_t bytes){
                auto &c=*static_cast<atracodec_stream_encoder*>(opaque);
                if(c.failed)return 1;
                const int result=c.sink(c.user,data,bytes);
                return result||c.failed?1:0;
            },context.get());
        context->busy=false;
        *output=context.release();stream_error.clear();return 0;
    }catch(const std::exception &e){stream_error=e.what();return -1;}
    catch(...){stream_error="Unknown streaming encoder initialization failure";return -1;}
}
extern "C" int atracodec_encoder_write(atracodec_stream_encoder *context,const int16_t *pcm,size_t frames){
    return mutate(context,[&](auto &codec){codec.write(pcm,frames);});
}
extern "C" int atracodec_encoder_finish(atracodec_stream_encoder *context){return mutate(context,[](auto &codec){codec.finish();},true);}
extern "C" void atracodec_encoder_destroy(atracodec_stream_encoder *context){destroy(context);}
#endif
extern "C" int atracodec_decoder_create(atracodec_pcm_sink sink,void *user,atracodec_stream_decoder **output){
    stream_error.clear();if(output)*output=nullptr;
    if(!output||!sink){stream_error="Output context pointer and PCM callback are required";return -1;}
    try{
        const atracodec_detail::FloatingEnvironment environment;
        auto context=std::make_unique<atracodec_stream_decoder>();
        context->sink=sink;context->user=user;
        context->codec=atracodec_stream::make_decoder(
            [](void *opaque,const int16_t *pcm,size_t frames){
                auto &c=*static_cast<atracodec_stream_decoder*>(opaque);
                if(c.failed)return 1;
                const int result=c.sink(c.user,pcm,frames);
                return result||c.failed?1:0;
            },context.get());
        *output=context.release();return 0;
    }catch(const std::exception &e){stream_error=e.what();return -1;}
    catch(...){stream_error="Unknown streaming decoder initialization failure";return -1;}
}
extern "C" int atracodec_decoder_write(atracodec_stream_decoder *context,const uint8_t *data,size_t bytes){
    return mutate(context,[&](auto &codec){codec.write(data,bytes);});
}
extern "C" int atracodec_decoder_finish(atracodec_stream_decoder *context){return mutate(context,[](auto &codec){codec.finish();},true);}
extern "C" int atracodec_decoder_info(const atracodec_stream_decoder *context,atracodec_audio_info *output){
    stream_error.clear();if(output)std::memset(output,0,sizeof(*output));
    if(!context||!output){stream_error="Context and information output are required";return -1;}
    return context->codec->info(*output)?0:1;
}
extern "C" void atracodec_decoder_destroy(atracodec_stream_decoder *context){destroy(context);}
extern "C" const char *atracodec_stream_last_error(void){return stream_error.c_str();}
