#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__linux__)
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
#else
    #error "sharedmemory.hpp: Unsupported platform. Only Windows and Linux are supported."
#endif
class SharedMemoryRegion {
    private:
        std::string name;
        uint64_t size;
        void* mapped_address = nullptr;
        bool is_producer = false;

    #if defined(_WIN32) || defined(_WIN64)
        HANDLE hMapFile = NULL;
    #else
        int shm_fd = -1;
    #endif

    public:
        SharedMemoryRegion(const std::string &shm_name, uint64_t shm_size, bool produce)
            : name(shm_name), size(shm_size), is_producer(produce)
        {
    #if defined(_WIN32) || defined(_WIN64)
            std::string win_name = "Local\\" + name;

            if(is_producer){
                DWORD size_high = static_cast<DWORD>((size >> 32) & 0xFFFFFFFF);
                DWORD size_low  = static_cast<DWORD>(size & 0xFFFFFFFF);

                hMapFile = CreateFileMappingA(
                    INVALID_HANDLE_VALUE,       
                    NULL,                       
                    PAGE_READWRITE,             
                    size_high,                          
                    size_low,
                    win_name.c_str()
                );
            }
            else{
                hMapFile = OpenFileMappingA(                     
                    FILE_MAP_ALL_ACCESS,             
                    false,
                    win_name.c_str()               
                );
            }

            if(hMapFile == NULL){
                if(is_producer){
                    std::cerr << "[Windows][Producer] CreateFileMappingA failed! Error: " << GetLastError() << std::endl;
                    return;
                }
                std::cerr << "[Windows][Consumer] OpenFileMappingA failed! Error: " << GetLastError() << std::endl;
                std::cerr << "[Windows][Consumer] Make sure a producer is running first!" << std::endl;
                return;
            }

            mapped_address = MapViewOfFile(
                hMapFile,               
                FILE_MAP_ALL_ACCESS,   
                0, 0,                   
                static_cast<SIZE_T>(size)               
            );

            if(mapped_address == nullptr){
                if(is_producer){
                    std::cerr << "[Windows][Producer] MapViewOfFile failed! Error: " << GetLastError() << std::endl;
                }
                else{
                    std::cerr << "[Windows][Consumer] MapViewOfFile failed! Error: " << GetLastError() << std::endl;
                }
                CloseHandle(hMapFile);
                hMapFile = NULL;
            }
    
    #elif defined(__linux__)
            std::string posix_name = "/" + name;

            if(is_producer){
                shm_fd = shm_open(posix_name.c_str(), O_CREAT | O_RDWR, 0666);

                if(shm_fd != -1){
                    if(ftruncate(shm_fd, size) == -1){
                        std::cerr << "[Linux][Producer] ftruncate failed! Error: " << strerror(errno) << std::endl;
                        close(shm_fd);
                        shm_fd = -1;
                        shm_unlink(posix_name.c_str());
                        return;
                    }
                }
            }
            else{
                shm_fd = shm_open(posix_name.c_str(), O_RDWR, 0666);
            }

            if(shm_fd == -1){
                if(is_producer){
                    std::cerr << "[Linux][Producer] shm_open failed! Error: " << strerror(errno) << std::endl;
                    return;
                }
                std::cerr << "[Linux][Consumer] shm_open failed! Error: " << strerror(errno) << std::endl;
                return;
            }

            mapped_address = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

            close(shm_fd);
            shm_fd = -1;

            if(mapped_address == MAP_FAILED){
                mapped_address = nullptr;
                if(is_producer){
                    std::cerr << "[Linux][Producer] mmap failed! Error: " << strerror(errno) << std::endl;
                }
                else{
                    std::cerr << "[Linux][Consumer] mmap failed! Error: " << strerror(errno) << std::endl;
                }
            }
    #endif
        }

        ~SharedMemoryRegion(){
        #if defined(_WIN32) || defined(_WIN64)
            if(mapped_address){
                UnmapViewOfFile(mapped_address);
            }
            if(hMapFile){
                CloseHandle(hMapFile);
            }
        #elif defined(__linux__)
            if(mapped_address){
                munmap(mapped_address, size);
            }
            if(is_producer){
                std::string posix_name = "/" + name;
                shm_unlink(posix_name.c_str());
            }
        #endif
        }

        void* get_address() const noexcept{
            return mapped_address;
        }

        bool is_valid() const noexcept{
            return mapped_address != nullptr;
        }

        // Disable copy semantics
        SharedMemoryRegion(const SharedMemoryRegion&) = delete;
        SharedMemoryRegion& operator=(const SharedMemoryRegion&) = delete;
    
};