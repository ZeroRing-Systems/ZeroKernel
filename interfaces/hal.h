#pragma once
class HAL {
public:
    virtual ~HAL() = default;
    virtual void print_text(const char*) = 0;
    virtual void network_request(const char*) = 0;
};