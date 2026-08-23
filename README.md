# Parallel Merge Sort – Client/Server

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white)]()
[![Platform](https://img.shields.io/badge/platform-Windows-0078D6?logo=windows&logoColor=white)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()

A client-server application written in **C++** that demonstrates and benchmarks single-threaded vs multi-threaded Merge Sort across multiple simultaneous clients.

## Overview

The server receives integer arrays from multiple clients, sorts each array using both a single-threaded and a multi-threaded Merge Sort, and returns the sorted result along with timing measurements for both approaches.

## Project Structure

```
.
├── server.cpp   # TCP server – sorts data, returns results + timing
└── client.cpp   # TCP client – sends arrays, prints sorted output + benchmarks
```

## How It Works

### Server
- Listens on port `8080`
- Accepts multiple clients, spawning a new `std::thread` per connection
- For each client:
  - Receives an array of integers (max 100,000 elements)
  - Runs **single-threaded** Merge Sort → measures time
  - Runs **multi-threaded** Merge Sort → measures time
  - Returns sorted array + both timings

### Multi-threaded Merge Sort
At each split, the algorithm checks if the active thread count is below `std::thread::hardware_concurrency()`. If yes, it spawns two new threads for the two halves. Otherwise it falls back to the single-threaded variant. Merging happens in the calling thread.

### Client
- Spawns **5 threads**, each acting as an independent client
- Each thread connects to `127.0.0.1:8080`, sends an array, and prints the sorted result + timing comparison
- Output is synchronized with `std::mutex` to avoid interleaved console output

## Build & Run

**Compile server:**
```bash
g++ server.cpp -o server.exe -lws2_32 -std=c++17 -pthread
```

**Compile client:**
```bash
g++ client.cpp -o client.exe -lws2_32 -std=c++17 -pthread
```

**Start server:**
```bash
./server.exe
# Output: Server is running on port 8080
```

**Start client** (in a separate terminal):
```bash
./client.exe
```

## Example Output

```
Client 1 : Sorted data:
Client 1 : Single thread time :  0 ms
Client 1 : Multi thread time  :  0 ms

Client 3 : Sorted data: -87 -21 9 38 64
Client 3 : Single thread time :  0.0039 ms
Client 3 : Multi thread time  :  6.1901 ms

Client 5 : Sorted data: -91 -76 -64 -57 -49 -33 -8 -2 0 3 7 14 ...
Client 5 : Single thread time :  0.0156 ms
Client 5 : Multi thread time  :  6.6588 ms
```

> Note: Multi-threaded overhead exceeds gains for small arrays. The parallel approach is designed to scale with larger datasets.

## Technologies

- C++17
- Winsock2 (Windows TCP networking)
- `std::thread`, `std::mutex`
- Merge Sort (single & parallel)
