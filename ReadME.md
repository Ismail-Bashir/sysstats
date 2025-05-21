# 📊 ConcurrentSystemStats

**`ConcurrentSystemStats`** (main executable: `mySystemStats`) is a real-time, terminal-based system monitoring utility. It reports live system metrics like CPU usage, memory consumption, and active user sessions — all while showcasing concurrent programming using multiple child processes and inter-process communication (IPC).

---

## 🔍 Overview

`ConcurrentSystemStats` captures system health data using a **multi-process architecture**. Each major system metric is collected by a dedicated child process, allowing parallel, non-blocking data gathering. A parent process aggregates and renders the data in an organized and user-friendly format.

Features include:
- Real-time updates
- Graphics (bars) for CPU and memory usage
- Graceful exit on `Ctrl-C`
- Terminal-clearing for clean refresh

---

## 🧠 Design Approach

### 🧵 Concurrent Data Collection
Each system component (CPU, memory, user sessions) is handled by an individual **child process** to enable simultaneous collection.

### 🔄 Inter-Process Communication (IPC)
Child processes use **pipes** to send data to the parent, allowing the parent process to coordinate output.

### 🛑 Robust Signal Handling
Custom signal handlers:
- Intercept `SIGINT` (Ctrl-C) for controlled termination
- Ignore `SIGTSTP` (Ctrl-Z) to avoid suspending the program

---

## ⚙️ Core Components

### 🔧 Child Process Functions

| Function | Description |
|---------|-------------|
| `storeMemArr(int samples, int memFD[2], int tdelay);` | Collects memory usage and writes to a pipe |
| `storeUserInfoThird(int userFD[2], int ucountFD[2]);` | Gathers user session data and count |
| `storeCpuArr(int cpuFD[2]);` | Reads CPU statistics from `/proc/stat` and writes to a pipe |

---

### 🖥️ Parent Process Functions

| Function | Description |
|---------|-------------|
| `fcnForPrintMemoryArr(...)` | Displays memory info using the received pipe data |
| `printUserInfoThird(...)` | Reads and prints user session data |

---

## 🧩 Signal Handling

### `void signal_handler(int signal);`

- **SIGINT (Ctrl-C):**
  - Asks user whether to quit
  - If yes, terminates all child processes and exits cleanly
- **SIGTSTP (Ctrl-Z):**
  - Ignored to prevent suspending the process

```c
signal(SIGTSTP, SIG_IGN); // Prevent Ctrl-Z from suspending the program

