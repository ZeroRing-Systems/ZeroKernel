#include "../interfaces/hal.h"
#include <cstdlib>

extern "C" {
void js_print_text(const char *);
void js_network_request(const char *);
void js_set_prompt(const char *);
int js_read_key();
void js_draw_pixel(int x, int y, int color);
}

struct WasmHAL : HAL {
  void print_text(const char *t) override { js_print_text(t); }
  void network_request(const char *p) override { js_network_request(p); }
  void set_prompt(const char *p) override { js_set_prompt(p); }
  int read_key() override { return js_read_key(); }
  void *allocate(size_t n) override { return malloc(n); }
  void draw_pixel(int x, int y, int color) override { js_draw_pixel(x, y, color); }
};

static WasmHAL g_hal;
extern "C" HAL *get_hal() { return &g_hal; }