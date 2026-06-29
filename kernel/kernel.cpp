// ============================================================================
// kernel.cpp — ZeroKernel main: freestanding C++ (no standard library)
// ============================================================================
// This is the core of ZeroRing OS. It runs inside the browser as WebAssembly.
// All I/O goes through the HAL function-pointer table. The kernel handles
// keystrokes, parses commands, and dispatches network requests to the
// ZeroRing-Cloud backend via JSON over WebSocket.
//
// CONSTRAINT: No #include <string>, <vector>, <iostream>, etc.
//             Only hal.h and raw C constructs.
// ============================================================================
#include "../interfaces/hal.h"

extern "C" HAL* hal_get(void);

// ---------------------------------------------------------------------------
// Freestanding string utilities
// ---------------------------------------------------------------------------
namespace str {

static int len(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static bool eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

static bool starts_with(const char* s, const char* prefix) {
    while (*prefix) {
        if (*s != *prefix) return false;
        s++; prefix++;
    }
    return true;
}

// Copy src into dst, return number of chars written (excluding nul).
static int copy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return i;
}

// Append src onto dst (dst must have room). Returns new total length.
static int append(char* dst, int dst_len, const char* src, int max) {
    int i = 0;
    while (src[i] && dst_len + i < max - 1) {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';
    return dst_len + i;
}

// Skip leading whitespace, return pointer into same buffer.
static const char* trim(const char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

// Return pointer to first char after the first space, or nullptr if none.
static const char* after_space(const char* s) {
    while (*s && *s != ' ') s++;
    if (*s == ' ') {
        s++;
        return s;
    }
    return nullptr;
}

} // namespace str

// ---------------------------------------------------------------------------
// JSON builder — constructs command payloads without any allocator
// ---------------------------------------------------------------------------
namespace json {

static const int BUF_SIZE = 1024;
static char buf[BUF_SIZE];

// Build: {"cmd":"<cmd>"}
static const char* cmd(const char* command) {
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

// Build: {"cmd":"<cmd>","path":"<path>"}
static const char* cmd_path(const char* command, const char* path) {
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"path\":\"", BUF_SIZE);
    pos = str::append(buf, pos, path, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

// Build: {"cmd":"save","path":"<path>","data":"<data>"}
static const char* cmd_save(const char* path, const char* data) {
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"save\",\"path\":\"", BUF_SIZE);
    pos = str::append(buf, pos, path, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"data\":\"", BUF_SIZE);
    pos = str::append(buf, pos, data, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

} // namespace json

// ---------------------------------------------------------------------------
// Kernel state
// ---------------------------------------------------------------------------
static HAL*  hal         = nullptr;
static char  line[256];
static int   line_pos    = 0;
static char  cwd[256]    = "/";

static const char* VERSION = "ZeroRing OS v0.2.0";
static const char* PROMPT_FMT = "zeroring:";

// ---------------------------------------------------------------------------
// Build the prompt string: "zeroring:/path$ "
// ---------------------------------------------------------------------------
static char prompt_buf[320];

static void refresh_prompt() {
    int pos = 0;
    pos = str::copy(prompt_buf, PROMPT_FMT, 320);
    pos = str::append(prompt_buf, pos, cwd, 320);
    pos = str::append(prompt_buf, pos, "$ ", 320);
    hal->set_prompt(prompt_buf);
}

// ---------------------------------------------------------------------------
// Built-in command table
// ---------------------------------------------------------------------------
static void cmd_help() {
    hal->print("Built-in commands:");
    hal->print("  help              Show this message");
    hal->print("  clear             Clear the terminal");
    hal->print("  echo <text>       Print text to terminal");
    hal->print("  version           Show kernel version");
    hal->print("  whoami            Show current user");
    hal->print("");
    hal->print("Filesystem (remote):");
    hal->print("  ls [path]         List directory");
    hal->print("  cd <path>         Change directory");
    hal->print("  mkdir <name>      Create directory");
    hal->print("  cat <file>        Read file contents");
    hal->print("  write <f> <data>  Write data to file");
    hal->print("  rm <path>         Remove file or directory");
    hal->print("  pwd               Print working directory");
}

static void execute_command(char* input) {
    const char* trimmed = str::trim(input);
    if (trimmed[0] == '\0') return;

    // ---- Local built-ins ----
    if (str::eq(trimmed, "clear")) {
        hal->clear_screen();
        return;
    }

    if (str::eq(trimmed, "help")) {
        cmd_help();
        return;
    }

    if (str::eq(trimmed, "version")) {
        hal->print(VERSION);
        return;
    }

    if (str::eq(trimmed, "whoami")) {
        hal->print("root");
        return;
    }

    if (str::eq(trimmed, "pwd")) {
        hal->print(cwd);
        return;
    }

    if (str::starts_with(trimmed, "echo ")) {
        hal->print(trimmed + 5);
        return;
    }

    // ---- cd is handled locally (updates cwd) + notifies backend ----
    if (str::starts_with(trimmed, "cd ")) {
        const char* target = str::trim(trimmed + 3);
        if (target[0] == '/') {
            str::copy(cwd, target, 256);
        } else if (str::eq(target, "..")) {
            // Navigate up: find last '/' and truncate
            int l = str::len(cwd);
            if (l > 1) {
                l--; // skip trailing char
                while (l > 0 && cwd[l] != '/') l--;
                if (l == 0) l = 1; // keep root "/"
                cwd[l] = '\0';
            }
        } else {
            int l = str::len(cwd);
            if (l > 1) {
                l = str::append(cwd, l, "/", 256);
            }
            str::append(cwd, l, target, 256);
        }
        refresh_prompt();
        return;
    }

    // ---- Remote filesystem commands (dispatched to backend via WebSocket) ----
    if (str::eq(trimmed, "ls")) {
        hal->net_send(json::cmd_path("ls", cwd));
        return;
    }

    if (str::starts_with(trimmed, "ls ")) {
        const char* path = str::trim(trimmed + 3);
        hal->net_send(json::cmd_path("ls", path));
        return;
    }

    if (str::starts_with(trimmed, "mkdir ")) {
        const char* name = str::trim(trimmed + 6);
        hal->net_send(json::cmd_path("mkdir", name));
        return;
    }

    if (str::starts_with(trimmed, "cat ")) {
        const char* file = str::trim(trimmed + 4);
        hal->net_send(json::cmd_path("cat", file));
        return;
    }

    if (str::starts_with(trimmed, "rm ")) {
        const char* path = str::trim(trimmed + 3);
        hal->net_send(json::cmd_path("rm", path));
        return;
    }

    if (str::starts_with(trimmed, "write ")) {
        // write <filename> <data...>
        const char* rest = str::trim(trimmed + 6);
        const char* data = str::after_space(rest);
        if (data) {
            // extract filename
            char fname[128];
            int i = 0;
            while (rest[i] && rest[i] != ' ' && i < 127) {
                fname[i] = rest[i];
                i++;
            }
            fname[i] = '\0';
            hal->net_send(json::cmd_save(fname, data));
        } else {
            hal->print("usage: write <filename> <data>");
        }
        return;
    }

    // ---- Unknown ----
    hal->print("zeroring: command not found: ");
    hal->print(trimmed);
    hal->print("Type 'help' for available commands.");
}

// ---------------------------------------------------------------------------
// Exported WASM entry points
// ---------------------------------------------------------------------------

// Called by terminal.js on every keystroke.
extern "C" void handle_key(int key) {
    // Enter
    if (key == 13) {
        line[line_pos] = '\0';
        execute_command(line);
        line_pos = 0;
        line[0] = '\0';
        refresh_prompt();
        return;
    }

    // Backspace / Delete
    if (key == 8 || key == 127) {
        if (line_pos > 0) line_pos--;
        line[line_pos] = '\0';
        return;
    }

    // Printable ASCII
    if (line_pos < 255 && key >= 32 && key < 127) {
        line[line_pos++] = (char)key;
        line[line_pos] = '\0';
    }
}

// Called by terminal.js when backend sends a WebSocket response.
extern "C" void handle_net_response(const char* json_response) {
    // For now, print the raw response. A future version will parse the
    // JSON and route it to the appropriate kernel subsystem.
    if (json_response) {
        hal->print(json_response);
    }
}

// Called once on page load after WASM instantiation.
extern "C" void kernel_main() {
    hal = hal_get();
    hal->print(VERSION);
    hal->print("Type 'help' for available commands.");
    hal->print("");
    refresh_prompt();
}