# 🧠 ZeroKernel

**ZeroKernel** is the pure C++ operating system core for the ZeroRing-Systems architecture. 

### ⚙️ Core Philosophy
This repository strictly adheres to the **"Environmental Ignorance"** rule: the kernel must never know whether it is running inside a web browser or on bare-metal x86 hardware. 

* **Zero Dependencies:** This repository contains strictly freestanding C++. It does not include `<emscripten.h>` or standard library calls (like `printf` or `malloc`).
* **The HAL Contract:** All hardware interactions, memory management, and I/O side-effects are routed entirely through the `hal.h` (Hardware Abstraction Layer) interface. 

### 🛠️ Build Instructions
This project uses **CMake** and the **Emscripten (emcc)** toolchain to compile the C++ logic into a WebAssembly executable (`kernel.wasm`).

```bash
mkdir build
cd build
emcmake cmake ..
make
