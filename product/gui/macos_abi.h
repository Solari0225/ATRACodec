#ifndef ATRACODEC_MACOS_ABI_H
#define ATRACODEC_MACOS_ABI_H
// Small, source-declared Apple ABI surface; no redistributed SDK/runtime.
// Sources and exact ABI boundaries are recorded in README.md and references.json.
#include <cstdint>
#include <cstddef>
#include <string>
#include <type_traits>
namespace cocoa {
using Obj=void*;using Class=void*;using Sel=void*;using IMP=void(*)(void);
using Integer=long;using UInteger=unsigned long;
#if defined(__OBJC_BOOL_IS_BOOL) && __OBJC_BOOL_IS_BOOL
using Bool=bool;
#else
using Bool=signed char;
#endif
constexpr Bool Yes=1,No=0;
inline constexpr const char *BoolObjectEncoding=std::is_same_v<Bool,bool>?"B@:@":"c@:@";
inline constexpr const char *BoolEncoding=std::is_same_v<Bool,bool>?"B@:":"c@:";
struct Point{double x,y;};struct Size{double width,height;};
struct Rect{Point origin;Size size;};struct Range{UInteger location,length;};
struct Super{Obj receiver;Class super_class;};
static_assert(sizeof(void*)==8&&sizeof(Rect)==32&&sizeof(Size)==16&&sizeof(Range)==16);
extern "C" {
Class objc_getClass(const char*);Sel sel_registerName(const char*);
Class objc_allocateClassPair(Class,const char*,std::size_t);
void objc_registerClassPair(Class);Bool class_addMethod(Class,Sel,IMP,const char*);
void objc_msgSend(void);void objc_msgSendSuper(void);
extern Obj NSApp;Obj NSTemporaryDirectory(void);
}
inline Class cls(const char*n){return objc_getClass(n);}
inline Sel selector(const char*n){return sel_registerName(n);}
template<class R,class...Args>R send(Obj target,const char*name,Args...args){
 // Apple's runtime requires a cast to the actual method signature.
 // No aggregate RETURNS are used: Intel objc_msgSend_stret is unnecessary.
 static_assert(!std::is_class_v<R>&&!std::is_union_v<R>,"Aggregate returns require an explicit platform ABI");
 return reinterpret_cast<R(*)(Obj,Sel,Args...)>(objc_msgSend)(target,selector(name),args...);
}
template<class R,class...Args>R superSend(Super *target,const char*name,Args...args){return reinterpret_cast<R(*)(Super*,Sel,Args...)>(objc_msgSendSuper)(target,selector(name),args...);}
inline Obj str(const char*s){return send<Obj>(cls("NSString"),"stringWithUTF8String:",s);}
inline std::string text(Obj s){const char*p=send<const char*>(s,"UTF8String");return p?p:"";}
inline Rect rect(double x,double y,double w,double h){return {{x,y},{w,h}};}
inline Obj alloc(const char*n){return send<Obj>(cls(n),"alloc");}
inline Obj make(const char*n){return send<Obj>(alloc(n),"init");}
inline void release(Obj o){send<void>(o,"release");}
inline Obj pool(){return make("NSAutoreleasePool");}
inline void drain(Obj p){send<void>(p,"drain");}
}
#endif
