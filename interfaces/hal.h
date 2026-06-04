#pragma once
#include <cstddef>

class HAL {
public:
    virtual ~HAL() = default;
    virtual void print_text(const char*) = 0;
    virtual void network_request(const char*) = 0;
    virtual void set_prompt(const char*) = 0;
    virtual int read_key() = 0;
    virtual void* allocate(size_t) = 0;
    virtual void draw_pixel(int x, int y, int color) = 0;
};