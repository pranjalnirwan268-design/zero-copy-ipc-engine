#pragma once

#include <cstdint>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <pthread.h>
    #include <sched.h>
#endif

inline bool pin_thread_to_core(uint32_t core_id) noexcept{
#if defined(_WIN32) || defined(_WIN64)
    if (core_id >= sizeof(DWORD_PTR) * 8) {
        std::cerr << "[Affinity] Core ID " << core_id << " exceeds mask limit (" 
                  << (sizeof(DWORD_PTR) * 8) << ")." << std::endl;
        return false;
    }
    HANDLE thread = GetCurrentThread();
    DWORD_PTR mask = static_cast<DWORD_PTR>(1ULL << core_id);
    DWORD_PTR result = SetThreadAffinityMask(thread, mask);
    if(result == 0){
        std::cerr << "[Affinity] Failed to pin thread to core " << core_id
                  << " (Error code: " << GetLastError() << ")" << std::endl;
        return false;
    }
#else
    if (core_id >= CPU_SETSIZE) {
        std::cerr << "[Affinity] Core ID " << core_id << " exceeds CPU_SETSIZE (" 
                  << CPU_SETSIZE << ")." << std::endl;
        return false;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if(rc != 0){
        std::cerr << "[Affinity] Failed to pin thread to core " << core_id
                  << " (rc: " << rc << ")" << std::endl;
        return false;
    }
#endif
    std::cout << "[Affinity] Successfully pinned thread to core " << core_id << std::endl;
    return true;
}

