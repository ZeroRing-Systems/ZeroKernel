# ZeroKernel

The core kernel for [ZeroRing OS](https://zeroring-cloud.vercel.app), compiled to WebAssembly. Runs at near-native speed directly in the browser.

## What it does

- Runs a custom shell (`zerosh`) with built-in commands for file management
- Communicates with the backend through a HAL (Hardware Abstraction Layer) interface
- The kernel is platform-agnostic — it doesn't know it's running in a browser, it just calls HAL functions
- Tokenizes user input with full quoted-string argument support
- Dispatches commands through a modular command table

## Shell Commands

| Command | Description |
|---|---|
| `help` | List all available commands |
| `version` | Show kernel version |
| `whoami` | Show current user |
| `pwd` | Print working directory |
| `echo <text>` | Print text to terminal |
| `cd <path>` | Change directory |
| `ls [path]` | List directory contents |
| `mkdir <path>` | Create a new directory |
| `cat <file>` | Print file contents |
| `write <file> <data>` | Write data to a file |
| `rm <path>` | Remove a file or empty directory |
| `edit <file>` | Open file in the integrated UI editor |
| `run <file>` | Execute a Python script in the server sandbox |

## Architecture

```
┌──────────────────────────────────────────┐
│              ZeroKernel (C++)             │
│                                           │
│  kernel.cpp                               │
│    ├── Tokenizer (quoted string support)  │
│    ├── Command dispatch table             │
│    └── JSON packet formatting             │
│                                           │
│  HAL Interface (hal.h)                    │
│    ├── print() / set_prompt()             │
│    ├── net_send() / net_response()        │
│    └── clear_screen()                     │
│                                           │
│  hal_wasm.cpp                             │
│    └── Bridges HAL → JavaScript imports   │
└──────────────────────────────────────────┘
         ↓ compiled via Emscripten ↓
              kernel.wasm
```

## Project Structure

| File | Description |
|---|---|
| `interfaces/hal.h` | Abstract HAL class defining the kernel's I/O capabilities |
| `kernel/kernel.cpp` | Shell implementation: tokenizer, command dispatch, all built-in commands |
| `kernel/hal_wasm.cpp` | WASM HAL implementation — bridges C++ HAL calls to JavaScript `env` imports |
| `CMakeLists.txt` | Emscripten build configuration |
| `build_and_run.sh` | Build helper script |

## Build

Requires [Emscripten](https://emscripten.org/docs/getting_started/downloads.html):

```bash
mkdir build && cd build
emcmake cmake ..
make
```

Produces `kernel.wasm` — copy it to `ZeroRing-Cloud/public/wasm/` for deployment.

## How it connects

The kernel is loaded by the frontend (`terminal.js`) via `WebAssembly.instantiateStreaming()`. JavaScript provides the HAL implementation through WASM imports:

- `js_print(ptr)` → renders text to the terminal
- `js_set_prompt(ptr)` → updates the shell prompt
- `js_net_send(ptr)` → sends JSON commands to the backend over WebSocket
- `js_clear_screen()` → clears the terminal output
