#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <affinity.hpp>
#include <cpupause.hpp>
#include <ringbuffer.hpp>
#include <sharedmemory.hpp>

constexpr int TOTAL_MESSAGES = 1000000;

int main(){
    std::string SHM_NAME = "MySharedRingBuffer";
    size_t SHM_SIZE = sizeof(ringbuffer<1024>);

    std::cout << "[Consumer] Attempting to connect to the shared memory region..." << std::endl;

    SharedMemoryRegion consumer_shm(SHM_NAME, SHM_SIZE, false);

    if(!consumer_shm.is_valid()){
        std::cerr << "[Consumer] Failed to connect to the shared memory region." << std::endl;
        std::cerr << "[Consumer] Make sure producer.exe is running first!" << std::endl;
        return 1;
    }

    std::cout << "[Consumer] Successfully connected to the shared memory region." << std::endl;

    auto* ring = static_cast<ringbuffer<1024>*>(consumer_shm.get_address());

    size_t local_head = 0;
    uint64_t pop_retries = 0;

    std::chrono::steady_clock::time_point start_time;

    std::vector<uint64_t> latencies;
    latencies.resize(TOTAL_MESSAGES-1);

    pin_thread_to_core(3);

    for(int i=1;i<=TOTAL_MESSAGES;i++){
        IPCMessage msg;

        while(!ring->pop(msg, local_head)){
            pop_retries++;
            cpu_pause();
        }

        uint64_t now = std::chrono::steady_clock::now().time_since_epoch().count();

        if(i == 1){
            start_time = std::chrono::steady_clock::now();
        }
        else{
            int64_t diff = static_cast<int64_t>(now - msg.timestamp);
            uint64_t lat = (diff > 0) ? static_cast<uint64_t>(diff) : 0;
            latencies[i-2] = lat;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();

    std::sort(latencies.begin(), latencies.end());

    size_t n = latencies.size();
    uint64_t p50  = latencies[static_cast<size_t>(n * 0.50)];
    uint64_t p90  = latencies[static_cast<size_t>(n * 0.90)];
    uint64_t p99  = latencies[static_cast<size_t>(n * 0.99)];
    uint64_t p999 = latencies[static_cast<size_t>(n * 0.999)];
    uint64_t max_lat = latencies.back();

    uint64_t total_latency_ns = 0;
    for (uint64_t lat : latencies) {
        total_latency_ns += lat;
    }
    double avg_latency_ns = static_cast<double>(total_latency_ns) / n;

    double processed_msgs = static_cast<double>(TOTAL_MESSAGES - 1);
    double throughput_mmsg = (processed_msgs / elapsed_sec) / 1e6;
    double bandwidth_mb = (processed_msgs * sizeof(IPCMessage)) / (1024.0 * 1024.0 * elapsed_sec);
    double avg_retries_per_msg = static_cast<double>(pop_retries) / TOTAL_MESSAGES;

    std::cout <<  "================ CONSUMER BENCHMARK RESULTS ================" << std::endl;
    std::cout << " Messages Received       : " << TOTAL_MESSAGES << std::endl;
    std::cout << " Elapsed Time            : " << elapsed_sec << " seconds" << std::endl;
    std::cout << " Throughput              : " << throughput_mmsg << " Million msg/sec" << std::endl;
    std::cout << " Ingress Bandwidth       : " << bandwidth_mb << " MB/sec" << std::endl;
    std::cout << "-------------------- CONTENTION / SYNC --------------------" << std::endl;
    std::cout << " Total Spin-Waits (Empty): " << pop_retries << " loops" << std::endl;
    std::cout << " Avg Retries / Msg       : " << avg_retries_per_msg << " retries/msg" << std::endl;
    std::cout << "-------- END-TO-END QUEUING LATENCY (WAIT + TRANSIT) --------" << std::endl;
    std::cout << " Average Latency         : " << avg_latency_ns << " ns (" << (avg_latency_ns / 1000.0) << " µs)" << std::endl;
    std::cout << " p50 (Median)            : " << p50 << " ns" << std::endl;
    std::cout << " p90                     : " << p90 << " ns" << std::endl;
    std::cout << " p99                     : " << p99 << " ns" << std::endl;
    std::cout << " p99.9                   : " << p999 << " ns" << std::endl;
    std::cout << " Max Latency             : " << max_lat << " ns (" << (max_lat / 1000.0) << " µs)" << std::endl;
    std::cout << "============================================================" << std::endl;

    std::cout << "[Consumer] Disconnecting from shared memory." << std::endl;

    return 0;

}