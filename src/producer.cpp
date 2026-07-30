#include <cstddef>
#include <iostream>
#include <new>
#include <string>
#include <windows.h>

#include <ringbuffer.hpp>

int main(){
    char* SHM_NAME = "Local\\MySharedRingBuffer";
    size_t SHM_SIZE = sizeof(ringbuffer<1024>);

    std::cout << "[Producer] Attempting to create shared memory region..." << std::endl;

    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,       
        NULL,                       
        PAGE_READWRITE,             
        0,                          
        SHM_SIZE,
        SHM_NAME                   
    );

    if(hMapFile==NULL){
        std::cout << "[Producer] CreateFileMappingA failed! Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "[Producer] Shared memory object created with handle: " << hMapFile << "\n";

    void* raw_ptr = MapViewOfFile(
        hMapFile,               
        FILE_MAP_ALL_ACCESS,   
        0, 0,                   
        SHM_SIZE               
    );

    if (raw_ptr == nullptr) {
        std::cerr << "[Producer] MapViewOfFile failed! Error: " << GetLastError() << "\n";
        CloseHandle(hMapFile);
        return 1;
    }

    auto* ring = static_cast<ringbuffer<1024>*>(raw_ptr);

    new (ring) ringbuffer<1024>();
    std::cout << "[Producer] Initialized ring buffer in shared memory." << std::endl;

    for(int i=1;i<=10;i++){
        IPCMessage msg;
        msg.id = i;
        msg.timestamp = ;
        msg.mssg = "";

        while(!ring->push(msg)){

        }

        std::cout << "[Producer] Pushed: " << msg.mssg << " (ID: " << msg.id << ")" << std::endl;
    }

    std::cout << "[Producer] Finished sending 10 messages." << std::endl;

    UnmapViewOfFile(raw_ptr);
    CloseHandle(hMapFile);
    std::cout << "[Producer] Shared memory closed." << std::endl;


    return 0;
}