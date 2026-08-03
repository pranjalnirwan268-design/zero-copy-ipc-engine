#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

struct alignas(64) IPCMessage{
    size_t id;
    uint64_t timestamp;
    char mssg[48];
};

template<size_t capacity>
class ringbuffer{
    static_assert((capacity & (capacity-1))==0, "capacity must be a power of 2!");

    private:
        alignas(64) std::atomic<size_t> head;
        alignas(64) std::atomic<size_t> tail;
        IPCMessage buffer[capacity];
    
    public:
        ringbuffer(){
            head = 0;
            tail = 0;
        }

        bool push(const IPCMessage& msg){
            size_t current_head = head.load(std::memory_order_relaxed);
            size_t current_tail = tail.load(std::memory_order_acquire);

            if(current_head-current_tail >= capacity){
                return false;
            }
            buffer[current_head&(capacity-1)] = msg;
            head.store(current_head+1, std::memory_order_release);
            return true;
        }

        bool pop(IPCMessage& msg){
            size_t current_head = head.load(std::memory_order_acquire);
            size_t current_tail = tail.load(std::memory_order_relaxed);

            if(current_head==current_tail){
                return false;
            }
            msg = buffer[current_tail&(capacity-1)];
            tail.store(current_tail+1, std::memory_order_release);
            return true;
        }
};