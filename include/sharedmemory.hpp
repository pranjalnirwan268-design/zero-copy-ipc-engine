#pragma once

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
#else
    #define PLATFORM_LINUX
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

class SharedMemoryRegion {
    private:
        std::string name;
        size_t size;
        void* mapped_address = nullptr;
        bool is_producer = false;

    #ifdef PLATFORM_WINDOWS
        HANDLE hMapFile = NULL;
    #else
        int shm_fd = -1;
    #endif

    public:
        SharedMemoryRegion(const std::string &shm_name, size_t shm_size, bool produce)
            : name(shm_name), size(shm_size), is_producer(produce)
        {
    #ifdef PLATFORM_WINDOWS
            std::string win_name = "Local\\" + name;

            if(is_producer){
                hMapFile = CreateFileMappingA(
                    INVALID_HANDLE_VALUE,       
                    NULL,                       
                    PAGE_READWRITE,             
                    0,                          
                    static_cast<DWORD>(size),
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
                size               
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
    
    #else
            std::string posix_name = "/" + name;

            if(is_producer){
                shm_fd = shm_open(posix_name.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);

                if(shm_fd != -1){
                    if(ftruncate(shm_fd, size) == -1){
                        std::cerr << "[Linux][Producer] ftruncate failed! Error: " << strerror(errno) << std::endl;
                        close(shm_fd);
                        shm_fd = -1;
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

            if(mapped_address == MAP_FAILED){
                mapped_address = nullptr;
                if(is_producer){
                    std::cerr << "[Linux][Producer] mmap failed! Error: " << strerror(errno) << std::endl;
                }
                else{
                    std::cerr << "[Linux][Consumer] mmap failed! Error: " << strerror(errno) << std::endl;
                }
                close(shm_fd);
                shm_fd = -1;
            }
    #endif
        }

        ~SharedMemoryRegion(){
            if(mapped_address){
    #ifdef PLATFORM_WINDOWS
                UnmapViewOfFile(mapped_address);
                CloseHandle(hMapFile);
    #else
                munmap(mapped_address, size);
                close(shm_fd);
                if(is_producer){
                    std::string posix_name = "/" + name;
                    shm_unlink(posix_name.c_str());
                }
    #endif
            }   
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