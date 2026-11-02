#ifndef ATRACODEC_CONVERTER_PROCESS_MACOS_H
#define ATRACODEC_CONVERTER_PROCESS_MACOS_H
#include "converter_common.h"
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern "C" char **environ;
namespace converter::mac {
struct Failure{int code;};
[[noreturn]]inline void fail(int e=errno){throw Failure{e};}
struct Fd{int n=-1;explicit Fd(int v=-1):n(v){}~Fd(){reset();}void reset(int v=-1){if(n>=0)close(n);n=v;}void high(){if(n<3){int copy=fcntl(n,F_DUPFD_CLOEXEC,3);if(copy<0)fail();reset(copy);}}Fd(const Fd&)=delete;Fd&operator=(const Fd&)=delete;};
struct Ticket{
 std::string directory,flag;
 explicit Ticket(std::string base){if(base.empty())fail(EINVAL);if(base.back()!='/')base+='/';base+="atracodec-XXXXXX";std::vector<char>b(base.begin(),base.end());b.push_back(0);if(!mkdtemp(b.data()))fail();
  try{directory=b.data();flag=directory+"/cancel";}catch(...){rmdir(b.data());throw;}}
 ~Ticket(){if(!flag.empty())unlink(flag.c_str());if(!directory.empty())rmdir(directory.c_str());}
 void signal(){Fd f(open(flag.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600));if(f.n<0&&errno!=EEXIST)fail();}
};
struct Actions{
 posix_spawn_file_actions_t value;bool ready=false;
 Actions(){int n=posix_spawn_file_actions_init(&value);if(n)fail(n);ready=true;}
 ~Actions(){if(ready)posix_spawn_file_actions_destroy(&value);}
 void dup(int from,int to){int n=posix_spawn_file_actions_adddup2(&value,from,to);if(n)fail(n);}
 void closeFd(int fd){int n=posix_spawn_file_actions_addclose(&value,fd);if(n)fail(n);}
};
struct SpawnAttributes{
 posix_spawnattr_t value;bool ready=false;
 SpawnAttributes(){int n=posix_spawnattr_init(&value);if(n)fail(n);ready=true;n=posix_spawnattr_setflags(&value,POSIX_SPAWN_CLOEXEC_DEFAULT);if(n){posix_spawnattr_destroy(&value);ready=false;fail(n);}}
 ~SpawnAttributes(){if(ready)posix_spawnattr_destroy(&value);}
};
struct Child{
 pid_t pid=-1;bool reaped=false;int status=0;
 bool check(){if(reaped)return true;pid_t n;do{n=waitpid(pid,&status,WNOHANG);}while(n<0&&errno==EINTR);if(n==pid){reaped=true;return true;}if(n<0){reaped=true;fail();}return false;}
 void signal(int n){if(pid>0&&!reaped&&!check()&&kill(pid,n)<0&&errno!=ESRCH)fail();}
 ~Child(){if(pid>0&&!reaped){kill(pid,SIGKILL);pid_t n;do{n=waitpid(pid,&status,0);}while(n<0&&errno==EINTR);}}
};
struct Job{std::string executable,input,output,temp_directory;std::size_t profile=default_profile;bool decode=false,overwrite=false;};
struct Result{int exit_code=-1,system_error=0;bool cancelled=false,forced=false;};
inline Result run(const Job&job,const std::function<void(const Event&)>&callback,std::atomic<bool>&cancel)noexcept{
 Result result;
 try{
  if(cancel.load()){result.cancelled=true;result.exit_code=130;return result;}
  Ticket ticket(job.temp_directory);Child child;int p[2];if(pipe(p))fail();Fd read(p[0]),write(p[1]);read.high();write.high();
  if(fcntl(read.n,F_SETFD,FD_CLOEXEC)<0||fcntl(write.n,F_SETFD,FD_CLOEXEC)<0)fail();
  if(fcntl(read.n,F_SETFL,fcntl(read.n,F_GETFL)|O_NONBLOCK)<0)fail();
  Fd input(open("/dev/null",O_RDONLY|O_CLOEXEC));if(input.n<0)fail();input.high();Actions actions;SpawnAttributes attributes;
  actions.dup(input.n,STDIN_FILENO);actions.dup(write.n,STDOUT_FILENO);actions.dup(write.n,STDERR_FILENO);
  actions.closeFd(read.n);actions.closeFd(write.n);actions.closeFd(input.n);
  const auto args=arguments(job.executable,job.decode,job.profile,job.input,job.output,job.overwrite,ticket.flag);if(args.empty()||job.executable.empty()||job.executable[0]!='/')fail(EINVAL);
  std::vector<char*>argv;for(const auto&s:args)argv.push_back(const_cast<char*>(s.c_str()));argv.push_back(nullptr);
  const int n=posix_spawn(&child.pid,job.executable.c_str(),&actions.value,&attributes.value,argv.data(),environ);if(n){child.pid=-1;fail(n);}write.reset();input.reset();
  using Clock=std::chrono::steady_clock;auto signalled_at=Clock::now(),last_progress=Clock::time_point{},terminated_at=Clock::time_point{};
  bool signalled=false,terminated=false,have_latest=false;Event latest;Lines lines;
  const auto emit=[&](const Event&e){if(e.kind==EventKind::Progress){latest=e;have_latest=true;const auto now=Clock::now();if(now-last_progress<std::chrono::milliseconds(50))return;last_progress=now;have_latest=false;}else if(have_latest){callback(latest);have_latest=false;}callback(e);};
  for(;;){const bool done=child.check();const auto now=Clock::now();
   if(!done&&cancel.load()&&!signalled){ticket.signal();signalled=true;signalled_at=now;}
   if(!done&&signalled&&!terminated&&now-signalled_at>std::chrono::seconds(3)){child.signal(SIGTERM);terminated=true;terminated_at=now;result.forced=true;}
   if(!done&&terminated&&now-terminated_at>std::chrono::seconds(1))child.signal(SIGKILL);
   bool got=false;for(unsigned batch=0;batch<32;++batch){char b[8192];ssize_t count=::read(read.n,b,sizeof(b));if(count>0){lines.feed(b,std::size_t(count),emit);got=true;continue;}if(count<0&&errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR)fail();break;}
   if(done&&!got)break;pollfd fd{read.n,POLLIN,0};if(!got&&poll(&fd,1,30)<0&&errno!=EINTR)fail();
  }
  lines.finish(emit);if(have_latest)callback(latest);result.exit_code=WIFEXITED(child.status)?WEXITSTATUS(child.status):WIFSIGNALED(child.status)?128+WTERMSIG(child.status):-1;result.cancelled=signalled&&result.exit_code!=0;
 }catch(const Failure&e){result.system_error=e.code;}catch(const std::bad_alloc&){result.system_error=ENOMEM;}catch(...){result.system_error=EIO;}
 return result;
}
inline bool same_file(const std::string&a,const std::string&b){struct stat x{},y{};return a==b||(!stat(a.c_str(),&x)&&!stat(b.c_str(),&y)&&x.st_dev==y.st_dev&&x.st_ino==y.st_ino);}
}
#endif
