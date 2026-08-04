#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <new>
#include <thread>
#include <windows.h>

#include <ringbuffer.hpp>

constexpr int TOTAL_MESSAGES = 1000000;

int main(){
    const char* SHM_NAME = "Local\\MySharedRingBuffer";
    size_t SHM_SIZE = sizeof(ringbuffer<1024>);

    std::cout << "[Producer] Attempting to create shared memory region..." << std::endl;

    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,       
        NULL,                       
        PAGE_READWRITE,             
        0,                          
        static_cast<DWORD>(SHM_SIZE),
        SHM_NAME                   
    );

    if(hMapFile==NULL){
        std::cerr << "[Producer] CreateFileMappingA failed! Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "[Producer] Shared memory object created with handle: " << hMapFile << std::endl;

    void* raw_ptr = MapViewOfFile(
        hMapFile,               
        FILE_MAP_ALL_ACCESS,   
        0, 0,                   
        SHM_SIZE               
    );

    if (raw_ptr == nullptr) {
        std::cerr << "[Producer] MapViewOfFile failed! Error: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    auto* ring = static_cast<ringbuffer<1024>*>(raw_ptr);

    new (ring) ringbuffer<1024>();
    std::cout << "[Producer] Initialized ring buffer in shared memory." << std::endl;

    std::cout << "[Producer] Ready! Press ENTER to start streaming 1 Million messages..." << std::endl;
    std::cin.get();

    auto start_time = std::chrono::high_resolution_clock::now();

    for(int i=1;i<=TOTAL_MESSAGES;i++){
        IPCMessage msg;
        msg.id = i;
        msg.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        snprintf(msg.mssg, sizeof(msg.mssg), "Hello from Producer! Msg #%d", i);

        while(!ring->push(msg)){
            std::this_thread::yield();
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "[Producer] Finished pushing " << TOTAL_MESSAGES << " messages in " 
              << elapsed_sec << " seconds!" << std::endl;
    std::cout << "[Producer] Throughput: " << (TOTAL_MESSAGES / elapsed_sec) / 1e6 
              << " Million msg/sec" << std::endl;

    std::cout << std::endl << "[Producer] Press ENTER to destroy shared memory and exit..." << std::endl;
    std::cin.get();

    UnmapViewOfFile(raw_ptr);
    CloseHandle(hMapFile);

    std::cout << "[Producer] Shared memory closed." << std::endl;

    return 0;
}