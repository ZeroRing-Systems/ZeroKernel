#include "../interfaces/hal.h"

extern "C" HAL* get_hal();

static HAL* hal;
static char line[256];
static int line_pos = 0;
static const char* prompt = "zerosh $ ";

static int slen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void scpy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = 0;
}

static void scat(char* dst, const char* src) {
    while (*dst) dst++;
    while (*src) *dst++ = *src++;
    *dst = 0;
}

static bool seq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++;
        b++;
    }
    return *a == *b;
}

static void int_to_str(int n, char* buf) {
    if (n < 0) { *buf++ = '-'; n = -n; }
    if (n == 0) { *buf++ = '0'; *buf = 0; return; }
    char tmp[12];
    int i = 0;
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) *buf++ = tmp[--i];
    *buf = 0;
}

static int str_to_int(const char* s) {
    int n = 0;
    bool neg = false;
    if (*s == '-') { neg = true; s++; }
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return neg ? -n : n;
}

static int hex_to_int(const char* s) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    int val = 0;
    while (*s) {
        int d;
        if (*s >= '0' && *s <= '9')      d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        else break;
        val = (val << 4) | d;
        s++;
    }
    return val;
}

struct Args {
    int argc;
    char argv[16][64];
};

static Args parse(const char* input) {
    Args a;
    a.argc = 0;
    int pos = 0;

    while (input[pos] == ' ') pos++;

    while (input[pos] && a.argc < 16) {
        int len = 0;
        if (input[pos] == '"') {
            pos++;
            while (input[pos] && input[pos] != '"')
                a.argv[a.argc][len++] = input[pos++];
            if (input[pos] == '"') pos++;
        } else {
            while (input[pos] && input[pos] != ' ')
                a.argv[a.argc][len++] = input[pos++];
        }
        a.argv[a.argc][len] = 0;
        a.argc++;
        while (input[pos] == ' ') pos++;
    }

    return a;
}

static void build_json(char* buf, const char* action,
                       const char* file = nullptr,
                       const char* extra_key = nullptr,
                       const char* extra_val = nullptr) {
    scpy(buf, "{\"action\":\"");
    scat(buf, action);
    scat(buf, "\"");
    if (file) {
        scat(buf, ",\"file\":\"");
        scat(buf, file);
        scat(buf, "\"");
    }
    if (extra_key && extra_val) {
        scat(buf, ",\"");
        scat(buf, extra_key);
        scat(buf, "\":\"");
        scat(buf, extra_val);
        scat(buf, "\"");
    }
    scat(buf, "}");
}

static void exec(const char* input) {
    Args a = parse(input);
    if (a.argc == 0) return;

    char buf[512];

    if (seq(a.argv[0], "help")) {
        hal->print_text("help              show this message");
        hal->print_text("clear             clear the screen");
        hal->print_text("uname             system info");
        hal->print_text("whoami            current user");
        hal->print_text("echo [text...]    print text");
        hal->print_text("ls                list files");
        hal->print_text("cat <file>        read a file");
        hal->print_text("write <f> <data>  write to a file");
        hal->print_text("rm <file>         delete a file");
        hal->print_text("stat <file>       check if file exists");
        hal->print_text("touch <file>      create empty file");
        hal->print_text("pixel <x> <y> <c> draw a pixel");
        hal->print_text("open <file>       open file descriptor");
        hal->print_text("fread <fd>        read from fd");
        hal->print_text("fwrite <fd> <d>   write to fd");
        hal->print_text("fclose <fd>       close fd");
    }

    else if (seq(a.argv[0], "clear")) {
        hal->print_text("\x1b[clear]");
    }

    else if (seq(a.argv[0], "uname")) {
        hal->print_text("ZeroRing OS v0.1 [wasm32]");
    }

    else if (seq(a.argv[0], "whoami")) {
        hal->print_text("root");
    }

    else if (seq(a.argv[0], "echo")) {
        if (a.argc < 2) {
            hal->print_text("");
        } else {
            scpy(buf, a.argv[1]);
            for (int i = 2; i < a.argc; i++) {
                scat(buf, " ");
                scat(buf, a.argv[i]);
            }
            hal->print_text(buf);
        }
    }

    else if (seq(a.argv[0], "ls")) {
        build_json(buf, "list_files");
        hal->network_request(buf);
    }

    else if (seq(a.argv[0], "cat")) {
        if (a.argc < 2) {
            hal->print_text("usage: cat <file>");
        } else {
            build_json(buf, "read_file", a.argv[1]);
            hal->network_request(buf);
        }
    }

    else if (seq(a.argv[0], "write")) {
        if (a.argc < 3) {
            hal->print_text("usage: write <file> <data>");
        } else {
            char data[256];
            scpy(data, a.argv[2]);
            for (int i = 3; i < a.argc; i++) {
                scat(data, " ");
                scat(data, a.argv[i]);
            }
            build_json(buf, "write_file", a.argv[1], "data", data);
            hal->network_request(buf);
        }
    }

    else if (seq(a.argv[0], "rm")) {
        if (a.argc < 2) {
            hal->print_text("usage: rm <file>");
        } else {
            build_json(buf, "delete_file", a.argv[1]);
            hal->network_request(buf);
        }
    }

    else if (seq(a.argv[0], "stat")) {
        if (a.argc < 2) {
            hal->print_text("usage: stat <file>");
        } else {
            build_json(buf, "read_file", a.argv[1]);
            hal->network_request(buf);
        }
    }

    else if (seq(a.argv[0], "touch")) {
        if (a.argc < 2) {
            hal->print_text("usage: touch <file>");
        } else {
            build_json(buf, "write_file", a.argv[1], "data", "");
            hal->network_request(buf);
        }
    }

    else if (seq(a.argv[0], "pixel")) {
        if (a.argc < 4) {
            hal->print_text("usage: pixel <x> <y> <color>");
        } else {
            int x = str_to_int(a.argv[1]);
            int y = str_to_int(a.argv[2]);
            int c = hex_to_int(a.argv[3]);
            hal->draw_pixel(x, y, c);
        }
    }

    else if (seq(a.argv[0], "open")) {
        if (a.argc < 2) {
            hal->print_text("usage: open <file>");
        } else {
            int flags = 0;
            if (a.argc >= 3) flags = str_to_int(a.argv[2]);
            int fd = hal->fs_open(a.argv[1], flags);
            if (fd < 0) {
                hal->print_text("open failed");
            } else {
                scpy(buf, "fd ");
                char num[12];
                int_to_str(fd, num);
                scat(buf, num);
                hal->print_text(buf);
            }
        }
    }

    else if (seq(a.argv[0], "fread")) {
        if (a.argc < 2) {
            hal->print_text("usage: fread <fd>");
        } else {
            int fd = str_to_int(a.argv[1]);
            int n = hal->fs_read(fd, buf, 511);
            if (n < 0) {
                hal->print_text("read failed");
            } else {
                buf[n] = 0;
                hal->print_text(n == 0 ? "(empty)" : buf);
            }
        }
    }

    else if (seq(a.argv[0], "fwrite")) {
        if (a.argc < 3) {
            hal->print_text("usage: fwrite <fd> <data>");
        } else {
            int fd = str_to_int(a.argv[1]);
            char data[512];
            scpy(data, a.argv[2]);
            for (int i = 3; i < a.argc; i++) {
                scat(data, " ");
                scat(data, a.argv[i]);
            }
            int n = hal->fs_write(fd, data, slen(data));
            if (n < 0) {
                hal->print_text("write failed");
            } else {
                scpy(buf, "wrote ");
                char num[12];
                int_to_str(n, num);
                scat(buf, num);
                scat(buf, " bytes");
                hal->print_text(buf);
            }
        }
    }

    else if (seq(a.argv[0], "fclose")) {
        if (a.argc < 2) {
            hal->print_text("usage: fclose <fd>");
        } else {
            int fd = str_to_int(a.argv[1]);
            int r = hal->fs_close(fd);
            hal->print_text(r < 0 ? "close failed" : "closed");
        }
    }

    else {
        scpy(buf, a.argv[0]);
        scat(buf, ": command not found");
        hal->print_text(buf);
    }
}

static void show_prompt() {
    hal->set_prompt(prompt);
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