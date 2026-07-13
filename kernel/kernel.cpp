#include "../interfaces/hal.h"

extern "C" HAL* hal_get(void);
extern "C" char js_scratch_buf[4096];
char js_scratch_buf[4096];

namespace str
{

static int len(const char* s)
{
    int n = 0;
    while (s[n])
        n++;
    return n;
}

static bool eq(const char* a, const char* b)
{
    while (*a && *b)
    {
        if (*a != *b)
            return false;
        a++;
        b++;
    }
    return *a == *b;
}

static bool starts_with(const char* s, const char* prefix)
{
    while (*prefix)
    {
        if (*s != *prefix)
            return false;
        s++;
        prefix++;
    }
    return true;
}

static int copy(char* dst, const char* src, int max)
{
    int i = 0;
    while (src[i] && i < max - 1)
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return i;
}

static int append(char* dst, int dst_len, const char* src, int max)
{
    int i = 0;
    while (src[i] && dst_len + i < max - 1)
    {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';
    return dst_len + i;
}

static const char* trim(const char* s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

static const char* after_space(const char* s)
{
    while (*s && *s != ' ')
        s++;
    if (*s == ' ')
    {
        s++;
        return s;
    }
    return nullptr;
}

static void resolve_path(const char* cwd, const char* target, char* resolved, int max)
{
    if (target[0] == '/')
    {
        copy(resolved, target, max);
    }
    else if (eq(target, ".."))
    {
        copy(resolved, cwd, max);
        int l = len(resolved);
        if (l > 1)
        {
            l--;
            while (l > 0 && resolved[l] != '/')
                l--;
            if (l == 0)
                l = 1;
            resolved[l] = '\0';
        }
    }
    else
    {
        copy(resolved, cwd, max);
        int l = len(resolved);
        if (l > 1)
        {
            l = append(resolved, l, "/", max);
        }
        append(resolved, l, target, max);
    }
}

} // namespace str

namespace json
{

static const int BUF_SIZE = 1024;
static char buf[BUF_SIZE];

static const char* cmd(const char* command)
{
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

static const char* cmd_path(const char* command, const char* path)
{
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"path\":\"", BUF_SIZE);
    pos = str::append(buf, pos, path, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

static const char* cmd_save(const char* path, const char* data)
{
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"save\",\"path\":\"", BUF_SIZE);
    pos = str::append(buf, pos, path, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"data\":\"", BUF_SIZE);

    int i = 0;
    while (data[i] && pos < BUF_SIZE - 4)
    {
        if (data[i] == '"')
        {
            buf[pos++] = '\\';
            buf[pos++] = '"';
        }
        else if (data[i] == '\\')
        {
            buf[pos++] = '\\';
            buf[pos++] = '\\';
        }
        else
        {
            buf[pos++] = data[i];
        }
        i++;
    }

    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

} // namespace json

static HAL* hal = nullptr;
static char line[256];
static int line_pos = 0;
static char cwd[256] = "/";
static char pending_cd[256] = "";
static bool cd_pending = false;

static const char* VERSION = "ZeroRing OS v2.0.0";
static const char* PROMPT_FMT = "zeroring:";

static char prompt_buf[320];

static void refresh_prompt()
{
    int pos = 0;
    pos = str::copy(prompt_buf, PROMPT_FMT, 320);
    pos = str::append(prompt_buf, pos, cwd, 320);
    pos = str::append(prompt_buf, pos, "$ ", 320);
    hal->set_prompt(prompt_buf);
}

static void cmd_help()
{
    hal->print("Built-in commands:");
    hal->print("  help              Show this message");
    hal->print("  clear             Clear the terminal");
    hal->print("  echo <text>       Print text to terminal");
    hal->print("  version           Show kernel version");
    hal->print("  whoami            Print current user");
    hal->print("  pwd               Print working directory");
    hal->print("  cd <path>         Change directory");
    hal->print("  ls [path]         List directory contents");
    hal->print("  mkdir <path>      Create a directory");
    hal->print("  rm <path>         Remove a file or empty dir");
    hal->print("  cat <file>        Print file contents");
    hal->print("  write <f> <data>  Write data to a file");
    hal->print("  edit <file>       Open file in text editor");
    hal->print("  run <file>        Execute script (.py, .js, .sh)");
    hal->print("  register <u > <p> Create a new user account");
    hal->print("  login <u> <p>     Log into your account");
    hal->print("  logout            Log out of your account");
    hal->print("  share <file>      Share a file publicly");
    hal->print("  unshare <file>    Remove a shared file");
    hal->print("  shared            List all shared files");
    hal->print("  upload <file>     Upload a local file");
    hal->print("  download <file>   Download a remote file");
}

static void execute_command(char* input)
{
    const char* trimmed = str::trim(input);
    if (trimmed[0] == '\0')
        return;

    if (str::eq(trimmed, "clear"))
    {
        hal->clear_screen();
        return;
    }

    if (str::eq(trimmed, "help"))
    {
        cmd_help();
        return;
    }

    if (str::eq(trimmed, "version"))
    {
        hal->print(VERSION);
        return;
    }

    if (str::eq(trimmed, "whoami"))
    {
        hal->net_send(json::cmd("whoami_user"));
        return;
    }

    if (str::eq(trimmed, "pwd"))
    {
        hal->print(cwd);
        return;
    }

    if (str::starts_with(trimmed, "echo "))
    {
        hal->print(trimmed + 5);
        return;
    }

    if (str::starts_with(trimmed, "cd "))
    {
        const char* target = str::trim(trimmed + 3);
        char resolved[256];
        str::resolve_path(cwd, target, resolved, 256);
        str::copy(pending_cd, resolved, 256);
        cd_pending = true;
        hal->net_send(json::cmd_path("stat", resolved));
        return;
    }

    if (str::eq(trimmed, "ls"))
    {
        hal->net_send(json::cmd_path("ls", cwd));
        return;
    }

    if (str::starts_with(trimmed, "ls "))
    {
        const char* path = str::trim(trimmed + 3);
        char resolved[256];
        str::resolve_path(cwd, path, resolved, 256);
        hal->net_send(json::cmd_path("ls", resolved));
        return;
    }

    if (str::starts_with(trimmed, "mkdir "))
    {
        const char* name = str::trim(trimmed + 6);
        char resolved[256];
        str::resolve_path(cwd, name, resolved, 256);
        hal->net_send(json::cmd_path("mkdir", resolved));
        return;
    }

    if (str::starts_with(trimmed, "cat "))
    {
        const char* file = str::trim(trimmed + 4);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        hal->net_send(json::cmd_path("cat", resolved));
        return;
    }

    if (str::starts_with(trimmed, "rm "))
    {
        const char* path = str::trim(trimmed + 3);
        char resolved[256];
        str::resolve_path(cwd, path, resolved, 256);
        hal->net_send(json::cmd_path("rm", resolved));
        return;
    }

    if (str::starts_with(trimmed, "edit "))
    {
        const char* file = str::trim(trimmed + 5);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        hal->net_send(json::cmd_path("edit", resolved));
        return;
    }

    if (str::starts_with(trimmed, "run "))
    {
        const char* file = str::trim(trimmed + 4);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        hal->net_send(json::cmd_path("run", resolved));
        return;
    }

    if (str::starts_with(trimmed, "write "))
    {
        const char* rest = str::trim(trimmed + 6);
        const char* data = str::after_space(rest);
        if (data)
        {
            char fname[128];
            int i = 0;
            while (rest[i] && rest[i] != ' ' && i < 127)
            {
                fname[i] = rest[i];
                i++;
            }
            fname[i] = '\0';
            char resolved[256];
            str::resolve_path(cwd, fname, resolved, 256);
            hal->net_send(json::cmd_save(resolved, data));
        }
        else
        {
            hal->print("usage: write <filename> <data>");
        }
        return;
    }

    if (str::starts_with(trimmed, "register "))
    {
        const char* rest = str::trim(trimmed + 9);
        const char* password = str::after_space(rest);
        if (password)
        {
            char uname[128];
            int i = 0;
            while (rest[i] && rest[i] != ' ' && i < 127)
            {
                uname[i] = rest[i];
                i++;
            }
            uname[i] = '\0';

            // Format JSON: {"cmd":"register","username":"...","password":"..."}
            char buf[512];
            int pos = 0;
            pos = str::copy(buf, "{\"cmd\":\"register\",\"username\":\"", 512);
            pos = str::append(buf, pos, uname, 512);
            pos = str::append(buf, pos, "\",\"password\":\"", 512);
            pos = str::append(buf, pos, password, 512);
            pos = str::append(buf, pos, "\"}", 512);
            hal->net_send(buf);
        }
        else
        {
            hal->print("usage: register <username> <password>");
        }
        return;
    }

    if (str::starts_with(trimmed, "login "))
    {
        const char* rest = str::trim(trimmed + 6);
        const char* password = str::after_space(rest);
        if (password)
        {
            char uname[128];
            int i = 0;
            while (rest[i] && rest[i] != ' ' && i < 127)
            {
                uname[i] = rest[i];
                i++;
            }
            uname[i] = '\0';

            // Format JSON: {"cmd":"login","username":"...","password":"..."}
            char buf[512];
            int pos = 0;
            pos = str::copy(buf, "{\"cmd\":\"login\",\"username\":\"", 512);
            pos = str::append(buf, pos, uname, 512);
            pos = str::append(buf, pos, "\",\"password\":\"", 512);
            pos = str::append(buf, pos, password, 512);
            pos = str::append(buf, pos, "\"}", 512);

            // Reset cwd to / on login
            cwd[0] = '/';
            cwd[1] = '\0';

            hal->net_send(buf);
        }
        else
        {
            hal->print("usage: login <username> <password>");
        }
        return;
    }

    if (str::eq(trimmed, "logout"))
    {
        cwd[0] = '/';
        cwd[1] = '\0';
        hal->net_send(json::cmd("logout"));
        return;
    }

    if (str::starts_with(trimmed, "share "))
    {
        const char* file = str::trim(trimmed + 6);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        hal->net_send(json::cmd_path("share", resolved));
        return;
    }

    if (str::starts_with(trimmed, "unshare "))
    {
        const char* file = str::trim(trimmed + 8);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        hal->net_send(json::cmd_path("unshare", resolved));
        return;
    }

    if (str::eq(trimmed, "shared"))
    {
        hal->net_send(json::cmd("shared"));
        return;
    }

    if (str::starts_with(trimmed, "download "))
    {
        const char* file = str::trim(trimmed + 9);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        hal->net_send(json::cmd_path("download", resolved));
        return;
    }

    if (str::starts_with(trimmed, "upload "))
    {
        const char* file = str::trim(trimmed + 7);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        // We tell JS to prompt for a file and then it sends the upload command.
        // Format: __upload__<resolved_path>
        char buf[256];
        int pos = str::copy(buf, "__upload__", 256);
        str::append(buf, pos, resolved, 256);
        hal->print(buf);
        return;
    }

    hal->print("zeroring: command not found: ");
    hal->print(trimmed);
    hal->print("Type 'help' for available commands.");
}

extern "C" void handle_key(int key)
{
    if (key == 13)
    {
        line[line_pos] = '\0';
        execute_command(line);
        line_pos = 0;
        line[0] = '\0';
        refresh_prompt();
        return;
    }

    if (key == 21)
    {
        line_pos = 0;
        line[0] = '\0';
        return;
    }

    if (key == 8 || key == 127)
    {
        if (line_pos > 0)
            line_pos--;
        line[line_pos] = '\0';
        return;
    }

    if (line_pos < 255 && key >= 32 && key < 127)
    {
        line[line_pos++] = (char)key;
        line[line_pos] = '\0';
    }
}

extern "C" void handle_net_response(const char* json_response)
{
    if (!json_response)
        return;

    if (str::starts_with(json_response, "__stat__"))
    {
        if (cd_pending)
        {
            cd_pending = false;
            if (str::eq(json_response, "__stat__dir"))
            {
                str::copy(cwd, pending_cd, 256);
                refresh_prompt();
            }
            else
            {
                hal->print("cd: no such directory: ");
                hal->print(pending_cd);
            }
            pending_cd[0] = '\0';
        }
        return;
    }

    hal->print(json_response);
}

extern "C" void kernel_main()
{
    hal = hal_get();
    hal->print(VERSION);
    hal->print("Type 'help' for available commands.");
    hal->print("");
    refresh_prompt();
}