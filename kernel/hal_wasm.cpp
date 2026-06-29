#include "../interfaces/hal.h"

extern "C" {
    void js_print(const char* text);
    void js_set_prompt(const char* prompt);
    void js_clear_screen(void);
    void js_net_send(const char* json);
    void js_log_error(const char* msg);
}

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