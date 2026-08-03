#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <new>
#include <thread>
#include <windows.h>

#include <ringbuffer.hpp>

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

    for(int i=1;i<=10;i++){
        IPCMessage msg;
        msg.id = i;
        msg.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        snprintf(msg.mssg, sizeof(msg.mssg), "Hello from Producer! Msg #%d", i);
        // strncpy(msg.mssg, "Hello from Producer!", sizeof(msg.mssg)-1);
        // msg.mssg[sizeof(msg.mssg)-1] = '\0';

        while(!ring->push(msg)){
            std::this_thread::yield();
        }

        std::cout << "[Producer] Pushed: " << msg.mssg << " (ID: " << msg.id << ")" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[Producer] Finished sending 10 messages." << std::endl;

    std::cout << "[Producer] Press ENTER to destroy shared memory and exit..." << std::endl;
    std::cin.get();

    UnmapViewOfFile(raw_ptr);
    CloseHandle(hMapFile);
    std::cout << "[Producer] Shared memory closed." << std::endl;

    return 0;
}