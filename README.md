# ZeroKernel

The kernel for ZeroRing OS, compiled to WebAssembly.

## What it does

- Runs a simple shell (`zerosh`) with commands like `ls`, `cat`, `write`, `rm`
- Talks to the backend through a HAL (Hardware Abstraction Layer) interface
- The kernel doesn't know it's running in a browser — it just calls HAL functions

## Files

- `interfaces/hal.h` — abstract class that defines what the kernel can do
- `kernel/kernel.cpp` — the shell and all commands
- `kernel/hal_wasm.cpp` — connects HAL functions to JavaScript

## Build

```bash
mkdir build && cd build
emcmake cmake ..
make
```

Produces `kernel.wasm`.
