#pragma once

struct HAL {
    void (*print)(const char* text);
    void (*set_prompt)(const char* prompt);
    void (*clear_screen)(void);
    void (*net_send)(const char* json);
    void (*log_error)(const char* msg);
};

#ifdef __cplusplus
extern "C" {
#endif

HAL* hal_get(void);

#ifdef __cplusplus
}
#endif