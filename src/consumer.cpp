#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <windows.h>

#include <ringbuffer.hpp>

constexpr int TOTAL_MESSAGES = 1000000;

int main(){
    const char* SHM_NAME = "Local\\MySharedRingBuffer";
    size_t SHM_SIZE = sizeof(ringbuffer<1024>);

    std::cout << "[Consumer] Attempting to connect to the shared memory region..." << std::endl;

    HANDLE hMapFile = OpenFileMappingA(                     
        FILE_MAP_ALL_ACCESS,             
        false,
        SHM_NAME                   
    );

    if(hMapFile==NULL){
        std::cerr << "[Consumer] OpenFileMappingA failed! Error: " << GetLastError() << std::endl;
        std::cerr << "[Consumer] Make sure producer.exe is running first!" << std::endl;
        return 1;
    }

    std::cout << "[Consumer] Successfully opened shared memory handle: " << hMapFile << std::endl;

    void* raw_ptr = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,   
        0, 0,                   
        SHM_SIZE
    );

    if(raw_ptr == nullptr){
        std::cerr << "[Consumer] MapViewOfFile failed! Error: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    auto* ring = static_cast<ringbuffer<1024>*>(raw_ptr);

    std::cout << "[Consumer] Connected to ring buffer in shared memory." << std::endl;

    uint64_t total_latency_ns = 0;
    int received_count = 0;

    std::chrono::high_resolution_clock::time_point start_time;

    for(int i=1;i<=TOTAL_MESSAGES;i++){
        IPCMessage msg;

        while(!ring->pop(msg)){
            std::this_thread::yield();
        }

        received_count++;

        if(received_count == 1){
            start_time = std::chrono::high_resolution_clock::now();
        }
        else if(received_count>1){
            uint64_t now = std::chrono::steady_clock::now().time_since_epoch().count();
            total_latency_ns += (now - msg.timestamp);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();
    double avg_latency_ns = static_cast<double>(total_latency_ns) / (TOTAL_MESSAGES - 1);

    std::cout << std::endl << "================ BENCHMARK RESULTS ================" << std::endl;
    std::cout << " Received Messages : " << TOTAL_MESSAGES << std::endl;
    std::cout << " Total Time Taken  : " << elapsed_sec << " seconds" << std::endl;
    std::cout << " Throughput        : " << (TOTAL_MESSAGES / elapsed_sec) / 1e6 << " Million msg/sec" << std::endl;
    std::cout << " Avg Latency       : " << avg_latency_ns << " ns (" << (avg_latency_ns / 1000.0) << " µs)" << std::endl;
    std::cout << "===================================================" << std::endl;

    UnmapViewOfFile(raw_ptr);
    CloseHandle(hMapFile);

    std::cout << "[Consumer] Disconnecting from shared memory." << std::endl;

    return 0;

}