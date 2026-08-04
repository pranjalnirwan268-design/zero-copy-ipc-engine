# High-Performance Zero-Copy IPC Engine

A sub-microsecond, lock-free Inter-Process Communication (IPC) engine in C++20 designed for high-frequency data pipelines. Built on Windows Shared Memory, it achieves **~8.85 Million messages/sec** throughput with an average end-to-end latency of **~540 nanoseconds**.

---

## Key Performance Metrics

Benchmarked with **1,000,000 messages** transferred across two independent OS processes:

| Metric | Measured Value |
| :--- | :--- |
| **Throughput** | **~8.85 Million msg/sec** |
| **Average Latency** | **540 ns (0.54 µs)** |
| **Total Transfer Time** | **~0.113 seconds** |
| **Message Loss** | **0%** |

---

## Architectural Highlights

- **Zero-Copy Architecture:** Operates over Win32 Shared Memory (`CreateFileMappingA` / `MapViewOfFile`). Data is written and read directly in shared RAM without copying through the OS kernel or intermediate buffers.
- **Lock-Free Single-Producer Single-Consumer (SPSC):** Implemented using C++11 atomic acquire-release memory order (`std::memory_order_acquire` / `std::memory_order_release`) for zero lock contention.
- **Cache-Line Alignment (`alignas(64)`):** `head` and `tail` atomic indices are aligned to 64-byte boundaries (L1 cache line size) to completely eliminate **False Sharing**.
- **Power-of-2 Bitwise Ring Indexing:** Bypasses costly modulo arithmetic (`%`) by using bitwise AND masking (`index & (capacity - 1)`), reducing CPU cycle overhead.

---

## Memory Layout

```
+-------------------------------------------------------------------+
|                   Win32 Shared Memory Region                      |
|                 (Local\MySharedRingBuffer)                        |
|                                                                   |
|   +-----------------------------------------------------------+   |
|   | alignas(64) std::atomic<size_t> head                      |   |
|   +-----------------------------------------------------------+   |
|   | alignas(64) std::atomic<size_t> tail                      |   |
|   +-----------------------------------------------------------+   |
|   | IPCMessage buffer[1024]                                   |   |
|   |   - size_t id                                             |   |
|   |   - uint64_t timestamp                                    |   |
|   |   - char mssg[48]                                         |   |
|   +-----------------------------------------------------------+   |
|                                                                   |
+-------------------------------------------------------------------+
       ^                                                     ^
       |                                                     |
+--------------+                                      +--------------+
| Producer.exe | -- (push)                      (pop) -- | Consumer.exe |
+--------------+                                      +--------------+
```

---

## Project Structure

```text
zero-copy-ipc-engine/
├── include/
│   └── ringbuffer.hpp      # Lock-free SPSC ring buffer & IPCMessage struct
├── src/
│   ├── producer.cpp        # Shared memory creator & stream producer
│   └── consumer.cpp        # Shared memory reader & benchmark collector
├── bin/                    # Compiled binary outputs
├── .gitignore              # Ignores compiled binaries
├── build.ps1               # PowerShell build script
└── README.md
```

---

## Building and Running

### Prerequisites
- C++20 compatible compiler (MSVC / MinGW g++)
- Windows OS (Win32 API)

### 1. Compile the Project
Run the PowerShell build script:
```powershell
.\build.ps1
```

### 2. Run the Benchmark
Open two terminal windows in `bin/`:

**Terminal 1 (Producer):**
```powershell
.\producer.exe
```

**Terminal 2 (Consumer):**
```powershell
.\consumer.exe
```

Press **ENTER** in Terminal 1 to stream 1,000,000 messages and view real-time latency and throughput metrics.