#include "../interfaces/hal.h"

extern "C" {
    void js_print_text(const char*);
    void js_set_prompt(const char*);
    void js_network_request(const char*);
}

struct WasmHAL : HAL {
    void print_text(const char* t)      override { js_print_text(t); }
    void set_prompt(const char* p)      override { js_set_prompt(p); }
    void network_request(const char* p) override { js_network_request(p); }
};

static WasmHAL g_hal;
extern "C" HAL* get_hal() { return &g_hal; }