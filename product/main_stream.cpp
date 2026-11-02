#include "atracodec.h"
#include "atracodec_stream.h"
#include "stream_file.hpp"
#include <array>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
#include <shellapi.h>
#endif
namespace {
volatile std::sig_atomic_t interrupted=0;
bool progress_errors=false;
void interrupt(int){interrupted=1;}
std::string json_text(const std::string &value){
    std::string out="\"";const char *hex="0123456789abcdef";
    for(unsigned char c:value){
        if(c=='"'||c=='\\'){out+='\\';out+=char(c);}
        else if(c<32){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
        else out+=char(c);
    }
    return out+'"';
}
void help(){
    std::cout<<"atracodec encode [--profile aalp-352] INPUT.wav OUTPUT.oma [--overwrite]\n"
        "atracodec decode INPUT.oma OUTPUT.wav [--overwrite]\n"
        "atracodec --list-profiles [--json]\n"
        "Options: --progress (JSON lines on stderr), --cancel-file PATH\n"
        "PCM:44100Hz,16-bit integer,stereo RIFF/WAVE or RF64/WAVE.\n"
        "Incremental processing; output is committed only after success.\n";
}
atracodec_profile profile(const std::string &id){
    atracodec_profile p{};
    for(uint32_t i=0;i<atracodec_profile_count();++i)if(!atracodec_profile_at(i,&p)&&id==p.id)return p;
    throw std::invalid_argument("Unknown profile: "+id+". Use --list-profiles.");
}
struct Options {
    bool decode=false,overwrite=false,progress=false;
    std::string profile="aalp-352";
    std::filesystem::path input,output,cancel;
};
Options parse(const std::vector<std::string> &args){
    Options o;o.decode=args[1]=="decode";std::vector<std::string> paths;
    for(size_t i=2;i<args.size();++i){
        const auto &a=args[i];
        if(a=="--overwrite")o.overwrite=true;
        else if(a=="--progress")o.progress=true;
        else if(a=="--profile"&&!o.decode){
            if(++i==args.size())throw std::invalid_argument("--profile needs a value");o.profile=args[i];
        }else if(a=="--cancel-file"){
            if(++i==args.size())throw std::invalid_argument("--cancel-file needs a path");o.cancel=std::filesystem::u8path(args[i]);
        }else if(a.rfind("--",0)==0)throw std::invalid_argument("Unknown option: "+a);
        else paths.push_back(a);
    }
    if(paths.size()!=2)throw std::invalid_argument(o.decode?"decode needs INPUT.oma and OUTPUT.wav":"encode needs INPUT.wav and OUTPUT.oma");
    o.input=std::filesystem::u8path(paths[0]);o.output=std::filesystem::u8path(paths[1]);
    if(std::filesystem::exists(o.output)){
        if(std::filesystem::equivalent(o.input,o.output))throw std::invalid_argument("Input and output paths must differ");
        if(!o.overwrite)throw std::invalid_argument("Output already exists; choose another path or use --overwrite");
    }
    return o;
}
struct Cancellation {
    const std::filesystem::path &flag;
    void check()const{
        if(interrupted||(!flag.empty()&&std::filesystem::exists(flag)))throw std::runtime_error("Conversion cancelled");
    }
};
class Progress {
    bool enabled_;uint64_t total_bytes_,total_frames_;
    std::chrono::steady_clock::time_point last_{};
public:
    Progress(bool enabled,uint64_t bytes,uint64_t frames):enabled_(enabled),total_bytes_(bytes),total_frames_(frames){}
    void update(uint64_t input,uint64_t output,uint64_t frames,bool force=false){
        if(!enabled_)return;
        const auto now=std::chrono::steady_clock::now();
        if(!force&&now-last_<std::chrono::milliseconds(100))return;last_=now;
        std::cerr<<"{\"event\":\"progress\",\"input_bytes\":"<<input<<",\"output_bytes\":"<<output<<",\"processed_frames\":"<<frames
            <<",\"total_frames\":"<<total_frames_<<",\"total_input_bytes\":"<<total_bytes_<<"}\n";
        std::cerr.flush();
    }
};
struct Sink {
    atracodec_file::Output &file;const Cancellation &cancel;
    std::string error;uint64_t frames=0;
    static int bytes(void *user,const uint8_t *data,size_t count){
        auto &s=*static_cast<Sink*>(user);
        try{s.cancel.check();s.file.write(data,count);return 0;}
        catch(const std::exception &e){s.error=e.what();return 1;}
        catch(...){s.error="Output callback failed";return 1;}
    }
    static int pcm(void *user,const int16_t *pcm,size_t frames){
        auto &s=*static_cast<Sink*>(user);
        try{
            s.cancel.check();std::array<uint8_t,65536> data{};size_t at=0;
            while(at<frames){
                const size_t count=std::min<size_t>(frames-at,data.size()/4);
                for(size_t i=0;i<count*2;++i)atracodec_file::put16(data.data()+i*2,uint16_t(pcm[at*2+i]));
                s.file.write(data.data(),count*4);at+=count;
            }
            if(uint64_t(frames)>UINT64_MAX-s.frames)throw std::runtime_error("PCM frame count overflow");
            s.frames+=frames;return 0;
        }catch(const std::exception &e){s.error=e.what();return 1;}
        catch(...){s.error="PCM output callback failed";return 1;}
    }
    void check(int status)const{
        if(status)throw std::runtime_error(error.empty()?atracodec_stream_last_error():error);
    }
};
int encode(const Options &o){
    const auto p=profile(o.profile);Cancellation cancel{o.cancel};cancel.check();
    atracodec_file::Input input(o.input);const auto wave=atracodec_file::read_wave(input);
    atracodec_file::Output output(o.output,o.overwrite);Sink sink{output,cancel};
    atracodec_stream_encoder *context=nullptr;
    sink.check(atracodec_encoder_create(p.format,p.nominal_bitrate,wave.frames,&Sink::bytes,&sink,&context));
    std::unique_ptr<atracodec_stream_encoder,decltype(&atracodec_encoder_destroy)> owner(context,atracodec_encoder_destroy);
    Progress progress(o.progress,input.size(),wave.frames);progress.update(wave.offset,output.bytes(),0,true);
    std::array<uint8_t,65536> bytes{};std::array<int16_t,32768> pcm{};uint64_t consumed=0;
    while(consumed<wave.bytes){
        cancel.check();const size_t count=size_t(std::min<uint64_t>(bytes.size(),wave.bytes-consumed));
        if(input.read(bytes.data(),count)!=count)throw std::runtime_error("Truncated WAV PCM data");
        for(size_t i=0;i<count/2;++i){const uint16_t v=atracodec_file::le16(bytes.data()+i*2);pcm[i]=v<32768?int16_t(v):int16_t(int32_t(v)-65536);}
        sink.check(atracodec_encoder_write(context,pcm.data(),count/4));consumed+=count;
        progress.update(wave.offset+consumed,output.bytes(),consumed/4);
    }
    cancel.check();sink.check(atracodec_encoder_finish(context));cancel.check();output.commit();
    progress.update(input.size(),output.bytes(),wave.frames,true);
    std::cout<<"{\"operation\":\"encode\",\"profile\":"<<json_text(p.id)<<",\"pcm_frames\":"<<wave.frames<<",\"input_bytes\":"<<input.size()<<",\"output_bytes\":"<<output.bytes()
        <<",\"lossless\":"<<(p.lossless?"true":"false")<<",\"streaming\":true,\"sony_runtime_used\":false,\"atracdenc_used\":false}\n";
    return 0;
}
int decode(const Options &o){
    Cancellation cancel{o.cancel};cancel.check();atracodec_file::Input input(o.input);
    atracodec_file::Output output(o.output,o.overwrite);
    const auto provisional=atracodec_file::wave_header(0);output.write(provisional.data(),provisional.size());
    Sink sink{output,cancel};atracodec_stream_decoder *context=nullptr;
    sink.check(atracodec_decoder_create(&Sink::pcm,&sink,&context));
    std::unique_ptr<atracodec_stream_decoder,decltype(&atracodec_decoder_destroy)> owner(context,atracodec_decoder_destroy);
    Progress progress(o.progress,input.size(),0);progress.update(0,output.bytes(),0,true);
    std::array<uint8_t,65536> bytes{};uint64_t consumed=0;
    while(consumed<input.size()){
        cancel.check();const size_t count=size_t(std::min<uint64_t>(bytes.size(),input.size()-consumed));
        if(input.read(bytes.data(),count)!=count)throw std::runtime_error("Truncated OMA input");
        sink.check(atracodec_decoder_write(context,bytes.data(),count));consumed+=count;
        progress.update(consumed,output.bytes(),sink.frames);
    }
    cancel.check();sink.check(atracodec_decoder_finish(context));atracodec_audio_info info{};
    if(atracodec_decoder_info(context,&info)||info.sample_frames!=sink.frames)throw std::runtime_error("Streaming decoder information mismatch");
    const auto header=atracodec_file::wave_header(sink.frames);
    output.seek(0);output.write(header.data(),header.size());cancel.check();output.commit();
    progress.update(consumed,output.bytes(),sink.frames,true);
    std::cout<<"{\"operation\":\"decode\",\"format\":"<<info.source_format<<",\"pcm_frames\":"<<info.sample_frames
        <<",\"sample_rate\":44100,\"channels\":2,\"sample_bits\":16,\"input_bytes\":"<<input.size()<<",\"output_bytes\":"<<output.bytes()
        <<",\"streaming\":true,\"wave_container\":"<<json_text(!std::memcmp(header.data(),"RF64",4)?"RF64":"RIFF")<<"}\n";
    return 0;
}
int run(const std::vector<std::string> &args){
    if(args.size()==1||args[1]=="--help"||args[1]=="-h"){help();return 0;}
    if(args[1]=="--version"){std::cout<<"ATRACodec "<<atracodec_version()<<'\n';return 0;}
    if(args[1]=="--list-profiles"){
        const bool json=args.size()==3&&args[2]=="--json";
        if(args.size()>2&&!json)throw std::invalid_argument("Unknown profile-list option");
        if(json)std::cout<<'[';
        for(uint32_t i=0;i<atracodec_profile_count();++i){
            atracodec_profile p{};atracodec_profile_at(i,&p);
            if(json){if(i)std::cout<<',';std::cout<<"{\"id\":"<<json_text(p.id)<<",\"format\":"<<p.format<<",\"nominal_bitrate\":"<<p.nominal_bitrate
                <<",\"core_frame_bytes\":"<<p.core_frame_bytes<<",\"core_frame_samples\":"<<p.core_frame_samples<<",\"lossless\":"<<(p.lossless?"true":"false")<<'}';}
            else std::cout<<p.id<<'\t'<<(p.lossless?"lossless":"lossy")<<'\t'<<p.nominal_bitrate/1000<<"kbps core\n";
        }
        if(json)std::cout<<"]\n";return 0;
    }
    if(args[1]!="encode"&&args[1]!="decode")throw std::invalid_argument("Expected encode, decode or --list-profiles");
    const auto options=parse(args);return options.decode?decode(options):encode(options);
}
#ifdef _WIN32
std::string utf8(const wchar_t *p){
    const int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,p,-1,nullptr,0,nullptr,nullptr);
    if(!bytes)throw std::runtime_error("Invalid Unicode command line");std::string out(size_t(bytes),'\0');
    WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,p,-1,out.data(),bytes,nullptr,nullptr);out.pop_back();return out;
}
#endif
}
int main(int argc,char **argv){
    std::signal(SIGINT,interrupt);std::signal(SIGTERM,interrupt);
    try{
        std::vector<std::string> args;
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);int count=0;
        auto *wide=CommandLineToArgvW(GetCommandLineW(),&count);if(!wide)throw std::runtime_error("Cannot read command line");
        for(int i=0;i<count;++i)args.push_back(utf8(wide[i]));LocalFree(wide);
#else
        for(int i=0;i<argc;++i)args.emplace_back(argv[i]);
#endif
        progress_errors=std::find(args.begin(),args.end(),"--progress")!=args.end();
        return run(args);
    }catch(const std::exception &e){
        if(progress_errors)std::cerr<<"{\"event\":\"error\",\"message\":"<<json_text(e.what())<<"}\n";
        else std::cerr<<"Error: "<<e.what()<<'\n';
        return std::string(e.what())=="Conversion cancelled"?2:1;
    }
}
