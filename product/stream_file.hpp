#ifndef ATRACODEC_STREAM_FILE_HPP
#define ATRACODEC_STREAM_FILE_HPP
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif
namespace atracodec_file {
inline uint16_t le16(const uint8_t *p){return uint16_t(p[0]|(uint16_t(p[1])<<8));}
inline uint32_t le32(const uint8_t *p){return uint32_t(le16(p))|(uint32_t(le16(p+2))<<16);}
inline uint64_t le64(const uint8_t *p){return uint64_t(le32(p))|(uint64_t(le32(p+4))<<32);}
inline void put16(uint8_t *p,uint16_t n){p[0]=uint8_t(n);p[1]=uint8_t(n>>8);}
inline void put32(uint8_t *p,uint32_t n){put16(p,uint16_t(n));put16(p+2,uint16_t(n>>16));}
inline void put64(uint8_t *p,uint64_t n){put32(p,uint32_t(n));put32(p+4,uint32_t(n>>32));}
inline std::runtime_error os_error(const char *message,unsigned long code){return std::runtime_error(std::string(message)+" (OS error "+std::to_string(code)+")");}
class Input {
#ifdef _WIN32
    HANDLE file_=INVALID_HANDLE_VALUE;
#else
    int file_=-1;
#endif
    uint64_t size_=0;
public:
    explicit Input(const std::filesystem::path &path){
#ifdef _WIN32
        file_=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file_==INVALID_HANDLE_VALUE)throw os_error("Cannot open input file",GetLastError());
        LARGE_INTEGER n;
        if(!GetFileSizeEx(file_,&n)||n.QuadPart<0){const DWORD e=GetLastError();CloseHandle(file_);file_=INVALID_HANDLE_VALUE;throw os_error("Cannot determine input file size",e);}
        size_=uint64_t(n.QuadPart);
#else
        static_assert(sizeof(off_t)>=8,"64-bit input offsets are required");
        file_=::open(path.c_str(),O_RDONLY);if(file_<0)throw os_error("Cannot open input file",unsigned(errno));
        struct stat s;
        if(::fstat(file_,&s)<0||s.st_size<0){const int e=errno;::close(file_);file_=-1;throw os_error("Cannot determine input file size",unsigned(e));}
        size_=uint64_t(s.st_size);
#endif
    }
    Input(const Input&)=delete;Input&operator=(const Input&)=delete;
    ~Input(){
#ifdef _WIN32
        if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);
#else
        if(file_>=0)::close(file_);
#endif
    }
    uint64_t size()const{return size_;}
    void seek(uint64_t offset){
        if(offset>size_||offset>uint64_t(INT64_MAX))throw std::runtime_error("Input offset out of range");
#ifdef _WIN32
        LARGE_INTEGER n;n.QuadPart=LONGLONG(offset);if(!SetFilePointerEx(file_,n,nullptr,FILE_BEGIN))throw os_error("Input seek failed",GetLastError());
#else
        if(::lseek(file_,off_t(offset),SEEK_SET)<0)throw os_error("Input seek failed",unsigned(errno));
#endif
    }
    size_t read(uint8_t *data,size_t n){
        size_t read=0;
        while(read<n){const size_t block=std::min<size_t>(n-read,1048576);size_t got=0;
#ifdef _WIN32
            DWORD count=0;if(!ReadFile(file_,data+read,DWORD(block),&count,nullptr))throw os_error("Input read failed",GetLastError());got=count;
#else
            const auto count=::read(file_,data+read,block);if(count<0){if(errno==EINTR)continue;throw os_error("Input read failed",unsigned(errno));}got=size_t(count);
#endif
            if(!got)break;read+=got;
        }
        return read;
    }
    void at(uint64_t offset,uint8_t *data,size_t n){seek(offset);if(read(data,n)!=n)throw std::runtime_error("Truncated input file");}
};
/* Output is staged beside the selected destination. Success commits once;
 * failure/cancellation removes only this instance's exclusively created file.
 * No-overwrite also remains atomic if a destination appears during conversion. */
class Output {
    std::filesystem::path destination_,temporary_;bool overwrite_,committed_=false;
    uint64_t position_=0,size_=0;
#ifdef _WIN32
    HANDLE file_=INVALID_HANDLE_VALUE;
#else
    int file_=-1;
#endif
    void close_checked(){
#ifdef _WIN32
        if(file_==INVALID_HANDLE_VALUE)return;
        if(!FlushFileBuffers(file_))throw os_error("Output flush failed",GetLastError());
        const auto file=file_;file_=INVALID_HANDLE_VALUE;if(!CloseHandle(file))throw os_error("Output close failed",GetLastError());
#else
        if(file_<0)return;
        if(::fsync(file_)<0)throw os_error("Output flush failed",unsigned(errno));
        const int file=file_;file_=-1;if(::close(file)<0)throw os_error("Output close failed",unsigned(errno));
#endif
    }
public:
    Output(const std::filesystem::path &destination,bool overwrite):destination_(destination),overwrite_(overwrite){
        static std::atomic<uint64_t> sequence{0};
#ifdef _WIN32
        const auto pid=GetCurrentProcessId();
#else
        const auto pid=::getpid();
#endif
        const auto parent=destination_.has_parent_path()?destination_.parent_path():std::filesystem::path(".");
        for(unsigned attempt=0;attempt<128;++attempt){
            temporary_=parent/(".atracodec-"+std::to_string(pid)+"-"+std::to_string(sequence.fetch_add(1))+".tmp");
#ifdef _WIN32
            file_=CreateFileW(temporary_.c_str(),GENERIC_WRITE|GENERIC_READ,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file_!=INVALID_HANDLE_VALUE)return;
            const DWORD error=GetLastError();if(error!=ERROR_FILE_EXISTS&&error!=ERROR_ALREADY_EXISTS)throw os_error("Cannot create temporary output",error);
#else
            file_=::open(temporary_.c_str(),O_RDWR|O_CREAT|O_EXCL,0666);if(file_>=0)return;
            if(errno!=EEXIST)throw os_error("Cannot create temporary output",unsigned(errno));
#endif
        }
        throw std::runtime_error("Cannot allocate a unique temporary output");
    }
    Output(const Output&)=delete;Output&operator=(const Output&)=delete;
    ~Output(){
#ifdef _WIN32
        if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);
        if(!committed_&&!temporary_.empty())DeleteFileW(temporary_.c_str());
#else
        if(file_>=0)::close(file_);
        if(!committed_&&!temporary_.empty())::unlink(temporary_.c_str());
#endif
    }
    uint64_t bytes()const{return size_;}
    void seek(uint64_t position){
        if(position>uint64_t(INT64_MAX))throw std::runtime_error("Output offset overflow");
#ifdef _WIN32
        LARGE_INTEGER offset;offset.QuadPart=LONGLONG(position);if(!SetFilePointerEx(file_,offset,nullptr,FILE_BEGIN))throw os_error("Output seek failed",GetLastError());
#else
        static_assert(sizeof(off_t)>=8,"64-bit file offsets are required");
        if(::lseek(file_,off_t(position),SEEK_SET)<0)throw os_error("Output seek failed",unsigned(errno));
#endif
        position_=position;
    }
    void write(const uint8_t *data,size_t count){
        if(uint64_t(count)>UINT64_MAX-position_)throw std::runtime_error("Output byte count overflow");
        while(count){const size_t block=std::min<size_t>(count,1048576);size_t done=0;
#ifdef _WIN32
            DWORD n=0;if(!WriteFile(file_,data,DWORD(block),&n,nullptr))throw os_error("Output write failed",GetLastError());done=n;
#else
            const auto n=::write(file_,data,block);if(n<0){if(errno==EINTR)continue;throw os_error("Output write failed",unsigned(errno));}done=size_t(n);
#endif
            if(!done)throw std::runtime_error("Output write made no progress");
            count-=done;data+=done;position_+=done;size_=std::max(size_,position_);
        }
    }
    void commit(){
        close_checked();
#ifdef _WIN32
        const DWORD flags=MOVEFILE_WRITE_THROUGH|(overwrite_?MOVEFILE_REPLACE_EXISTING:0);
        if(!MoveFileExW(temporary_.c_str(),destination_.c_str(),flags)){
            const DWORD error=GetLastError();
            if(!overwrite_&&(error==ERROR_FILE_EXISTS||error==ERROR_ALREADY_EXISTS))throw std::runtime_error("Output already exists; choose another path or use --overwrite");
            throw os_error("Cannot commit output file",error);
        }
#else
        if(overwrite_){if(::rename(temporary_.c_str(),destination_.c_str())<0)throw os_error("Cannot commit output file",unsigned(errno));}
        else{
            if(::link(temporary_.c_str(),destination_.c_str())<0){if(errno==EEXIST)throw std::runtime_error("Output already exists; choose another path or use --overwrite");throw os_error("Cannot commit output file",unsigned(errno));}
            if(::unlink(temporary_.c_str())<0){committed_=true;throw os_error("Output committed but temporary file cleanup failed",unsigned(errno));}
        }
#endif
        committed_=true;
    }
};
struct WaveInput {uint64_t offset=0,bytes=0,frames=0;bool rf64=false;};
inline WaveInput read_wave(Input &file){
    std::array<uint8_t,40> b{};file.at(0,b.data(),12);
    const bool rf64=!std::memcmp(b.data(),"RF64",4);
    if((!rf64&&std::memcmp(b.data(),"RIFF",4))||std::memcmp(b.data()+8,"WAVE",4))throw std::runtime_error("Expected PCM RIFF/WAVE or RF64/WAVE");
    if(rf64&&le32(b.data()+4)!=UINT32_MAX)throw std::runtime_error("Invalid RF64 size marker");
    uint64_t logical=file.size();
    if(!rf64){const uint32_t declared=le32(b.data()+4);if(declared<4||uint64_t(declared)+8>file.size())throw std::runtime_error("Invalid RIFF length");logical=uint64_t(declared)+8;}
    bool format=false,data=false,ds64=false;uint64_t data64=0,samples64=0;WaveInput result;result.rf64=rf64;
    for(uint64_t at=12;at<=logical&&logical-at>=8;){
        file.at(at,b.data(),8);uint64_t n=le32(b.data()+4);const bool is_data=!std::memcmp(b.data(),"data",4);
        if(rf64&&n==UINT32_MAX){if(!is_data||!ds64)throw std::runtime_error("Unsupported RF64 chunk-size override");n=data64;}
        if(n>logical-at-8)throw std::runtime_error("Truncated WAVE chunk");
        if(!std::memcmp(b.data(),"ds64",4)){
            if(!rf64||ds64||n<28)throw std::runtime_error("Invalid RF64 ds64 chunk");file.at(at+8,b.data(),28);
            const uint64_t riff=le64(b.data());data64=le64(b.data()+8);samples64=le64(b.data()+16);const uint32_t table=le32(b.data()+24);
            if(riff>file.size()-8||riff<at+n||uint64_t(table)>(n-28)/12)throw std::runtime_error("Invalid RF64 length/table");
            logical=riff+8;ds64=true;
        }else if(!std::memcmp(b.data(),"fmt ",4)){
            if(format||n<16)throw std::runtime_error("Invalid WAVE format chunk");file.at(at+8,b.data(),16);
            if(le16(b.data())!=1||le16(b.data()+2)!=2||le32(b.data()+4)!=44100||le32(b.data()+8)!=176400||le16(b.data()+12)!=4||le16(b.data()+14)!=16)throw std::runtime_error("Expected44100Hz16-bit stereo integer PCM WAV");format=true;
        }else if(is_data){
            if(data||n%4)throw std::runtime_error("Invalid WAVE PCM data chunk");result.offset=at+8;result.bytes=n;result.frames=n/4;data=true;
        }
        if(format&&data)break;
        if(n==UINT64_MAX||at>UINT64_MAX-8-n-(n&1))throw std::runtime_error("WAVE chunk offset overflow");
        at+=8+n+(n&1);
    }
    if(!format||!data||(rf64&&!ds64)||(rf64&&samples64&&samples64!=result.frames))throw std::runtime_error("Missing or inconsistent WAVE format/data/ds64");
    file.seek(result.offset);return result;
}
inline std::array<uint8_t,80> wave_header(uint64_t frames){
    if(frames>(UINT64_MAX-80)/4)throw std::runtime_error("WAVE output size overflow");
    const uint64_t bytes=frames*4;const bool rf64=bytes>UINT32_MAX-72u;std::array<uint8_t,80> h{};
    std::memcpy(h.data(),rf64?"RF64":"RIFF",4);put32(h.data()+4,rf64?UINT32_MAX:uint32_t(bytes+72));std::memcpy(h.data()+8,"WAVE",4);
    std::memcpy(h.data()+12,rf64?"ds64":"JUNK",4);put32(h.data()+16,28);
    if(rf64){put64(h.data()+20,bytes+72);put64(h.data()+28,bytes);put64(h.data()+36,frames);put32(h.data()+44,0);}
    std::memcpy(h.data()+48,"fmt ",4);put32(h.data()+52,16);put16(h.data()+56,1);put16(h.data()+58,2);put32(h.data()+60,44100);put32(h.data()+64,176400);put16(h.data()+68,4);put16(h.data()+70,16);
    std::memcpy(h.data()+72,"data",4);put32(h.data()+76,rf64?UINT32_MAX:uint32_t(bytes));return h;
}
}
#endif
