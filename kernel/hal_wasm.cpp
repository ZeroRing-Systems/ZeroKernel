// ============================================================================
// hal_wasm.cpp — WebAssembly HAL implementation for ZeroKernel
// ============================================================================
// This file bridges the freestanding C++ kernel to JavaScript host functions
// via Emscripten's extern "C" import mechanism. Each js_* function is
// provided by the WebAssembly.instantiate() import object in terminal.js.
// ============================================================================
#include "../interfaces/hal.h"

// ---------------------------------------------------------------------------
// JavaScript imports — these symbols are resolved at WASM instantiation time.
// The browser provides them in the `env` namespace of the import object.
// ---------------------------------------------------------------------------
extern "C" {
    void js_print(const char* text);
    void js_set_prompt(const char* prompt);
    void js_clear_screen(void);
    void js_net_send(const char* json);
    void js_log_error(const char* msg);
}

// ---------------------------------------------------------------------------
// HAL function table — static singleton, zero-cost dispatch
// ---------------------------------------------------------------------------
static HAL g_hal = {
    .print        = js_print,
    .set_prompt   = js_set_prompt,
    .clear_screen = js_clear_screen,
    .net_send     = js_net_send,
    .log_error    = js_log_error,
};

extern "C" HAL* hal_get(void) {
    return &g_hal;
}