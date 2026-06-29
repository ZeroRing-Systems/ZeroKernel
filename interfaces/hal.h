// ============================================================================
// hal.h — Hardware Abstraction Layer for ZeroKernel
// ============================================================================
// This interface defines the contract between the freestanding C++ kernel and
// the host environment (browser/WASM). All methods use C-string ABI so no
// std::string crosses the boundary. Implementations live in hal_wasm.cpp.
// ============================================================================
#pragma once

//
// HAL function pointer table — pure C ABI, no vtable overhead, no RTTI.
// The kernel never knows whether it is running inside a browser, a test
// harness, or a native debug shim.  It only calls through this table.
//
struct HAL {
    // ---- Terminal I/O ----
    void (*print)(const char* text);             // Write a line to the terminal
    void (*set_prompt)(const char* prompt);       // Update the visible prompt
    void (*clear_screen)(void);                   // Clear terminal output

    // ---- Network (WebSocket bridge to ZeroRing-Cloud) ----
    void (*net_send)(const char* json);           // Send a JSON command to backend

    // ---- Diagnostics ----
    void (*log_error)(const char* msg);           // Write an error to the JS console
};

// Implemented by hal_wasm.cpp — returns the singleton HAL instance.
#ifdef __cplusplus
extern "C" {
#endif

HAL* hal_get(void);

#ifdef __cplusplus
}
#endif