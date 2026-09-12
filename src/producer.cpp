#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <new>
#include <string>
#include <thread>

#include <ringbuffer.hpp>
#include <sharedmemory.hpp>

constexpr int TOTAL_MESSAGES = 1000000;

int main(){
    std::string SHM_NAME = "MySharedRingBuffer";
    size_t SHM_SIZE = sizeof(ringbuffer<1024>);

    std::cout << "[Producer] Attempting to create shared memory region..." << std::endl;

    SharedMemoryRegion producer_shm(SHM_NAME, SHM_SIZE, true);

    if(!producer_shm.is_valid()) {
        std::cerr << "[Producer] Failed to initialize shared memory region." << std::endl;
        return 1;
    }

    auto* ring = static_cast<ringbuffer<1024>*>(producer_shm.get_address());

    new (ring) ringbuffer<1024>();
    std::cout << "[Producer] Initialized ring buffer in shared memory." << std::endl;

    std::cout << "[Producer] Ready! Press ENTER to start streaming 1 Million messages..." << std::endl;
    std::cin.get();

    auto start_time = std::chrono::high_resolution_clock::now();
    size_t local_tail = 0;

    for(int i=1;i<=TOTAL_MESSAGES;i++){
        IPCMessage msg;
        msg.id = i;
        msg.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        snprintf(msg.mssg, sizeof(msg.mssg), "Hello from Producer! Msg #%d", i);

        while(!ring->push(msg, local_tail)){
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

    std::cout << "[Producer] Shared memory closed." << std::endl;

    return 0;
}