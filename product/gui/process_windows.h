#ifndef ATRACODEC_CONVERTER_PROCESS_WINDOWS_H
#define ATRACODEC_CONVERTER_PROCESS_WINDOWS_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>
#include "converter_common.h"
#include "gui_options.h"
namespace converter::win {
struct Handle {
 HANDLE h=nullptr;Handle()=default;explicit Handle(HANDLE x):h(x){}
 ~Handle(){reset();}Handle(const Handle&)=delete;Handle&operator=(const Handle&)=delete;
 void reset(HANDLE x=nullptr){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);h=x;}
};
struct Failure{DWORD code;};
[[noreturn]]inline void fail(DWORD n=GetLastError()){throw Failure{n};}
struct Ticket {
 std::wstring reservation,flag;
 Ticket(){
  wchar_t base[MAX_PATH+1]{},path[MAX_PATH+1]{};const DWORD n=GetTempPathW(MAX_PATH,base);
  if(!n||n>=MAX_PATH)fail(ERROR_FILENAME_EXCED_RANGE);
  for(unsigned i=0;i<32;++i){if(!GetTempFileNameW(base,L"atc",0,path))fail();
   try{reservation=path;flag=reservation+L".cancel";}catch(...){DeleteFileW(path);throw;}
   if(GetFileAttributesW(flag.c_str())==INVALID_FILE_ATTRIBUTES&&GetLastError()==ERROR_FILE_NOT_FOUND)return;
   DeleteFileW(reservation.c_str());reservation.clear();flag.clear();
  }fail(ERROR_FILE_EXISTS);
 }
 ~Ticket(){if(!flag.empty())DeleteFileW(flag.c_str());if(!reservation.empty())DeleteFileW(reservation.c_str());}
 void signal(){Handle file(CreateFileW(flag.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY,nullptr));if(file.h==INVALID_HANDLE_VALUE&&GetLastError()!=ERROR_FILE_EXISTS)fail();}
};
struct Attributes {
 std::vector<unsigned char> bytes;LPPROC_THREAD_ATTRIBUTE_LIST p=nullptr;HANDLE handles[2]{};
 Attributes(HANDLE output,HANDLE input){SIZE_T n=0;InitializeProcThreadAttributeList(nullptr,1,0,&n);if(!n)fail();bytes.resize(n);p=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(bytes.data());
  if(!InitializeProcThreadAttributeList(p,1,0,&n)){p=nullptr;fail();}handles[0]=output;handles[1]=input;
  if(!UpdateProcThreadAttribute(p,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof(handles),nullptr,nullptr)){const DWORD e=GetLastError();DeleteProcThreadAttributeList(p);p=nullptr;fail(e);}}
 ~Attributes(){if(p)DeleteProcThreadAttributeList(p);}
};
struct Child {
 Handle job,process,thread;
 Child():job(CreateJobObjectW(nullptr,nullptr)){if(!job.h)fail();JOBOBJECT_EXTENDED_LIMIT_INFORMATION v{};v.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;if(!SetInformationJobObject(job.h,JobObjectExtendedLimitInformation,&v,sizeof(v)))fail();}
 void force(){if(!TerminateJobObject(job.h,ERROR_CANCELLED)&&process.h)TerminateProcess(process.h,ERROR_CANCELLED);}
 ~Child(){if(process.h&&WaitForSingleObject(process.h,0)!=WAIT_OBJECT_0){force();WaitForSingleObject(process.h,INFINITE);}}
};
struct Job {std::wstring executable,directory,input,output;std::size_t profile=default_profile;bool decode=false,overwrite=false;};
struct Result {DWORD exit_code=DWORD(-1),system_error=0;bool cancelled=false,forced=false;};
inline std::wstring widen_ascii(const char *s){std::wstring r;while(*s)r+=wchar_t(*s++);return r;}
inline std::vector<std::wstring> argv(const Job&j,const std::wstring&flag){std::vector<std::wstring>a{j.executable,j.decode?L"decode":L"encode"};if(!j.decode){if(j.profile>=profile_count)fail(ERROR_INVALID_PARAMETER);a.insert(a.end(),{L"--profile",widen_ascii(profiles[j.profile].id)});}a.insert(a.end(),{j.input,j.output,L"--progress",L"--cancel-file",flag});if(j.overwrite)a.push_back(L"--overwrite");return a;}
inline Result run(const Job &j,const std::function<void(const Event&)>&callback,std::atomic<bool>&cancel)noexcept{
 Result result;
 try{
  if(cancel.load()){result.cancelled=true;result.exit_code=ERROR_CANCELLED;return result;}
  Ticket ticket;Child child;SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};HANDLE r=nullptr,w=nullptr;
  if(!CreatePipe(&r,&w,&sa,0))fail();Handle read(r),write(w);if(!SetHandleInformation(read.h,HANDLE_FLAG_INHERIT,0))fail();
  Handle input(CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));if(input.h==INVALID_HANDLE_VALUE)fail();
  Attributes attrs(write.h,input.h);STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;startup.StartupInfo.wShowWindow=SW_HIDE;startup.StartupInfo.hStdInput=input.h;startup.StartupInfo.hStdOutput=startup.StartupInfo.hStdError=write.h;startup.lpAttributeList=attrs.p;
  std::wstring command;for(const auto&arg:argv(j,ticket.flag)){if(!command.empty())command+=L' ';command+=test_gui::quote_argument(arg);}
  if(command.size()>=32767)fail(ERROR_FILENAME_EXCED_RANGE);PROCESS_INFORMATION pi{};
  if(!CreateProcessW(j.executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED|CREATE_UNICODE_ENVIRONMENT|EXTENDED_STARTUPINFO_PRESENT,nullptr,j.directory.c_str(),&startup.StartupInfo,&pi))fail();
  child.process.reset(pi.hProcess);child.thread.reset(pi.hThread);
  if(!AssignProcessToJobObject(child.job.h,child.process.h)){const DWORD e=GetLastError();TerminateProcess(child.process.h,e);WaitForSingleObject(child.process.h,INFINITE);fail(e);}
  write.reset();input.reset();if(ResumeThread(child.thread.h)==DWORD(-1))fail();child.thread.reset();
  Lines lines;bool pipe=true,signalled=false;ULONGLONG signalled_at=0,last_progress=0;Event latest;bool have_latest=false;
  const auto emit=[&](const Event&e){if(e.kind==EventKind::Progress){latest=e;have_latest=true;const auto now=GetTickCount64();if(now-last_progress<50)return;last_progress=now;have_latest=false;}else if(have_latest){callback(latest);have_latest=false;}callback(e);};
  for(;;){const DWORD state=WaitForSingleObject(child.process.h,0);if(state==WAIT_FAILED)fail();
   if(state!=WAIT_OBJECT_0&&cancel.load()&&!signalled){ticket.signal();signalled=true;signalled_at=GetTickCount64();}
   if(state!=WAIT_OBJECT_0&&signalled&&GetTickCount64()-signalled_at>3000&&!result.forced){child.force();result.forced=true;}
   DWORD available=0;if(pipe&&!PeekNamedPipe(read.h,nullptr,0,nullptr,&available,nullptr)){if(GetLastError()!=ERROR_BROKEN_PIPE)fail();pipe=false;}
   if(available){char data[8192];DWORD n=0;if(!ReadFile(read.h,data,std::min<DWORD>(available,sizeof(data)),&n,nullptr)){if(GetLastError()!=ERROR_BROKEN_PIPE)fail();pipe=false;}else lines.feed(data,n,emit);continue;}
   if(state==WAIT_OBJECT_0)break;WaitForSingleObject(child.process.h,30);
  }
  lines.finish(emit);if(have_latest)callback(latest);if(!GetExitCodeProcess(child.process.h,&result.exit_code))fail();result.cancelled=signalled&&result.exit_code!=0;
 }catch(const Failure&e){result.system_error=e.code;}catch(const std::bad_alloc&){result.system_error=ERROR_NOT_ENOUGH_MEMORY;}catch(...){result.system_error=ERROR_GEN_FAILURE;}
 return result;
}
inline bool same_file(const std::wstring&a,const std::wstring&b){
 if(CompareStringOrdinal(a.c_str(),-1,b.c_str(),-1,TRUE)==CSTR_EQUAL)return true;
 Handle x(CreateFileW(a.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr));
 Handle y(CreateFileW(b.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr));
 BY_HANDLE_FILE_INFORMATION i{},j{};return x.h!=INVALID_HANDLE_VALUE&&y.h!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(x.h,&i)&&GetFileInformationByHandle(y.h,&j)&&i.dwVolumeSerialNumber==j.dwVolumeSerialNumber&&i.nFileIndexHigh==j.nFileIndexHigh&&i.nFileIndexLow==j.nFileIndexLow;
}
}
#endif
