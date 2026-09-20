#pragma once

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    // x86 / x86-64 (Intel & AMD)
    #include <immintrin.h>
    inline void cpu_pause() noexcept {
        _mm_pause();
    }
#elif defined(__aarch64__) || defined(_M_ARM64)
    // ARM64
    #if defined(_MSC_VER)
        #include <arm64_neon.h>
        inline void cpu_pause() noexcept {
            __yield();
        }
    #else
        inline void cpu_pause() noexcept {
            asm volatile("yield" ::: "memory");
        }
    #endif
#else
    #error "cpu_pause() is not supported on this CPU architecture."
#endif