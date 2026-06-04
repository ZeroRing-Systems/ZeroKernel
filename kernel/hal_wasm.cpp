#include "../interfaces/hal.h"
#include <cstdlib>

extern "C" {
void js_print_text(const char *);
void js_network_request(const char *);
void js_set_prompt(const char *);
int js_read_key();
void js_draw_pixel(int x, int y, int color);
int js_fs_open(const char *path, int flags);
int js_fs_read(int fd, char *buf, int len);
int js_fs_write(int fd, const char *buf, int len);
int js_fs_close(int fd);
void js_set_storage(const char *mode);
}

struct WasmHAL : HAL {
  void print_text(const char *t) override { js_print_text(t); }
  void network_request(const char *p) override { js_network_request(p); }
  void set_prompt(const char *p) override { js_set_prompt(p); }
  int read_key() override { return js_read_key(); }
  void *allocate(size_t n) override { return malloc(n); }
  void draw_pixel(int x, int y, int color) override { js_draw_pixel(x, y, color); }
  int fs_open(const char *path, int flags) override { return js_fs_open(path, flags); }
  int fs_read(int fd, char *buf, int len) override { return js_fs_read(fd, buf, len); }
  int fs_write(int fd, const char *buf, int len) override { return js_fs_write(fd, buf, len); }
  int fs_close(int fd) override { return js_fs_close(fd); }
  void set_storage(const char *mode) override { js_set_storage(mode); }
};

static WasmHAL g_hal;
extern "C" HAL *get_hal() { return &g_hal; }