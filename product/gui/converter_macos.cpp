// Native AppKit controls, dynamically messaged through Apple's documented ABI.
// This source is compiled/cross-linked only by automated QA; GUI testing is manual.
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "converter_common.h"
#include "macos_abi.h"
#include "process_macos.h"

namespace {
using namespace cocoa;using converter::Text;using converter::Language;
struct Problem{std::string message;};struct App;App *application=nullptr;
Class root_class=nullptr;
void resized(Obj,Sel,Size);Bool flipped(Obj,Sel);void action(Obj,Sel,Obj);
Bool windowClose(Obj,Sel,Obj);UInteger appTerminate(Obj,Sel,Obj);Bool afterClose(Obj,Sel,Obj);
void received(Obj,Sel,Obj);void taskFinished(Obj,Sel,Obj);
// Apple documents entering Cocoa's threaded mode through NSThread before
// issuing Foundation calls from a separately created POSIX/C++ worker.
void warmup(Obj,Sel,Obj){Obj p=pool();drain(p);}

struct App{
 Obj app{},delegate{},root{},controller{},window{},operationLabel{},operation{},languageLabel{},language{},help{},inputLabel{},input{},outputLabel{},output{},browseInput{},browseOutput{},profileLabel{},profile{},hint{},overwrite{},start{},cancel{},progress{},status{},logLabel{},clear{},log{},scroll{},quitItem{},editItem{},editMenu{},cutItem{},copyItem{},pasteItem{},allItem{};
 Language locale=Language::English;Text statusKey=Text::Ready;converter::SessionState state;std::atomic<bool>cancelRequest{false};std::thread worker;converter::mac::Result workerResult;
 double width=860,height=660;bool controls=false,previousDecode=false,seenResult=false;std::string executable,tempDirectory;
 ~App(){if(worker.joinable()){cancelRequest.store(true);worker.join();}}
 const char*t(Text key)const{return converter::tr(locale,key);}Obj ts(Text key)const{return str(t(key));}
 bool decoding()const{return send<Integer>(operation,"indexOfSelectedItem")==1;}
 std::string value(Obj control)const{return text(send<Obj>(control,"stringValue"));}
 void title(Obj control,Text key){send<void>(control,"setTitle:",ts(key));}
 void label(Obj control,Text key){send<void>(control,"setStringValue:",ts(key));}
 void append(const std::string&s){if(s.empty())return;Obj storage=send<Obj>(log,"textStorage");if(send<UInteger>(storage,"length")>400000){send<void>(log,"setString:",str(t(Text::LogTrimmed)));}
  Obj line=send<Obj>(alloc("NSAttributedString"),"initWithString:",str((s+"\n").c_str()));send<void>(storage,"appendAttributedString:",line);release(line);send<void>(log,"scrollRangeToVisible:",Range{send<UInteger>(storage,"length"),0});}
 void showStatus(Text key){statusKey=key;label(status,key);}
 void enabled(){const Bool idle=!state.active;for(Obj v:{operation,language,input,output,browseInput,browseOutput,overwrite,start,clear})send<void>(v,"setEnabled:",idle);send<void>(profile,"setEnabled:",Bool(idle&&!decoding()));send<void>(cancel,"setEnabled:",Bool(state.active&&!state.cancel_requested));}
 Obj add(Obj v){if(!v)throw Problem{t(Text::TaskFailed)};send<void>(root,"addSubview:",v);return v;}
 Obj newLabel(bool wrapping=false){Obj v=send<Obj>(cls("NSTextField"),wrapping?"wrappingLabelWithString:":"labelWithString:",str(""));send<void>(v,"setFont:",send<Obj>(cls("NSFont"),"systemFontOfSize:",13.0));return add(v);}
 Obj newField(){Obj v=send<Obj>(alloc("NSTextField"),"initWithFrame:",rect(0,0,10,24));add(v);release(v);return v;}
 Obj button(const char*selectorName,bool check=false){Obj v=send<Obj>(cls("NSButton"),check?"checkboxWithTitle:target:action:":"buttonWithTitle:target:action:",str(""),delegate,selector(selectorName));if(check){send<void>(v,"setTarget:",Obj(nullptr));send<void>(v,"setAction:",Sel(nullptr));}return add(v);}
 Obj popup(const char*selectorName){Obj v=send<Obj>(alloc("NSPopUpButton"),"initWithFrame:pullsDown:",rect(0,0,10,26),No);send<void>(v,"setTarget:",delegate);send<void>(v,"setAction:",selector(selectorName));add(v);release(v);return v;}
 void frame(Obj v,double x,double y,double w,double h){send<void>(v,"setFrame:",rect(x,y,std::max(1.0,w),h));}
 void layout(){if(!controls)return;double m=20,w=width-2*m,button=145,gap=10;
  frame(operationLabel,m,22,95,22);frame(operation,m+100,15,280,28);frame(languageLabel,width-m-220,22,70,22);frame(language,width-m-145,15,145,28);
  frame(help,m,61,w,27);frame(inputLabel,m,98,w,22);frame(input,m,124,w-button-gap,27);frame(browseInput,width-m-button,123,button,30);
  frame(outputLabel,m,165,w,22);frame(output,m,191,w-button-gap,27);frame(browseOutput,width-m-button,190,button,30);
  frame(profileLabel,m,237,110,22);frame(profile,m+115,230,w-115,29);frame(hint,m,271,w,40);frame(overwrite,m,316,w,25);
  frame(start,m,359,130,34);frame(cancel,m+140,359,105,34);frame(progress,m+265,367,w-265,16);frame(status,m,408,w,25);
  frame(logLabel,m,445,w-120,24);frame(clear,width-m-110,438,110,30);frame(scroll,m,478,w,std::max(60.0,height-498));
 }
 void menus(){Obj bar=make("NSMenu"),appItem=make("NSMenuItem"),appMenu=make("NSMenu");editItem=make("NSMenuItem");editMenu=make("NSMenu");
  quitItem=send<Obj>(alloc("NSMenuItem"),"initWithTitle:action:keyEquivalent:",str(""),selector("terminate:"),str("q"));send<void>(quitItem,"setTarget:",app);send<void>(appMenu,"addItem:",quitItem);release(quitItem);send<void>(appItem,"setSubmenu:",appMenu);send<void>(bar,"addItem:",appItem);release(appMenu);release(appItem);
  auto item=[&](const char*sel,const char*key){Obj i=send<Obj>(alloc("NSMenuItem"),"initWithTitle:action:keyEquivalent:",str(""),selector(sel),str(key));send<void>(editMenu,"addItem:",i);release(i);return i;};cutItem=item("cut:","x");copyItem=item("copy:","c");pasteItem=item("paste:","v");allItem=item("selectAll:","a");
  send<void>(editItem,"setSubmenu:",editMenu);send<void>(bar,"addItem:",editItem);release(editItem);send<void>(app,"setMainMenu:",bar);release(bar);release(editMenu);
 }
 void translate(){const auto selected=send<Integer>(profile,"indexOfSelectedItem"),mode=send<Integer>(operation,"indexOfSelectedItem");title(window,Text::Title);label(operationLabel,Text::Operation);label(languageLabel,Text::Language);
  send<void>(operation,"removeAllItems");for(Text key:{Text::Encode,Text::Decode})send<void>(operation,"addItemWithTitle:",ts(key));send<void>(operation,"selectItemAtIndex:",Integer(mode==1?1:0));
  send<void>(profile,"removeAllItems");for(std::size_t i=0;i<converter::profile_count;++i)send<void>(profile,"addItemWithTitle:",str(converter::profile_label(i,locale).c_str()));send<void>(profile,"selectItemAtIndex:",selected>=0?selected:Integer(converter::default_profile));
  label(help,decoding()?Text::HelpDecode:Text::HelpEncode);label(inputLabel,decoding()?Text::InputOma:Text::InputWav);label(outputLabel,decoding()?Text::OutputWav:Text::OutputOma);label(profileLabel,Text::Profile);label(hint,Text::LosslessHint);
  for(auto p:{std::pair<Obj,Text>{browseInput,Text::BrowseInput},{browseOutput,Text::BrowseOutput},{overwrite,Text::Overwrite},{start,Text::Start},{cancel,Text::Cancel},{clear,Text::Clear},{quitItem,Text::Quit},{editMenu,Text::Edit},{editItem,Text::Edit},{cutItem,Text::Cut},{copyItem,Text::Copy},{pasteItem,Text::Paste},{allItem,Text::SelectAll}})title(p.first,p.second);
  label(logLabel,Text::Log);showStatus(statusKey);enabled();
 }
 void initialize(){app=send<Obj>(cls("NSApplication"),"sharedApplication");if(NSApp!=app)throw Problem{"NSApplication initialization failed"};send<Bool>(app,"setActivationPolicy:",Integer(0));
  Obj preferences=send<Obj>(cls("NSLocale"),"preferredLanguages");if(send<UInteger>(preferences,"count")){const auto language=text(send<Obj>(preferences,"objectAtIndex:",UInteger(0)));if(language.rfind("zh",0)==0)locale=Language::Chinese;}
  tempDirectory=text(NSTemporaryDirectory());Obj bundle=send<Obj>(cls("NSBundle"),"mainBundle");Obj cli=send<Obj>(send<Obj>(bundle,"executablePath"),"stringByDeletingLastPathComponent");executable=text(send<Obj>(cli,"stringByAppendingPathComponent:",str("atracodec")));
  Class d=objc_allocateClassPair(cls("NSObject"),"ATRACodecDelegate",0);if(!d)throw Problem{t(Text::TaskFailed)};
  for(const char*s:{"languageChanged:","modeChanged:","browseInput:","browseOutput:","start:","cancel:","clear:"})if(!class_addMethod(d,selector(s),reinterpret_cast<IMP>(action),"v@:@"))throw Problem{t(Text::TaskFailed)};
  class_addMethod(d,selector("windowShouldClose:"),reinterpret_cast<IMP>(windowClose),BoolObjectEncoding);class_addMethod(d,selector("applicationShouldTerminate:"),reinterpret_cast<IMP>(appTerminate),"Q@:@");class_addMethod(d,selector("applicationShouldTerminateAfterLastWindowClosed:"),reinterpret_cast<IMP>(afterClose),BoolObjectEncoding);class_addMethod(d,selector("receivedEvent:"),reinterpret_cast<IMP>(received),"v@:@");class_addMethod(d,selector("taskFinished:"),reinterpret_cast<IMP>(taskFinished),"v@:@");class_addMethod(d,selector("cocoaThreadWarmup:"),reinterpret_cast<IMP>(warmup),"v@:@");objc_registerClassPair(d);delegate=send<Obj>(send<Obj>(d,"alloc"),"init");send<void>(app,"setDelegate:",delegate);if(!send<Bool>(cls("NSThread"),"isMultiThreaded"))send<void>(cls("NSThread"),"detachNewThreadSelector:toTarget:withObject:",selector("cocoaThreadWarmup:"),delegate,Obj(nullptr));
  root_class=objc_allocateClassPair(cls("NSView"),"ATRACodecContent",0);if(!root_class)throw Problem{t(Text::TaskFailed)};class_addMethod(root_class,selector("setFrameSize:"),reinterpret_cast<IMP>(resized),"v@:{CGSize=dd}");class_addMethod(root_class,selector("isFlipped"),reinterpret_cast<IMP>(flipped),BoolEncoding);objc_registerClassPair(root_class);
  root=send<Obj>(send<Obj>(root_class,"alloc"),"initWithFrame:",rect(0,0,width,height));controller=make("NSViewController");send<void>(controller,"setView:",root);release(root);window=send<Obj>(cls("NSWindow"),"windowWithContentViewController:",controller);send<void>(window,"retain");send<void>(window,"setReleasedWhenClosed:",No);send<void>(window,"setDelegate:",delegate);send<void>(window,"setContentMinSize:",Size{760,590});send<void>(window,"setContentSize:",Size{width,height});release(controller);
  operationLabel=newLabel();operation=popup("modeChanged:");languageLabel=newLabel();language=popup("languageChanged:");for(const char*s:{"中文","English"})send<void>(language,"addItemWithTitle:",str(s));send<void>(language,"selectItemAtIndex:",Integer(locale==Language::Chinese?0:1));help=newLabel(true);inputLabel=newLabel();input=newField();browseInput=button("browseInput:");outputLabel=newLabel();output=newField();browseOutput=button("browseOutput:");profileLabel=newLabel();profile=popup("modeChanged:");hint=newLabel(true);overwrite=button("none:",true);start=button("start:");send<void>(start,"setKeyEquivalent:",str("\r"));cancel=button("cancel:");send<void>(cancel,"setKeyEquivalent:",str("\033"));progress=send<Obj>(alloc("NSProgressIndicator"),"initWithFrame:",rect(0,0,20,16));send<void>(progress,"setMinValue:",0.0);send<void>(progress,"setMaxValue:",1000.0);send<void>(progress,"setIndeterminate:",No);add(progress);release(progress);status=newLabel();logLabel=newLabel();clear=button("clear:");
  scroll=send<Obj>(alloc("NSScrollView"),"initWithFrame:",rect(0,0,400,100));send<void>(scroll,"setHasVerticalScroller:",Yes);send<void>(scroll,"setHasHorizontalScroller:",No);send<void>(scroll,"setBorderType:",UInteger(2));log=send<Obj>(alloc("NSTextView"),"initWithFrame:",rect(0,0,800,160));send<void>(log,"setEditable:",No);send<void>(log,"setSelectable:",Yes);send<void>(log,"setRichText:",No);send<void>(log,"setVerticallyResizable:",Yes);send<void>(log,"setHorizontallyResizable:",No);send<void>(log,"setAutoresizingMask:",UInteger(2));Obj container=send<Obj>(log,"textContainer");send<void>(container,"setWidthTracksTextView:",Yes);send<void>(scroll,"setDocumentView:",log);release(log);add(scroll);release(scroll);
  menus();controls=true;translate();layout();send<void>(window,"center");send<void>(window,"makeKeyAndOrderFront:",Obj(nullptr));send<void>(app,"activateIgnoringOtherApps:",Yes);
 }
 void modeChanged(){if(state.active)return;auto source=value(input),destination=value(output);if(destination.empty()||destination==converter::suggested_output(source,previousDecode))send<void>(output,"setStringValue:",str(source.empty()?"":converter::suggested_output(source,decoding()).c_str()));previousDecode=decoding();translate();}
 void browse(bool save){Obj panel=send<Obj>(cls(save?"NSSavePanel":"NSOpenPanel"),save?"savePanel":"openPanel");send<void>(panel,"setTitle:",ts(save?Text::OutputDialog:Text::InputDialog));if(!save){send<void>(panel,"setCanChooseFiles:",Yes);send<void>(panel,"setCanChooseDirectories:",No);send<void>(panel,"setAllowsMultipleSelection:",No);}const bool oma=save!=decoding();Obj types=send<Obj>(cls("NSArray"),"arrayWithObject:",str(oma?"oma":"wav"));send<void>(panel,"setAllowedFileTypes:",types);send<void>(panel,"setAllowsOtherFileTypes:",Yes);
  const auto old=value(save?output:input);if(!old.empty()){Obj path=str(old.c_str());if(save)send<void>(panel,"setNameFieldStringValue:",send<Obj>(path,"lastPathComponent"));send<void>(panel,"setDirectoryURL:",send<Obj>(cls("NSURL"),"fileURLWithPath:",send<Obj>(path,"stringByDeletingLastPathComponent")));}
  if(send<Integer>(panel,"runModal")!=1)return;const auto chosen=text(send<Obj>(send<Obj>(panel,"URL"),"path"));send<void>(save?output:input,"setStringValue:",str(chosen.c_str()));if(!save&&value(output).empty())send<void>(output,"setStringValue:",str(converter::suggested_output(chosen,decoding()).c_str()));
 }
 std::string path(Obj field){auto raw=value(field);if(raw.empty())return {};Obj s=send<Obj>(str(raw.c_str()),"stringByExpandingTildeInPath");auto p=text(s);if(p.empty()||p.find('\0')!=std::string::npos)throw Problem{t(Text::BadInput)};if(p[0]!='/'){char *cwd=getcwd(nullptr,0);if(!cwd)throw Problem{t(Text::BadInput)};p=std::string(cwd)+"/"+p;free(cwd);}return text(send<Obj>(str(p.c_str()),"stringByStandardizingPath"));}
 void post(const converter::Event&e){Obj p=pool();try{auto packet=std::make_unique<converter::Event>(e);Obj box=send<Obj>(cls("NSValue"),"valueWithPointer:",static_cast<const void*>(packet.get()));send<void>(box,"retain");send<void>(delegate,"performSelectorOnMainThread:withObject:waitUntilDone:",selector("receivedEvent:"),box,No);packet.release();drain(p);}catch(...){drain(p);throw;}}
 void begin(){if(state.active)return;const auto source=path(input),destination=path(output);if(source.empty()||destination.empty())throw Problem{t(Text::ChooseBoth)};struct stat in{},out{};if(stat(source.c_str(),&in)||!S_ISREG(in.st_mode)||access(source.c_str(),R_OK))throw Problem{t(Text::BadInput)};if(converter::mac::same_file(source,destination))throw Problem{t(Text::SameFile)};bool replace=send<Integer>(overwrite,"state")==1;if(!stat(destination.c_str(),&out)){if(S_ISDIR(out.st_mode))throw Problem{t(Text::OutputDirectory)};if(!replace)throw Problem{t(Text::OutputExists)};}if(access(executable.c_str(),X_OK))throw Problem{t(Text::MissingCli)};auto selected=send<Integer>(profile,"indexOfSelectedItem");if(selected<0||std::size_t(selected)>=converter::profile_count)throw Problem{t(Text::Profile)};
  converter::mac::Job job{executable,source,destination,tempDirectory,std::size_t(selected),decoding(),replace};send<void>(input,"setStringValue:",str(source.c_str()));send<void>(output,"setStringValue:",str(destination.c_str()));append(t(Text::Started));append(std::string(t(Text::Input))+source);append(std::string(t(Text::Output))+destination);cancelRequest.store(false);state.start();seenResult=false;enabled();showStatus(Text::Running);send<void>(progress,"setIndeterminate:",Yes);send<void>(progress,"startAnimation:",Obj(nullptr));
  try{worker=std::thread([this,job]{Obj p=pool();workerResult=converter::mac::run(job,[this](const converter::Event&e){post(e);},cancelRequest);send<void>(delegate,"performSelectorOnMainThread:withObject:waitUntilDone:",selector("taskFinished:"),Obj(nullptr),No);drain(p);});}catch(...){state.finish();enabled();send<void>(progress,"stopAnimation:",Obj(nullptr));showStatus(Text::Failed);throw Problem{t(Text::TaskFailed)};}
 }
 void stop(){if(state.cancel()){cancelRequest.store(true);enabled();showStatus(Text::Cancelling);}}
 void event(const converter::Event&e){if(e.kind==converter::EventKind::Progress){if(!state.cancel_requested){int n=converter::progress_units(e);send<void>(progress,"setIndeterminate:",Bool(n<0));if(n<0)send<void>(progress,"startAnimation:",Obj(nullptr));else{send<void>(progress,"stopAnimation:",Obj(nullptr));send<void>(progress,"setDoubleValue:",double(n));const auto s=std::string(t(Text::Running))+" "+std::to_string(n/10)+"% · "+t(Text::Written)+converter::byte_label(e.output_bytes);send<void>(status,"setStringValue:",str(s.c_str()));}}return;}if(e.kind==converter::EventKind::Result)seenResult=true;append(converter::event_text(e,locale));}
 void finished(){if(worker.joinable())worker.join();const auto result=workerResult;bool close=state.finish();enabled();send<void>(progress,"stopAnimation:",Obj(nullptr));send<void>(progress,"setIndeterminate:",No);if(result.cancelled){showStatus(Text::Cancelled);append(t(Text::Cancelled));}else if(result.exit_code==0&&!result.system_error){showStatus(Text::Completed);send<void>(progress,"setDoubleValue:",1000.0);if(!seenResult)append(t(Text::Completed));}else{showStatus(Text::Failed);append(t(Text::Failed));if(result.system_error)append(std::string(t(Text::Error))+strerror(result.system_error));}if(close)send<void>(app,"terminate:",Obj(nullptr));}
 bool mayClose(){if(!state.active)return true;if(state.close_pending)return false;Obj alert=make("NSAlert");send<void>(alert,"setMessageText:",ts(Text::CloseTitle));send<void>(alert,"setInformativeText:",ts(Text::CloseQuestion));send<Obj>(alert,"addButtonWithTitle:",ts(Text::KeepRunning));send<Obj>(alert,"addButtonWithTitle:",ts(Text::YesStop));const auto choice=send<Integer>(alert,"runModal");release(alert);if(choice!=1001)return false;if(!state.active)return true;state.close(true);cancelRequest.store(true);enabled();showStatus(Text::Cancelling);return false;}
 void error(const std::string&s){append(std::string(t(Text::Error))+s);showStatus(Text::Failed);}
};

void resized(Obj self,Sel,Size size){Super s{self,cls("NSView")};superSend<void>(&s,"setFrameSize:",size);if(application){application->width=size.width;application->height=size.height;application->layout();}}
Bool flipped(Obj,Sel){return Yes;}
void action(Obj,Sel sel,Obj sender){if(!application)return;try{auto&a=*application;if(sel==selector("cancel:")){a.stop();return;}if(a.state.active)return;if(sel==selector("languageChanged:")){a.locale=send<Integer>(a.language,"indexOfSelectedItem")==0?Language::Chinese:Language::English;a.translate();}else if(sel==selector("modeChanged:")){if(sender==a.operation)a.modeChanged();}else if(sel==selector("browseInput:"))a.browse(false);else if(sel==selector("browseOutput:"))a.browse(true);else if(sel==selector("start:"))a.begin();else if(sel==selector("clear:"))send<void>(a.log,"setString:",str(""));}catch(const Problem&e){application->error(e.message);}catch(...){application->error(application->t(Text::TaskFailed));}}
Bool windowClose(Obj,Sel,Obj){try{return application&&application->mayClose()?Yes:No;}catch(...){return No;}}
UInteger appTerminate(Obj,Sel,Obj){return windowClose(nullptr,nullptr,nullptr)?1:0;}
Bool afterClose(Obj,Sel,Obj){return Yes;}
void received(Obj,Sel,Obj box){std::unique_ptr<converter::Event>e(static_cast<converter::Event*>(send<void*>(box,"pointerValue")));release(box);if(application&&e)try{application->event(*e);}catch(...){application->error(application->t(Text::NoDisplay));}}
void taskFinished(Obj,Sel,Obj){if(application)try{application->finished();}catch(...){application->error(application->t(Text::TaskFailed));}}
}
int main(){Obj p=pool();int result=0;try{App a;application=&a;a.initialize();send<void>(a.app,"run");application=nullptr;}catch(...){application=nullptr;result=1;Obj alert=make("NSAlert");send<void>(alert,"setMessageText:",str("ATRACodec could not start. / 无法启动 ATRACodec。"));send<Integer>(alert,"runModal");release(alert);}drain(p);return result;}
