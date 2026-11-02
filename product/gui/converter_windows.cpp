// Native Win32 converter. GUI execution is reserved for the user's manual test.
#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "converter_common.h"
#include "process_windows.h"

namespace {
using converter::Text;using converter::Language;
constexpr wchar_t ClassName[]=L"ATRACodecConverterWindow";
constexpr UINT EventMessage=WM_APP+1,DoneMessage=WM_APP+2;
enum Control{Input=100,Output,BrowseInput,BrowseOutput,Profile,Operation,Lang,Overwrite,Start,Cancel,Clear};
struct Problem{std::wstring message;};
std::wstring wide(const std::string&s){if(s.empty())return {};const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);if(!n)return L"?";std::wstring r(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),r.data(),n);return r;}
std::wstring value(HWND h){int n=GetWindowTextLengthW(h);std::wstring s(std::size_t(n)+1,0);GetWindowTextW(h,s.data(),n+1);s.resize(n);return s;}
std::wstring system_error(DWORD n,Language language){wchar_t *p=nullptr;DWORD size=FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,n,language==Language::English?MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US):MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED),reinterpret_cast<wchar_t*>(&p),0,nullptr);if(!size)size=FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,n,0,reinterpret_cast<wchar_t*>(&p),0,nullptr);std::wstring s=size?std::wstring(p,size):L"";if(p)LocalFree(p);while(!s.empty()&&(s.back()=='\r'||s.back()=='\n'))s.pop_back();return s+L" ("+std::to_wstring(n)+L")";}
std::wstring absolute(std::wstring s,Language language){if(s.size()>1&&s.front()=='"'&&s.back()=='"')s=s.substr(1,s.size()-2);if(s.empty())return {};const DWORD n=GetFullPathNameW(s.c_str(),0,nullptr,nullptr);if(!n||n>32768)throw Problem{wide(converter::tr(language,n>32768?Text::FileTooLong:Text::BadInput))};std::wstring r(n,0);const DWORD written=GetFullPathNameW(s.c_str(),n,r.data(),nullptr);if(!written||written>=n)throw Problem{wide(converter::tr(language,n>32768?Text::FileTooLong:Text::BadInput))};r.resize(written);return r;}
std::wstring module_path(Language language){std::vector<wchar_t>b(512);while(b.size()<=32768){DWORD n=GetModuleFileNameW(nullptr,b.data(),DWORD(b.size()));if(!n)break;if(n<b.size())return {b.data(),n};b.resize(b.size()*2);}throw Problem{wide(converter::tr(language,Text::MissingCli))};}
template<class F>F api(const char*name){F f{};FARPROC p=GetProcAddress(GetModuleHandleW(L"user32.dll"),name);static_assert(sizeof(f)==sizeof(p));std::memcpy(&f,&p,sizeof(f));return f;}
struct Task{HWND window;converter::win::Job job;std::atomic<bool>*cancel;};
DWORD WINAPI worker(void *argument){std::unique_ptr<Task>task(static_cast<Task*>(argument));auto result=converter::win::run(task->job,[&](const converter::Event&e){auto *copy=new converter::Event(e);if(!PostMessageW(task->window,EventMessage,0,reinterpret_cast<LPARAM>(copy)))delete copy;},*task->cancel);const auto flags=(ULONG_PTR(result.system_error)<<2)|(result.cancelled?1:0)|(result.forced?2:0);PostMessageW(task->window,DoneMessage,result.exit_code,LPARAM(flags));return 0;}

struct App{
 HWND window{},operationLabel{},operation{},languageLabel{},language{},help{},inputLabel{},input{},outputLabel{},output{},browseInput{},browseOutput{},profileLabel{},profile{},hint{},overwrite{},start{},cancel{},progress{},status{},logLabel{},clear{},log{};
 HFONT font{};UINT dpi=96;Language locale=PRIMARYLANGID(GetUserDefaultUILanguage())==LANG_CHINESE?Language::Chinese:Language::English;
 converter::SessionState state;std::atomic<bool>cancelRequest{false};converter::win::Handle thread;std::wstring executable,directory;Text statusKey=Text::Ready;bool previousDecode=false,seenResult=false;
 ~App(){if(state.active){cancelRequest.store(true);if(thread.h)WaitForSingleObject(thread.h,INFINITE);}if(font)DeleteObject(font);}
 int px(int x)const{return MulDiv(x,int(dpi),96);}std::wstring t(Text key)const{return wide(converter::tr(locale,key));}
 bool decoding()const{return SendMessageW(operation,CB_GETCURSEL,0,0)==1;}
 void set(HWND h,Text key){SetWindowTextW(h,t(key).c_str());}
 void append(std::wstring s){if(GetWindowTextLengthW(log)>400000)SetWindowTextW(log,(t(Text::LogTrimmed)+L"\r\n").c_str());s+=L"\r\n";const auto n=GetWindowTextLengthW(log);SendMessageW(log,EM_SETSEL,n,n);SendMessageW(log,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(s.c_str()));SendMessageW(log,EM_SCROLLCARET,0,0);}
 void bar(bool moving){const auto style=GetWindowLongPtrW(progress,GWL_STYLE);if(moving){SetWindowLongPtrW(progress,GWL_STYLE,style|PBS_MARQUEE);SendMessageW(progress,PBM_SETMARQUEE,TRUE,30);}else{SendMessageW(progress,PBM_SETMARQUEE,FALSE,0);SetWindowLongPtrW(progress,GWL_STYLE,style&~LONG_PTR(PBS_MARQUEE));}}
 void showStatus(Text key){statusKey=key;set(status,key);}
 void enabled(){const bool idle=!state.active;for(HWND h:{operation,language,input,output,browseInput,browseOutput,overwrite,start,clear})EnableWindow(h,idle);EnableWindow(profile,idle&&!decoding());EnableWindow(profileLabel,!decoding());EnableWindow(cancel,state.active&&!state.cancel_requested);}
 void translate(){
  const auto selection=SendMessageW(profile,CB_GETCURSEL,0,0);const auto mode=SendMessageW(operation,CB_GETCURSEL,0,0);
  SetWindowTextW(window,t(Text::Title).c_str());set(operationLabel,Text::Operation);set(languageLabel,Text::Language);
  SendMessageW(operation,CB_RESETCONTENT,0,0);for(Text key:{Text::Encode,Text::Decode}){auto s=t(key);SendMessageW(operation,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s.c_str()));}SendMessageW(operation,CB_SETCURSEL,mode==1?1:0,0);
  SendMessageW(profile,CB_RESETCONTENT,0,0);for(std::size_t i=0;i<converter::profile_count;++i){const auto label=wide(converter::profile_label(i,locale));SendMessageW(profile,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(label.c_str()));}SendMessageW(profile,CB_SETCURSEL,selection>=0?selection:converter::default_profile,0);
  set(help,decoding()?Text::HelpDecode:Text::HelpEncode);set(inputLabel,decoding()?Text::InputOma:Text::InputWav);set(outputLabel,decoding()?Text::OutputWav:Text::OutputOma);
  set(browseInput,Text::BrowseInput);set(browseOutput,Text::BrowseOutput);set(profileLabel,Text::Profile);set(hint,Text::LosslessHint);set(overwrite,Text::Overwrite);set(start,Text::Start);set(cancel,Text::Cancel);set(logLabel,Text::Log);set(clear,Text::Clear);showStatus(statusKey);enabled();
 }
 void fonts(){HFONT old=font;NONCLIENTMETRICSW m{};m.cbSize=sizeof(m);LOGFONTW f{};if(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(m),&m,0))f=m.lfMessageFont;else{f.lfHeight=-13;lstrcpynW(f.lfFaceName,L"Segoe UI",LF_FACESIZE);}f.lfHeight=-MulDiv(10,int(dpi),72);font=CreateFontIndirectW(&f);for(HWND h:{operationLabel,operation,languageLabel,language,help,inputLabel,input,outputLabel,output,browseInput,browseOutput,profileLabel,profile,hint,overwrite,start,cancel,status,logLabel,clear,log})SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);if(old)DeleteObject(old);}
 HWND child(const wchar_t*cls,DWORD style,int id=0,DWORD ex=0){auto h=CreateWindowExW(ex,cls,L"",WS_CHILD|WS_VISIBLE|style,0,0,0,0,window,reinterpret_cast<HMENU>(UINT_PTR(id)),GetModuleHandleW(nullptr),nullptr);if(!h)throw Problem{t(Text::TaskFailed)};return h;}
 void create(){using GetDpi=UINT(WINAPI*)(HWND);if(auto f=api<GetDpi>("GetDpiForWindow"))dpi=f(window);executable=test_gui::companion_executable(module_path(locale));directory=executable.substr(0,executable.find_last_of(L"\\/"));
  operationLabel=child(L"STATIC",SS_LEFT);operation=child(L"COMBOBOX",WS_TABSTOP|CBS_DROPDOWNLIST,Operation);
  languageLabel=child(L"STATIC",SS_LEFT);language=child(L"COMBOBOX",WS_TABSTOP|CBS_DROPDOWNLIST,Lang);for(const wchar_t*s:{L"中文",L"English"})SendMessageW(language,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s));SendMessageW(language,CB_SETCURSEL,locale==Language::Chinese?0:1,0);
  help=child(L"STATIC",SS_LEFT);inputLabel=child(L"STATIC",SS_LEFT);input=child(L"EDIT",WS_TABSTOP|ES_AUTOHSCROLL,Input,WS_EX_CLIENTEDGE);browseInput=child(L"BUTTON",WS_TABSTOP|BS_PUSHBUTTON,BrowseInput);
  outputLabel=child(L"STATIC",SS_LEFT);output=child(L"EDIT",WS_TABSTOP|ES_AUTOHSCROLL,Output,WS_EX_CLIENTEDGE);browseOutput=child(L"BUTTON",WS_TABSTOP|BS_PUSHBUTTON,BrowseOutput);
  profileLabel=child(L"STATIC",SS_LEFT);profile=child(L"COMBOBOX",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,Profile);hint=child(L"STATIC",SS_LEFT);overwrite=child(L"BUTTON",WS_TABSTOP|BS_AUTOCHECKBOX,Overwrite);
  start=child(L"BUTTON",WS_TABSTOP|BS_DEFPUSHBUTTON,Start);cancel=child(L"BUTTON",WS_TABSTOP|BS_PUSHBUTTON,Cancel);progress=child(PROGRESS_CLASSW,PBS_SMOOTH|PBS_MARQUEE);SendMessageW(progress,PBM_SETRANGE32,0,1000);
  status=child(L"STATIC",SS_LEFT);logLabel=child(L"STATIC",SS_LEFT);clear=child(L"BUTTON",WS_TABSTOP|BS_PUSHBUTTON,Clear);log=child(L"EDIT",WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,0,WS_EX_CLIENTEDGE);
  SendMessageW(log,EM_SETLIMITTEXT,600000,0);SendMessageW(input,EM_SETLIMITTEXT,32767,0);SendMessageW(output,EM_SETLIMITTEXT,32767,0);fonts();translate();layout();
 }
 void layout(){RECT r{};GetClientRect(window,&r);const int m=px(20),w=r.right-2*m,button=px(145),gap=px(10);auto put=[&](HWND h,int x,int y,int width,int height){MoveWindow(h,x,px(y),std::max(1,width),px(height),TRUE);};
  put(operationLabel,m,19,px(95),24);put(operation,m+px(100),14,px(280),200);put(languageLabel,r.right-m-px(220),19,px(70),24);put(language,r.right-m-px(145),14,px(145),150);
  put(help,m,57,w,26);put(inputLabel,m,93,w,22);put(input,m,117,w-button-gap,30);put(browseInput,r.right-m-button,117,button,30);
  put(outputLabel,m,159,w,22);put(output,m,183,w-button-gap,30);put(browseOutput,r.right-m-button,183,button,30);
  put(profileLabel,m,229,px(105),25);put(profile,m+px(110),222,w-px(110),440);put(hint,m,263,w,36);put(overwrite,m,307,w,25);
  put(start,m,348,px(130),34);put(cancel,m+px(140),348,px(105),34);put(progress,m+px(265),355,w-px(265),20);put(status,m,395,w,25);
  put(logLabel,m,431,w-px(120),24);put(clear,r.right-m-px(110),425,px(110),28);MoveWindow(log,m,px(460),std::max(1,w),std::max(px(60),int(r.bottom)-px(460)-m),TRUE);
 }
 void modeChanged(){if(state.active)return;const auto source=value(input),destination=value(output);const auto oldSuggestion=converter::suggested_output(source,previousDecode);const bool mode=decoding();if(destination.empty()||destination==oldSuggestion)SetWindowTextW(output,source.empty()?L"":converter::suggested_output(source,mode).c_str());previousDecode=mode;translate();}
 void browse(bool save){std::vector<wchar_t> buffer(32768,0);auto old=value(save?output:input);if(old.size()<buffer.size())std::copy(old.begin(),old.end(),buffer.begin());OPENFILENAMEW d{};d.lStructSize=sizeof(d);d.hwndOwner=window;d.lpstrFile=buffer.data();d.nMaxFile=DWORD(buffer.size());const bool oma=save!=decoding();std::wstring title=t(save?Text::OutputDialog:Text::InputDialog);d.lpstrTitle=title.c_str();d.lpstrFilter=locale==Language::Chinese?(oma?L"OMA (*.oma)\0*.oma\0所有文件 (*.*)\0*.*\0\0":L"WAV (*.wav)\0*.wav\0所有文件 (*.*)\0*.*\0\0"):(oma?L"OMA (*.oma)\0*.oma\0All files (*.*)\0*.*\0\0":L"WAV (*.wav)\0*.wav\0All files (*.*)\0*.*\0\0");d.lpstrDefExt=oma?L"oma":L"wav";d.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?0u:OFN_FILEMUSTEXIST);if(save&&SendMessageW(overwrite,BM_GETCHECK,0,0)==BST_CHECKED)d.Flags|=OFN_OVERWRITEPROMPT;
  if(!(save?GetSaveFileNameW(&d):GetOpenFileNameW(&d))){if(CommDlgExtendedError())throw Problem{t(Text::DialogFailed)};return;}SetWindowTextW(save?output:input,buffer.data());if(!save&&value(output).empty())SetWindowTextW(output,converter::suggested_output(std::wstring(buffer.data()),decoding()).c_str());
 }
 void begin(){if(state.active)return;auto source=absolute(value(input),locale),destination=absolute(value(output),locale);if(source.empty()||destination.empty())throw Problem{t(Text::ChooseBoth)};const DWORD in=GetFileAttributesW(source.c_str());if(in==INVALID_FILE_ATTRIBUTES||(in&FILE_ATTRIBUTE_DIRECTORY))throw Problem{t(Text::BadInput)};if(converter::win::same_file(source,destination))throw Problem{t(Text::SameFile)};const DWORD out=GetFileAttributesW(destination.c_str());const bool replace=SendMessageW(overwrite,BM_GETCHECK,0,0)==BST_CHECKED;if(out!=INVALID_FILE_ATTRIBUTES){if(out&FILE_ATTRIBUTE_DIRECTORY)throw Problem{t(Text::OutputDirectory)};if(!replace)throw Problem{t(Text::OutputExists)};}const DWORD cli=GetFileAttributesW(executable.c_str());if(cli==INVALID_FILE_ATTRIBUTES||(cli&FILE_ATTRIBUTE_DIRECTORY))throw Problem{t(Text::MissingCli)};
  const auto selected=SendMessageW(profile,CB_GETCURSEL,0,0);if(!decoding()&&(selected<0||std::size_t(selected)>=converter::profile_count))throw Problem{t(Text::Profile)};auto task=std::make_unique<Task>();task->window=window;task->cancel=&cancelRequest;task->job={executable,directory,source,destination,std::size_t(selected<0?0:selected),decoding(),replace};SetWindowTextW(input,source.c_str());SetWindowTextW(output,destination.c_str());append(t(Text::Started));append(t(Text::Input)+source);append(t(Text::Output)+destination);cancelRequest.store(false);state.start();seenResult=false;enabled();showStatus(Text::Running);SendMessageW(progress,PBM_SETPOS,0,0);bar(true);
  HANDLE h=CreateThread(nullptr,0,worker,task.get(),0,nullptr);if(!h){const auto error=GetLastError();state.finish();enabled();bar(false);showStatus(Text::Failed);throw Problem{t(Text::TaskFailed)+L" "+system_error(error,locale)};}task.release();thread.reset(h);
 }
 void stop(){if(state.cancel()){cancelRequest.store(true);enabled();showStatus(Text::Cancelling);}}
 void event(const converter::Event&e){if(e.kind==converter::EventKind::Progress){if(!state.cancel_requested){const int n=converter::progress_units(e);bar(n<0);if(n>=0){SendMessageW(progress,PBM_SETPOS,n,0);SetWindowTextW(status,(t(Text::Running)+L" "+std::to_wstring(n/10)+L"% · "+t(Text::Written)+wide(converter::byte_label(e.output_bytes))).c_str());}}return;}if(e.kind==converter::EventKind::Result)seenResult=true;const auto s=converter::event_text(e,locale);if(!s.empty())append(wide(s));}
 void finished(DWORD code,ULONG_PTR flags){if(thread.h){WaitForSingleObject(thread.h,INFINITE);thread.reset();}const bool close=state.finish();enabled();bar(false);const DWORD error=DWORD(flags>>2);if(flags&1){showStatus(Text::Cancelled);append(t(Text::Cancelled));}else if(!error&&code==0){showStatus(Text::Completed);SendMessageW(progress,PBM_SETPOS,1000,0);if(!seenResult)append(t(Text::Completed));}else{showStatus(Text::Failed);append(t(Text::Failed));if(error)append(system_error(error,locale));}if(close)PostMessageW(window,WM_CLOSE,0,0);}
 bool mayClose(){if(!state.active)return true;if(state.close_pending)return false;const bool yes=MessageBoxW(window,t(Text::CloseQuestion).c_str(),t(Text::CloseTitle).c_str(),MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)==IDYES;if(!yes)return false;if(!state.active)return true;state.close(true);cancelRequest.store(true);enabled();showStatus(Text::Cancelling);return false;}
};
LRESULT CALLBACK procedure(HWND h,UINT m,WPARAM w,LPARAM l){auto*a=reinterpret_cast<App*>(GetWindowLongPtrW(h,GWLP_USERDATA));try{
 if(m==WM_NCCREATE){a=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);a->window=h;SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(a));}if(!a)return DefWindowProcW(h,m,w,l);
 switch(m){case WM_CREATE:a->create();return 0;case WM_SIZE:if(a->log)a->layout();return 0;case DM_GETDEFID:return MAKELONG(Start,DC_HASDEFID);
 case WM_DPICHANGED:{a->dpi=HIWORD(w);a->fonts();auto*r=reinterpret_cast<RECT*>(l);SetWindowPos(h,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOACTIVATE|SWP_NOZORDER);a->layout();return 0;}
 case WM_GETMINMAXINFO:reinterpret_cast<MINMAXINFO*>(l)->ptMinTrackSize={a->px(760),a->px(600)};return 0;
 case WM_COMMAND:if(HIWORD(w)==CBN_SELCHANGE){if(LOWORD(w)==Lang&&!a->state.active){a->locale=SendMessageW(a->language,CB_GETCURSEL,0,0)==0?Language::Chinese:Language::English;a->translate();}if(LOWORD(w)==Operation)a->modeChanged();return 0;}if(HIWORD(w)==BN_CLICKED){switch(LOWORD(w)){case BrowseInput:if(!a->state.active)a->browse(false);break;case BrowseOutput:if(!a->state.active)a->browse(true);break;case Start:a->begin();break;case Cancel:a->stop();break;case Clear:if(!a->state.active)SetWindowTextW(a->log,L"");break;}}return 0;
 case EventMessage:{std::unique_ptr<converter::Event>e(reinterpret_cast<converter::Event*>(l));if(e)a->event(*e);return 0;}
 case DoneMessage:a->finished(DWORD(w),ULONG_PTR(l));return 0;case WM_CLOSE:if(a->mayClose())DestroyWindow(h);return 0;case WM_DESTROY:PostQuitMessage(0);return 0;}
 }catch(const Problem&e){if(a&&a->log){a->append(a->t(Text::Error)+e.message);a->showStatus(Text::Failed);}else MessageBoxW(h,e.message.c_str(),L"ATRACodec",MB_OK|MB_ICONERROR);if(m==WM_CREATE)return -1;}catch(...){if(a&&a->log)a->append(a->t(Text::TaskFailed));if(m==WM_CREATE)return -1;}return DefWindowProcW(h,m,w,l);}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int show){try{INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_PROGRESS_CLASS};if(!InitCommonControlsEx(&controls))return 1;App app;WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);cls.lpfnWndProc=procedure;cls.hInstance=instance;cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hIcon=cls.hIconSm=LoadIconW(nullptr,IDI_APPLICATION);cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);cls.lpszClassName=ClassName;if(!RegisterClassExW(&cls))return 1;HWND window=CreateWindowExW(WS_EX_CONTROLPARENT,ClassName,app.t(Text::Title).c_str(),WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,900,760,nullptr,nullptr,instance,&app);if(!window)return 1;RECT r{0,0,app.px(860),app.px(650)};using Adjust=BOOL(WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);if(auto f=api<Adjust>("AdjustWindowRectExForDpi"))f(&r,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_CONTROLPARENT,app.dpi);else AdjustWindowRectEx(&r,WS_OVERLAPPEDWINDOW,FALSE,WS_EX_CONTROLPARENT);MONITORINFO mon{};mon.cbSize=sizeof(mon);int width=r.right-r.left,height=r.bottom-r.top;if(GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&mon)){width=std::min(width,int(mon.rcWork.right-mon.rcWork.left));height=std::min(height,int(mon.rcWork.bottom-mon.rcWork.top));}SetWindowPos(window,nullptr,0,0,width,height,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);ShowWindow(window,show);UpdateWindow(window);MSG msg{};BOOL status=0;while((status=GetMessageW(&msg,nullptr,0,0))>0)if(!IsDialogMessageW(window,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}return status<0?1:int(msg.wParam);}catch(...){MessageBoxW(nullptr,L"ATRACodec could not start. / 无法启动 ATRACodec。",L"ATRACodec",MB_OK|MB_ICONERROR);return 1;}}
