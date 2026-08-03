#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <thread>
#include <windows.h>

#include <ringbuffer.hpp>

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

    for(int i=1;i<=10;i++){
        IPCMessage msg;

        while(!ring->pop(msg)){
            std::this_thread::yield();
        }

        uint64_t now = std::chrono::steady_clock::now().time_since_epoch().count();
        uint64_t latency = now-msg.timestamp;

        std::cout << "[Consumer] Popped: " << msg.mssg << " | ID: " << msg.id << " | Latency(µs): " << latency/1000.0 << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[Consumer] Finished receiving 10 messages." << std::endl;

    UnmapViewOfFile(raw_ptr);
    CloseHandle(hMapFile);
    std::cout << "[Consumer] Disconnecting from shared memory." << std::endl;

    return 0;

}