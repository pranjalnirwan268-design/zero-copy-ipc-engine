#pragma once

#if defined(_MSC_VER) || defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
    #include <immintrin.h>
    inline void cpu_pause() noexcept {
        _mm_pause();
    }
#elif defined(__aarch64__) || defined(_M_ARM64)
    inline void cpu_pause() noexcept {
        asm volatile("yield" ::: "memory");
    }
#else
    #include <thread>
    inline void cpu_pause() noexcept {
        std::this_thread::yield();
    }
#endif