#include "../interfaces/hal.h"

extern "C" HAL* get_hal();

static HAL* hal;
static char line[256];
static int line_pos = 0;
static const char* prompt = "zerosh $ ";

static bool streq(const char* a, const char* b) {
    while (*a && *b) { if (*a++ != *b++) return false; }
    return *a == *b;
}

static bool starts_with(const char* str, const char* prefix) {
    while (*prefix) { if (*str++ != *prefix++) return false; }
    return true;
}

static void show_prompt() {
    hal->set_prompt(prompt);
}

static void exec(const char* cmd) {
    if (streq(cmd, "help")) {
        hal->print_text("commands: help, clear, ls, cat <file>, write <file> <data>, rm <file>, uname");
        return;
    }

    if (streq(cmd, "clear")) {
        hal->print_text("\x1b[clear]");
        return;
    }

    if (streq(cmd, "uname")) {
        hal->print_text("ZeroRing OS v0.1 [wasm32]");
        return;
    }

    if (streq(cmd, "ls")) {
        hal->network_request("{\"action\":\"list_files\"}");
        return;
    }

    if (starts_with(cmd, "cat ")) {
        const char* file = cmd + 4;
        char buf[512];
        int i = 0;
        const char* pre = "{\"action\":\"read_file\",\"file\":\"";
        while (*pre) buf[i++] = *pre++;
        while (*file) buf[i++] = *file++;
        const char* suf = "\"}";
        while (*suf) buf[i++] = *suf++;
        buf[i] = 0;
        hal->network_request(buf);
        return;
    }

    if (starts_with(cmd, "write ")) {
        const char* rest = cmd + 6;
        char file[128];
        int fi = 0;
        while (*rest && *rest != ' ') file[fi++] = *rest++;
        file[fi] = 0;
        if (*rest) rest++;

        char buf[512];
        int i = 0;
        const char* pre = "{\"action\":\"write_file\",\"file\":\"";
        while (*pre) buf[i++] = *pre++;
        const char* f = file;
        while (*f) buf[i++] = *f++;
        const char* mid = "\",\"data\":\"";
        while (*mid) buf[i++] = *mid++;
        while (*rest) buf[i++] = *rest++;
        const char* suf = "\"}";
        while (*suf) buf[i++] = *suf++;
        buf[i] = 0;
        hal->network_request(buf);
        return;
    }

    if (starts_with(cmd, "rm ")) {
        const char* file = cmd + 3;
        char buf[512];
        int i = 0;
        const char* pre = "{\"action\":\"delete_file\",\"file\":\"";
        while (*pre) buf[i++] = *pre++;
        while (*file) buf[i++] = *file++;
        const char* suf = "\"}";
        while (*suf) buf[i++] = *suf++;
        buf[i] = 0;
        hal->network_request(buf);
        return;
    }

    if (cmd[0] != 0) {
        hal->print_text("unknown command");
    }
}

extern "C" void handle_key(int key) {
    if (key == 13 || key == 10) {
        line[line_pos] = 0;
        hal->print_text("");
        exec(line);
        line_pos = 0;
        line[0] = 0;
        show_prompt();
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
    hal->print_text("type 'help' for commands");
    hal->print_text("");
    show_prompt();
}