#pragma once

#include <atomic>
#include <cstdint>

struct alignas(64) IPCMessage{
    uint64_t id;
    uint64_t timestamp;

    uint64_t sequence;
    double price;
    uint32_t volume;
    uint32_t flags;
    char symbol[8];
    uint8_t reserved[16];
};

template<uint64_t capacity>
class ringbuffer{
    static_assert(capacity > 0 && (capacity & (capacity-1))==0, "Capacity must be a power of 2!");

    private:
        alignas(64) std::atomic<uint64_t> head{0};
        alignas(64) std::atomic<uint64_t> tail{0};
        IPCMessage buffer[capacity];
    
    public:
        ringbuffer() noexcept = default;

        ringbuffer(const ringbuffer&) = delete;
        ringbuffer& operator=(const ringbuffer&) = delete;

        bool push(const IPCMessage& msg, uint64_t& local_tail) noexcept{
            uint64_t current_head = head.load(std::memory_order_relaxed);

            if(current_head-local_tail >= capacity){
                local_tail = tail.load(std::memory_order_acquire);

                if(current_head-local_tail >= capacity){
                    return false;
                }
            }

            buffer[current_head&(capacity-1)] = msg;
            head.store(current_head+1, std::memory_order_release);
            return true;
        }

        bool pop(IPCMessage& msg, uint64_t& local_head) noexcept{
            uint64_t current_tail = tail.load(std::memory_order_relaxed);

            if(current_tail==local_head){
                local_head = head.load(std::memory_order_acquire);

                if(current_tail==local_head){
                    return false;
                }
            }

            msg = buffer[current_tail&(capacity-1)];
            tail.store(current_tail+1, std::memory_order_release);
            return true;
        }
};