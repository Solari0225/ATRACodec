#ifndef ATRACODEC_FENV_HPP
#define ATRACODEC_FENV_HPP
#include <cfenv>
#include <cstdint>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#endif
// Reused from the independently tested original-only encoder boundary.
namespace atracodec_detail {
class FloatingEnvironment {
    std::fenv_t saved{};bool valid=false;
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    uint32_t saved_mxcsr=0;
#elif defined(__aarch64__)
    uint64_t saved_fpcr=0,saved_fpsr=0;
#endif
public:
    FloatingEnvironment(){
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        saved_mxcsr=_mm_getcsr();
#elif defined(__aarch64__)
        __asm__ volatile("mrs %0, fpcr" : "=r"(saved_fpcr));
        __asm__ volatile("mrs %0, fpsr" : "=r"(saved_fpsr));
#endif
        if(std::fegetenv(&saved))
            throw std::runtime_error("Cannot initialize IEEE floating-point environment");
        valid=true;
        if(std::fesetenv(FE_DFL_ENV)){
            std::fesetenv(&saved);valid=false;
            throw std::runtime_error("Cannot initialize IEEE floating-point environment");
        }
        /* Some Windows CRT fenv implementations cover x87 only. The core
         * also uses binary32 SIMD arithmetic and needs nearest-even, gradual
         * underflow and masked traps, independent of the embedding host. */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        _mm_setcsr(0x1f80u);
#elif defined(__aarch64__)
        const uint64_t zero=0;
        __asm__ volatile("msr fpcr, %0\n\tmsr fpsr, %0" : : "r"(zero) : "memory");
#endif
    }
    ~FloatingEnvironment(){
        if(!valid)return;
        std::fesetenv(&saved);
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        _mm_setcsr(saved_mxcsr);
#elif defined(__aarch64__)
        __asm__ volatile("msr fpcr, %0\n\tmsr fpsr, %1" : : "r"(saved_fpcr),"r"(saved_fpsr) : "memory");
#endif
    }
};
}
#endif
