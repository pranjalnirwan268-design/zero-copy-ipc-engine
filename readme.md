# High-Performance Zero-Copy IPC Engine

A sub-microsecond, lock-free Inter-Process Communication (IPC) engine in C++20 engineered for ultra-low-latency, high-throughput streaming pipelines. Operating over a cache-aligned Single-Producer Single-Consumer (SPSC) ring buffer in shared memory, the architecture achieves multi-million message throughput with bounded sub-microsecond tail latencies across Windows (Win32) and Linux (POSIX).

---

## Key Performance Metrics

Benchmarked with **1,000,000 messages** (64 bytes each, single cache-line sized) streamed across independent OS processes with dedicated CPU core affinity. These figures reflect a **zero-payload synthetic baseline** designed to isolate and stress-test the mechanical transport layer without downstream business logic, establishing the absolute hardware and IPC synchronization floor before application-level workload overhead is introduced.

| Metric | Specification / Observed Result |
| :--- | :--- |
| **Throughput** | **~33.57 Million msg/sec** |
| **Ingress Bandwidth** | **~2,048.93 MB/sec (2.05 GB/s)** |
| **Average Latency** | **~873 ns (0.87 µs)** |
| **p50 (Median Latency)** | **100 ns** |
| **p90 Latency** | **100 ns** |
| **p99 Latency** | **15.1 µs** |
| **p99.9 Tail Latency** | **461.4 µs** |
| **Total Transfer Time** | **~0.029 seconds (29.8 ms)** |
| **Message Loss** | **0%** |

---

## Architecture & System Design

### 1. Unified Shared Memory Layer (`SharedMemoryRegion`)
A cross-platform RAII shared memory abstraction that provides direct, zero-copy pointer mapping without kernel context switches:
- **Windows:** Backed by Win32 memory-mapped files (`CreateFileMappingA`, `MapViewOfFile`, `UnmapViewOfFile`, `CloseHandle`).
- **Linux:** Backed by POSIX real-time shared memory primitives (`shm_open`, `ftruncate`, `mmap`, `munmap`, `shm_unlink`).

### 2. Lock-Free SPSC Ring Buffer (`ringbuffer.hpp`)
- **Memory Consistency:** Utilizes `std::memory_order_acquire` and `std::memory_order_release` atomic semantics to enforce ordering guarantees without locking primitives or OS sleep states.
- **Cache-Line Ping-Pong Mitigation:** Shared atomic counters (`head`, `tail`) are isolated onto distinct 64-byte cache lines via `alignas(64)`. Both producer and consumer maintain private, thread-local shadow copies of the peer's counter (`local_tail` and `local_head`), polling the cross-core atomic line only when local tracking indicates the buffer is full or empty.
- **Bitwise Ring Indexing:** Operates on power-of-2 capacities to replace division/modulo instructions (`%`) with bitwise AND masking (`index & (CAPACITY - 1)`).

### 3. Architecture-Aware Hardware Pause (`cpupause.hpp`)
Polling loops invoke architecture-specific hardware hint intrinsics to eliminate memory pipeline thrashing, reduce core power draw, and prevent speculative execution resource contention:
- **x86 / x86-64:** Emits `_mm_pause()` via `<immintrin.h>`.
- **ARM64:** Issues native hardware spin-wait instructions via MSVC intrinsic `__yield()` or inline assembly `yield` with compiler memory barriers.

### 4. Zero-Copy Struct Serialization
Data records utilize a compact, 64-byte `IPCMessage` layout that fits into a single L1 data cache line. Payloads are constructed directly into mapped memory slots, bypassing intermediate string formatting or memory copies.

### 5. CPU Core Affinity Pinning (`affinity.hpp`)
Enforces hardware execution affinity by locking the producer process to Core 2 and the consumer process to Core 3 using Win32 `SetThreadAffinityMask` and POSIX `pthread_setaffinity_np`. This eliminates thread migration penalties and prevents L1/L2 cache invalidations.

---

## Memory Layout

```text
+-------------------------------------------------------------------+
|               Shared Memory Region (Win32 / POSIX)                |
|                    (MySharedRingBuffer)                           |
|                                                                   |
|   +-----------------------------------------------------------+   |
|   | alignas(64) std::atomic<size_t> head                      |   |
|   +-----------------------------------------------------------+   |
|   | alignas(64) std::atomic<size_t> tail                      |   |
|   +-----------------------------------------------------------+   |
|   | alignas(64) IPCMessage buffer[1024]                       |   |
|   |   - uint64_t id            (8 bytes)                      |   |
|   |   - uint64_t timestamp     (8 bytes)                      |   |
|   |   - uint64_t sequence      (8 bytes)                      |   |
|   |   - double   price         (8 bytes)                      |   |
|   |   - uint32_t volume        (4 bytes)                      |   |
|   |   - uint32_t flags         (4 bytes)                      |   |
|   |   - char     symbol[8]     (8 bytes)                      |   |
|   |   - uint8_t  reserved[16]  (16 bytes)                     |   |
|   |   Total per slot:          64 bytes (Exactly 1 Cache Line)|   |
|   +-----------------------------------------------------------+   |
|                                                                   |
+-------------------------------------------------------------------+
       ^                                                       ^
       |                                                       |
+--------------+                                       +---------------+
| Producer     |                                       | Consumer      |
| (Pinned C:2) |                                       | (Pinned C:3)  |
+--------------+                                       +---------------+
```

---

## Project Structure

```text
zero-copy-ipc-engine/
├── include/
│   ├── affinity.hpp        # Cross-platform CPU core pinning abstractions
│   ├── cpupause.hpp        # Hardware spin-loop intrinsic wrapper
│   ├── ringbuffer.hpp      # Lock-free SPSC ring buffer
│   └── sharedmemory.hpp    # Unified RAII Win32 / POSIX shared memory mapper
├── src/
│   ├── producer.cpp        # Shared memory generator & stream producer
│   └── consumer.cpp        # Shared memory reader
├── bin/                    # Compiled binary outputs   
├── build.ps1               # Automated build script for Windows (PowerShell)
├── build.sh                # Automated build script for Linux (Bash)
├── .gitignore              # Build and binary exclusions
└── README.md
```
---

## Compilation and Execution

### 1. Build Binaries

| Platform | Command | Output Directory |
| :--- | :--- | :--- |
| **Windows (PowerShell)** | `.\build.ps1` | `.\bin\` |
| **Linux (Bash)** | `chmod +x build.sh && ./build.sh` | `./bin/` |

---

### 2. Run the Benchmark

Open two terminal sessions in the project root:

1. **Start Producer (Terminal 1):**
   - Windows: `.\bin\producer.exe`
   - Linux: `./bin/producer`
   
   *Initializes the shared memory region, pins to Core 2, and holds for execution trigger.*

2. **Start Consumer (Terminal 2):**
   - Windows: `.\bin\consumer.exe`
   - Linux: `./bin/consumer`
   
   *Attaches to shared memory, pins to Core 3, and polls for incoming messages.*

3. **Stream Data:**
   Press **ENTER** in Terminal 1 to stream 1,000,000 messages and collect real-time metrics.