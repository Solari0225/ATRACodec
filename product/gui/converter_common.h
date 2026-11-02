#ifndef ATRACODEC_CONVERTER_COMMON_H
#define ATRACODEC_CONVERTER_COMMON_H
#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace converter {
enum class Language { Chinese, English };
enum class Text { Title, Language, Operation, Encode, Decode, HelpEncode, HelpDecode,
 InputWav, InputOma, OutputOma, OutputWav, BrowseInput, BrowseOutput, Profile,
 Overwrite, Start, Cancel, Clear, Log, Ready, Running, Cancelling, Cancelled,
 Completed, Failed, ChooseBoth, BadInput, SameFile, OutputExists, OutputDirectory,
 MissingCli, CloseQuestion, CloseTitle, YesStop, KeepRunning, InputDialog,
 OutputDialog, DialogFailed, Error, Started, Input, Output, LosslessHint,
 Written, LogTrimmed, FileTooLong, TaskFailed, NoDisplay, Quit, Edit, Cut,
 Copy, Paste, SelectAll, Lossy, Lossless, Count };
struct Translation {const char *zh,*en;};
inline constexpr Translation strings[] = {
 {"ATRACodec 音频转换器","ATRACodec Converter"}, {"语言","Language"},
 {"操作","Operation"}, {"编码 WAV → OMA","Encode WAV → OMA"},
 {"解码 OMA → WAV","Decode OMA → WAV"},
 {"选择 44.1 kHz、16 位、双声道 WAV 音频。","Choose a 44.1 kHz, 16-bit stereo WAV file."},
 {"将 OMA 音频还原为 WAV 文件。","Convert an OMA audio file to WAV."},
 {"输入 WAV","Input WAV"}, {"输入 OMA","Input OMA"},
 {"输出 OMA","Output OMA"}, {"输出 WAV","Output WAV"},
 {"选择输入…","Choose input…"}, {"选择输出…","Choose output…"},
 {"编码规格","Encoding format"}, {"允许覆盖已有输出文件","Replace an existing output file"},
 {"开始转换","Start"}, {"取消","Cancel"}, {"清空日志","Clear log"},
 {"运行记录","Activity"}, {"就绪","Ready"}, {"正在转换…","Converting…"},
 {"正在取消…","Cancelling…"}, {"已取消","Cancelled"}, {"转换完成","Conversion complete"},
 {"转换失败，请查看运行记录。","Conversion failed. See the activity log."},
 {"请先选择输入和输出文件。","Choose an input and an output file first."},
 {"输入文件不存在或无法读取。","The input file does not exist or cannot be read."},
 {"输入与输出必须是不同文件。","Input and output must be different files."},
 {"输出文件已存在。请选择新位置，或允许覆盖。","The output already exists. Choose another name or allow replacement."},
 {"输出位置必须包含文件名。","Choose an output filename, not a folder."},
 {"缺少转换程序。请使用完整的软件包。","The conversion program is missing. Use the complete application package."},
 {"转换尚未完成，是否取消并关闭？","Conversion is still running. Cancel it and close?"},
 {"关闭转换器","Close converter"}, {"取消并关闭","Cancel and close"}, {"继续转换","Keep converting"},
 {"选择输入音频","Choose input audio"}, {"选择输出位置","Choose output location"},
 {"无法打开文件选择窗口。","The file picker could not be opened."},
 {"错误：","Error: "}, {"开始转换","Starting conversion"},
 {"输入：","Input: "}, {"输出：","Output: "},
 {"AAL3 / AALP 为无损格式；所列码率指音频核心，文件实际码率会变化。","AAL3 / AALP are lossless. Their rate labels describe the audio core; file sizes vary."},
 {"已写入 ","Written "}, {"较早的运行记录已清理。","Earlier activity was cleared."},
 {"文件路径过长。","The file path is too long."},
 {"无法启动或完成转换任务。","The conversion could not be started or completed."},
 {"无法显示这条消息。","This message could not be displayed."},
 {"退出 ATRACodec","Quit ATRACodec"}, {"编辑","Edit"},
 {"剪切","Cut"}, {"复制","Copy"}, {"粘贴","Paste"}, {"全选","Select All"},
 {"有损","lossy"}, {"无损","lossless"}
};
static_assert(sizeof(strings)/sizeof(strings[0])==static_cast<unsigned>(Text::Count));
inline const char *tr(Language l,Text key){const auto &s=strings[static_cast<unsigned>(key)];return l==Language::Chinese?s.zh:s.en;}
struct Profile {const char *id,*codec;unsigned kbps;bool lossless;};
inline constexpr Profile profiles[] = {
 {"atrac3-66","ATRAC3",66,false},{"atrac3-105","ATRAC3",105,false},{"atrac3-132","ATRAC3",132,false},
 {"atrac3plus-48","ATRAC3plus",48,false},{"atrac3plus-64","ATRAC3plus",64,false},
 {"atrac3plus-96","ATRAC3plus",96,false},{"atrac3plus-128","ATRAC3plus",128,false},
 {"atrac3plus-160","ATRAC3plus",160,false},{"atrac3plus-192","ATRAC3plus",192,false},
 {"atrac3plus-256","ATRAC3plus",256,false},{"atrac3plus-320","ATRAC3plus",320,false},
 {"atrac3plus-352","ATRAC3plus",352,false},{"aal3-132","AAL3",132,true},
 {"aalp-48","AALP",48,true},{"aalp-64","AALP",64,true},{"aalp-96","AALP",96,true},
 {"aalp-128","AALP",128,true},{"aalp-160","AALP",160,true},{"aalp-192","AALP",192,true},
 {"aalp-256","AALP",256,true},{"aalp-320","AALP",320,true},{"aalp-352","AALP",352,true}
};
inline constexpr std::size_t profile_count=sizeof(profiles)/sizeof(profiles[0]),default_profile=21;
static_assert(profile_count==22);
inline std::string profile_label(std::size_t i,Language l){const auto&p=profiles[i];return std::string(p.codec)+" · "+std::to_string(p.kbps)+" kbps ("+tr(l,p.lossless?Text::Lossless:Text::Lossy)+")";}
inline bool valid_argument(const std::string &s){return s.find('\0')==std::string::npos;}
inline std::vector<std::string> arguments(const std::string &exe,bool decode,std::size_t profile,
 const std::string &input,const std::string &output,bool replace,const std::string &cancel_file){
 std::vector<std::string> a{exe,decode?"decode":"encode"};
 if(!decode){if(profile>=profile_count)return {};a.insert(a.end(),{"--profile",profiles[profile].id});}
 a.insert(a.end(),{input,output,"--progress","--cancel-file",cancel_file});if(replace)a.push_back("--overwrite");
 for(const auto&v:a)if(!valid_argument(v))return {};return a;
}
template<class String> String suggested_output(String input,bool decode){
 const auto slash=input.find_last_of(String{'/','\\'}),dot=input.find_last_of(typename String::value_type('.'));
 const auto begin=slash==String::npos?0:slash+1;
 if(dot!=String::npos&&dot>begin)input.resize(dot);
 const char *suffix=decode?".wav":".oma";while(*suffix)input.push_back(typename String::value_type(*suffix++));return input;
}
struct SessionState {
 bool active=false,cancel_requested=false,close_pending=false;
 bool start(){if(active)return false;active=true;cancel_requested=false;close_pending=false;return true;}
 bool cancel(){if(!active||cancel_requested)return false;cancel_requested=true;return true;}
 bool close(bool confirmed){if(!active)return true;if(confirmed){close_pending=true;cancel();}return false;}
 bool finish(){const bool close=close_pending;active=false;cancel_requested=false;close_pending=false;return close;}
};
enum class EventKind { Progress, Result, Error, Message };
struct Event {
 EventKind kind=EventKind::Message;std::string message,operation;
 uint64_t input_bytes=0,output_bytes=0,processed_frames=0,total_frames=0,pcm_frames=0;
};
inline int progress_units(const Event &e){if(!e.total_frames)return -1;const double p=double(e.processed_frames)/double(e.total_frames);return int(std::min(1.0,std::max(0.0,p))*1000);}
namespace detail {
struct Json {
 std::string_view s;std::size_t p=0;
 void ws(){while(p<s.size()&&(s[p]==' '||s[p]=='\t'||s[p]=='\r'||s[p]=='\n'))++p;}
 bool ch(char c){ws();if(p==s.size()||s[p]!=c)return false;++p;return true;}
 bool hex4(uint32_t &n){n=0;for(int i=0;i<4;++i){if(p==s.size())return false;char c=s[p++];unsigned d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:16;if(d>15)return false;n=n*16+d;}return true;}
 static void utf8(std::string&o,uint32_t c){if(c<128)o+=char(c);else if(c<2048){o+=char(0xc0|(c>>6));o+=char(0x80|(c&63));}else if(c<65536){o+=char(0xe0|(c>>12));o+=char(0x80|((c>>6)&63));o+=char(0x80|(c&63));}else{o+=char(0xf0|(c>>18));o+=char(0x80|((c>>12)&63));o+=char(0x80|((c>>6)&63));o+=char(0x80|(c&63));}}
 bool string(std::string &out){if(!ch('"'))return false;out.clear();while(p<s.size()){unsigned char c=s[p++];if(c=='"')return true;if(c<32)return false;if(c!='\\'){out+=char(c);continue;}if(p==s.size())return false;c=s[p++];switch(c){case'"':case'\\':case'/':out+=char(c);break;case'b':out+='\b';break;case'f':out+='\f';break;case'n':out+='\n';break;case'r':out+='\r';break;case't':out+='\t';break;case'u':{uint32_t cp=0;if(!hex4(cp))return false;if(cp>=0xd800&&cp<=0xdbff){if(p+2>s.size()||s[p++]!='\\'||s[p++]!='u')return false;uint32_t low;if(!hex4(low)||low<0xdc00||low>0xdfff)return false;cp=0x10000+(cp-0xd800)*1024+low-0xdc00;}else if(cp>=0xdc00&&cp<=0xdfff)return false;utf8(out,cp);break;}default:return false;}}return false;}
 bool number(uint64_t &v){ws();const auto first=p;v=0;while(p<s.size()&&s[p]>='0'&&s[p]<='9'){unsigned n=s[p++]-'0';if(v>(UINT64_MAX-n)/10)return false;v=v*10+n;}return p>first;}
 bool skip(unsigned depth=0){if(depth>16)return false;ws();if(p==s.size())return false;if(s[p]=='"'){std::string x;return string(x);}if(s[p]=='{'||s[p]=='['){const char close=s[p++]=='{'?'}':']';ws();if(p<s.size()&&s[p]==close){++p;return true;}for(;;){if(close=='}'){std::string key;if(!string(key)||!ch(':'))return false;}if(!skip(depth+1))return false;if(ch(close))return true;if(!ch(','))return false;}}const auto first=p;while(p<s.size()&&s[p]!=','&&s[p]!='}'&&s[p]!=']'&&s[p]!=' '&&s[p]!='\t'&&s[p]!='\r'&&s[p]!='\n')++p;return p>first;}
};
}
inline Event parse_line(std::string line){
 while(!line.empty()&&(line.back()=='\r'||line.back()=='\n'))line.pop_back();Event e;e.message=line;
 if(line.rfind("Error: ",0)==0){e.kind=EventKind::Error;e.message=line.substr(7);return e;}
 const auto first=line.find_first_not_of(" \t\r\n");
 if(first==std::string::npos||line.size()>65536||line[first]!='{')return e;
 detail::Json j{line};if(!j.ch('{'))return e;std::string kind,message,operation;bool output=false,frames=false,ok=true;
 if(!j.ch('}'))for(;;){std::string key;if(!j.string(key)||!j.ch(':')){ok=false;break;}
  if(key=="event")ok=j.string(kind);else if(key=="message"||key=="error")ok=j.string(message);else if(key=="operation")ok=j.string(operation);
  else if(key=="input_bytes")ok=j.number(e.input_bytes);else if(key=="output_bytes"){ok=j.number(e.output_bytes);output=true;}
  else if(key=="processed_frames")ok=j.number(e.processed_frames);else if(key=="total_frames")ok=j.number(e.total_frames);
  else if(key=="pcm_frames"){ok=j.number(e.pcm_frames);frames=true;}else ok=j.skip();
  if(!ok||j.ch('}'))break;if(!j.ch(',')){ok=false;break;}
 }
 j.ws();if(!ok||j.p!=line.size())return Event{EventKind::Message,line};e.operation=operation;
 if(kind=="progress"){e.kind=EventKind::Progress;e.message.clear();}
 else if(kind=="error"||!message.empty()){e.kind=EventKind::Error;e.message=message;}
 else if(kind=="result"||(output&&(frames||!operation.empty()))){e.kind=EventKind::Result;e.message.clear();}
 return e;
}
class Lines {
 std::string pending_;
public:
 template<class Callback>void feed(const char *data,std::size_t n,Callback cb){
  for(std::size_t i=0;i<n;++i){if(data[i]=='\n'){cb(parse_line(std::move(pending_)));pending_.clear();}
   else{pending_+=data[i];if(pending_.size()>=65536){cb(Event{EventKind::Message,std::move(pending_)});pending_.clear();}}}
 }
 template<class Callback>void finish(Callback cb){if(!pending_.empty())cb(parse_line(std::move(pending_)));pending_.clear();}
};
inline std::string byte_label(uint64_t n){const uint64_t unit=1024*1024;if(n<1024)return std::to_string(n)+" B";if(n<unit)return std::to_string(n/1024)+" KB";return std::to_string(n/unit)+"."+std::to_string((n%unit)*10/unit)+" MB";}
inline std::string event_text(const Event&e,Language l){
 if(e.kind==EventKind::Progress)return {};
 if(e.kind==EventKind::Result)return std::string(tr(l,Text::Completed))+" · "+tr(l,Text::Written)+byte_label(e.output_bytes);
 if(e.kind==EventKind::Error){if(e.message.find("Conversion cancelled")!=std::string::npos)return tr(l,Text::Cancelled);return std::string(tr(l,Text::Error))+e.message;}
 return e.message;
}
}
#endif
