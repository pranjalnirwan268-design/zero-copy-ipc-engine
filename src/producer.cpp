#include <chrono>
#include <cstdint>
#include <iostream>
#include <new>
#include <string>

#include <affinity.hpp>
#include <cpupause.hpp>
#include <ringbuffer.hpp>
#include <sharedmemory.hpp>

constexpr int TOTAL_MESSAGES = 1000000;

void wait_for_enter() {
    std::string dummy;
    std::getline(std::cin, dummy);
}

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
    wait_for_enter();

    size_t local_tail = 0;
    uint64_t push_retries = 0;

    pin_thread_to_core(2);

    auto start_time = std::chrono::steady_clock::now();

    for(int i=1;i<=TOTAL_MESSAGES;i++){
        IPCMessage msg;
        msg.id = i;
        msg.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        msg.sequence = i;
        msg.price = 100.50 + i;
        msg.volume = 1000;
        msg.flags = 0x1;
        
        while(!ring->push(msg, local_tail)){
            push_retries++;
            cpu_pause();
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();
    double throughput_mmsg = (TOTAL_MESSAGES / elapsed_sec) / 1e6;
    double bandwidth_mb = (static_cast<double>(TOTAL_MESSAGES) * sizeof(IPCMessage)) / (1024.0 * 1024.0 * elapsed_sec);
    double avg_retries_per_msg = static_cast<double>(push_retries) / TOTAL_MESSAGES;

    std::cout << "================ PRODUCER BENCHMARK RESULTS ================" << std::endl;
    std::cout << " Messages Pushed         : " << TOTAL_MESSAGES << std::endl;
    std::cout << " Elapsed Time            : " << elapsed_sec << " seconds" << std::endl;
    std::cout << " Throughput              : " << throughput_mmsg << " Million msg/sec" << std::endl;
    std::cout << " Egress Bandwidth        : " << bandwidth_mb << " MB/sec" << std::endl;
    std::cout << "-------------------- CONTENTION / SYNC --------------------" << std::endl;
    std::cout << " Total Retries (Full) : " << push_retries << " loops" << std::endl;
    std::cout << " Avg Retries / Msg       : " << avg_retries_per_msg << " retries/msg" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    std::cout << std::endl << "[Producer] Press ENTER to destroy shared memory and exit..." << std::endl;
    wait_for_enter();

    std::cout << "[Producer] Shared memory closed." << std::endl;

    return 0;
}