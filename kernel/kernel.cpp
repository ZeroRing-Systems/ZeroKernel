#include "../interfaces/hal.h"

extern "C" HAL* get_hal();

static HAL* hal;
static char line[256];
static int line_pos = 0;

extern "C" void handle_key(int key) {
    if (key == 13) {
        line[line_pos] = 0;
        hal->print_text("");
        hal->print_text(line);
        line_pos = 0;
        line[0] = 0;
        hal->set_prompt("$ ");
        return;
    }

    if (key == 8 || key == 127) {
        if (line_pos > 0) line_pos--;
        line[line_pos] = 0;
        return;
    }

    if (line_pos < 255 && key >= 32 && key < 127) {
        line[line_pos++] = (char)key;
        line[line_pos] = 0;
    }
}

extern "C" void kernel_main() {
    hal = get_hal();
    hal->print_text("ZeroRing OS v0.1");
    hal->set_prompt("$ ");
}