#pragma once

class HAL {
public:
    virtual ~HAL() = default;
    virtual void print_text(const char* text) = 0;
    virtual void set_prompt(const char* prompt) = 0;
    virtual void network_request(const char* json) = 0;
};