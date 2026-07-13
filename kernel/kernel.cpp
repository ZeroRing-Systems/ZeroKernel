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

static void cmd_tldr(const char* topic)
{
    if (!topic[0] || str::eq(topic, "tldr"))
    {
        hal->print("\033[1;36mtldr\033[0m - Simplified, community-driven command examples");
        hal->print("");
        hal->print("  \033[32mUsage:\033[0m tldr <command>");
        hal->print("");
        hal->print("  \033[33mExamples:\033[0m");
        hal->print("    tldr ls");
        hal->print("    tldr share");
        hal->print("    tldr chat");
        hal->print("");
        hal->print("  Available pages: help, clear, echo, version, whoami, pwd,");
        hal->print("  cd, ls, mkdir, rm, cat, write, edit, run, register, login,");
        hal->print("  cd, ls, mkdir, rm, cat, write, touch, edit, run, register, login,");
        hal->print("  logout, share, unshare, shared, upload, download, chat, zpm");
        return;
    }
    if (str::eq(topic, "help"))
    {
        hal->print("\033[1;36mhelp\033[0m - Display all available commands");
        hal->print("");
        hal->print("  \033[32m$\033[0m help");
        return;
    }
    if (str::eq(topic, "clear"))
    {
        hal->print("\033[1;36mclear\033[0m - Clear the terminal screen");
        hal->print("");
        hal->print("  \033[32m$\033[0m clear");
        return;
    }
    if (str::eq(topic, "echo"))
    {
        hal->print("\033[1;36mecho\033[0m - Print text to the terminal");
        hal->print("");
        hal->print("  Print a simple message:");
        hal->print("  \033[32m$\033[0m echo Hello, World!");
        hal->print("");
        hal->print("  Print a variable-like message:");
        hal->print("  \033[32m$\033[0m echo Build v1.0 deployed successfully");
        return;
    }
    if (str::eq(topic, "version"))
    {
        hal->print("\033[1;36mversion\033[0m - Show the ZeroRing kernel version");
        hal->print("");
        hal->print("  \033[32m$\033[0m version");
        return;
    }
    if (str::eq(topic, "whoami"))
    {
        hal->print("\033[1;36mwhoami\033[0m - Print the current user and session info");
        hal->print("");
        hal->print("  Check if you are logged in:");
        hal->print("  \033[32m$\033[0m whoami");
        hal->print("  \033[90manonymous (session: 8f032115...)\033[0m");
        hal->print("");
        hal->print("  After logging in:");
        hal->print("  \033[32m$\033[0m whoami");
        hal->print("  \033[90mifkabir\033[0m");
        return;
    }
    if (str::eq(topic, "pwd"))
    {
        hal->print("\033[1;36mpwd\033[0m - Print the current working directory");
        hal->print("");
        hal->print("  \033[32m$\033[0m pwd");
        hal->print("  \033[90m/\033[0m");
        return;
    }
    if (str::eq(topic, "cd"))
    {
        hal->print("\033[1;36mcd\033[0m - Change the current working directory");
        hal->print("");
        hal->print("  Navigate into a directory:");
        hal->print("  \033[32m$\033[0m cd projects");
        hal->print("");
        hal->print("  Go back to root:");
        hal->print("  \033[32m$\033[0m cd /");
        hal->print("");
        hal->print("  Go up one level:");
        hal->print("  \033[32m$\033[0m cd ..");
        return;
    }
    if (str::eq(topic, "ls"))
    {
        hal->print("\033[1;36mls\033[0m - List directory contents");
        hal->print("");
        hal->print("  List files in the current directory:");
        hal->print("  \033[32m$\033[0m ls");
        hal->print("");
        hal->print("  List files in a specific path:");
        hal->print("  \033[32m$\033[0m ls /shared");
        return;
    }
    if (str::eq(topic, "mkdir"))
    {
        hal->print("\033[1;36mmkdir\033[0m - Create a new directory");
        hal->print("");
        hal->print("  Create a project folder:");
        hal->print("  \033[32m$\033[0m mkdir projects");
        hal->print("");
        hal->print("  Create a nested directory:");
        hal->print("  \033[32m$\033[0m mkdir projects/webapp");
        return;
    }
    if (str::eq(topic, "rm"))
    {
        hal->print("\033[1;36mrm\033[0m - Remove a file or empty directory");
        hal->print("");
        hal->print("  Delete a file:");
        hal->print("  \033[32m$\033[0m rm old_script.py");
        hal->print("");
        hal->print("  Delete an empty directory:");
        hal->print("  \033[32m$\033[0m rm temp");
        return;
    }
    if (str::eq(topic, "cat"))
    {
        hal->print("\033[1;36mcat\033[0m - Print file contents to the terminal");
        hal->print("");
        hal->print("  View a file:");
        hal->print("  \033[32m$\033[0m cat readme.txt");
        hal->print("");
        hal->print("  View a shared file:");
        hal->print("  \033[32m$\033[0m cat /shared/welcome.txt");
        return;
    }
    if (str::eq(topic, "write"))
    {
        hal->print("\033[1;36mwrite\033[0m - Write data to a file (creates or overwrites)");
        hal->print("");
        hal->print("  Create a text file:");
        hal->print("  \033[32m$\033[0m write hello.txt Hello, ZeroRing!");
        hal->print("");
        hal->print("  Create a Python script:");
        hal->print("  \033[32m$\033[0m write game.py print('Hello!')");
        return;
    }
    if (str::eq(topic, "touch"))
    {
        hal->print("\033[1;36mtouch\033[0m - Create an empty file or update its timestamp");
        hal->print("");
        hal->print("  Create a new empty file:");
        hal->print("  \033[32m$\033[0m touch new_file.txt");
        return;
    }
    if (str::eq(topic, "edit"))
    {
        hal->print("\033[1;36medit\033[0m - Open a file in the built-in GUI editor");
        hal->print("");
        hal->print("  Edit an existing file:");
        hal->print("  \033[32m$\033[0m edit game.py");
        hal->print("");
        hal->print("  Create and edit a new file:");
        hal->print("  \033[32m$\033[0m write notes.md \"\"");
        hal->print("  \033[32m$\033[0m edit notes.md");
        return;
    }
    if (str::eq(topic, "run"))
    {
        hal->print("\033[1;36mrun\033[0m - Execute a script on the server sandbox");
        hal->print("");
        hal->print("  Run a Python script:");
        hal->print("  \033[32m$\033[0m run game.py");
        hal->print("");
        hal->print("  Run a JavaScript file:");
        hal->print("  \033[32m$\033[0m run app.js");
        hal->print("");
        hal->print("  Run a shell script:");
        hal->print("  \033[32m$\033[0m run deploy.sh");
        hal->print("");
        hal->print("  \033[33mSupported:\033[0m .py, .js, .sh");
        return;
    }
    if (str::eq(topic, "register"))
    {
        hal->print("\033[1;36mregister\033[0m - Create a new persistent user account");
        hal->print("");
        hal->print("  Register with a username and password:");
        hal->print("  \033[32m$\033[0m register alice mypassword");
        hal->print("");
        hal->print("  \033[33mNote:\033[0m Files are saved permanently under /users/<name>");
        return;
    }
    if (str::eq(topic, "login"))
    {
        hal->print("\033[1;36mlogin\033[0m - Log into your account");
        hal->print("");
        hal->print("  \033[32m$\033[0m login alice mypassword");
        hal->print("");
        hal->print("  \033[33mNote:\033[0m Your files from /users/<name> become accessible");
        return;
    }
    if (str::eq(topic, "logout"))
    {
        hal->print("\033[1;36mlogout\033[0m - Log out and return to anonymous session");
        hal->print("");
        hal->print("  \033[32m$\033[0m logout");
        return;
    }
    if (str::eq(topic, "share"))
    {
        hal->print("\033[1;36mshare\033[0m - Share a file with the world or a specific user");
        hal->print("");
        hal->print("  Share a file globally (visible to everyone):");
        hal->print("  \033[32m$\033[0m share game.py");
        hal->print("");
        hal->print("  Share a file privately with a user:");
        hal->print("  \033[32m$\033[0m share @alice game.py");
        hal->print("");
        hal->print("  \033[33mNote:\033[0m Global shares go to /shared/, private shares copy");
        hal->print("  the file directly into the target user's directory.");
        return;
    }
    if (str::eq(topic, "unshare"))
    {
        hal->print("\033[1;36munshare\033[0m - Remove a file from /shared/");
        hal->print("");
        hal->print("  \033[32m$\033[0m unshare game.py");
        return;
    }
    if (str::eq(topic, "shared"))
    {
        hal->print("\033[1;36mshared\033[0m - List all globally shared files");
        hal->print("");
        hal->print("  \033[32m$\033[0m shared");
        hal->print("");
        hal->print("  Read a shared file:");
        hal->print("  \033[32m$\033[0m cat /shared/game.py");
        return;
    }
    if (str::eq(topic, "upload"))
    {
        hal->print("\033[1;36mupload\033[0m - Upload a file from your computer");
        hal->print("");
        hal->print("  Upload to a specific path:");
        hal->print("  \033[32m$\033[0m upload myfile.txt");
        hal->print("");
        hal->print("  \033[33mTip:\033[0m You can also drag and drop files onto the terminal!");
        return;
    }
    if (str::eq(topic, "download"))
    {
        hal->print("\033[1;36mdownload\033[0m - Download a file to your computer");
        hal->print("");
        hal->print("  \033[32m$\033[0m download game.py");
        hal->print("");
        hal->print("  Download a shared file:");
        hal->print("  \033[32m$\033[0m download /shared/welcome.txt");
        return;
    }
    if (str::eq(topic, "chat"))
    {
        hal->print("\033[1;36mchat\033[0m - Send messages to other connected users");
        hal->print("");
        hal->print("  Broadcast to everyone (global channel):");
        hal->print("  \033[32m$\033[0m chat Hello everyone!");
        hal->print("");
        hal->print("  Send a private message to a specific user:");
        hal->print("  \033[32m$\033[0m chat @alice Hey, check out my script!");
        hal->print("");
        hal->print("  \033[33mTip:\033[0m Click the Chat button in the title bar to open");
        hal->print("  the chat panel for a full messaging experience.");
        return;
    }
    if (str::eq(topic, "zpm"))
    {
        hal->print("\033[1;36mzpm\033[0m - ZeroRing Package Manager");
        hal->print("");
        hal->print("  List available packages:");
        hal->print("  \033[32m$\033[0m zpm list");
        hal->print("");
        hal->print("  Publish a script to the global registry:");
        hal->print("  \033[32m$\033[0m zpm publish my_game.js");
        hal->print("");
        hal->print("  Install a package to your current directory:");
        hal->print("  \033[32m$\033[0m zpm install snake_game.js");
        return;
    }
    hal->print("tldr: page not found for '");
    hal->print(topic);
    hal->print("'. Try 'tldr' to see available pages.");
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
    hal->print("  touch <file>      Create an empty file");
    hal->print("  edit <file>       Open file in text editor");
    hal->print("  run <file>        Execute script (.py, .js, .sh)");
    hal->print("  register <u> <p>  Create a new user account");
    hal->print("  login <u> <p>     Log into your account");
    hal->print("  logout            Log out of your account");
    hal->print("  share <file>      Share a file publicly");
    hal->print("  unshare <file>    Remove a shared file");
    hal->print("  shared            List all shared files");
    hal->print("  upload <file>     Upload a local file");
    hal->print("  download <file>   Download a remote file");
    hal->print("  chat <msg>        Broadcast message to all users");
    hal->print("  tldr <cmd>        Show practical command examples");
    hal->print("  zpm <cmd>         ZeroRing Package Manager");
}

static char g_pipe_target[256];
static char g_redir_target[256];
static bool g_redir_append;

static void dispatch_cmd(const char* payload)
{
    if (!g_pipe_target[0] && !g_redir_target[0]) {
        hal->net_send(payload);
        return;
    }
    
    char new_payload[1024];
    int len = 0;
    while (payload[len] && payload[len] != '}') {
        new_payload[len] = payload[len];
        len++;
    }
    
    if (g_pipe_target[0]) {
        const char* pipe_key = ",\"pipe\":\"";
        for (int i = 0; pipe_key[i]; i++) new_payload[len++] = pipe_key[i];
        for (int i = 0; g_pipe_target[i]; i++) {
            if (g_pipe_target[i] == '"' || g_pipe_target[i] == '\\') new_payload[len++] = '\\';
            new_payload[len++] = g_pipe_target[i];
        }
        new_payload[len++] = '"';
    }
    if (g_redir_target[0]) {
        const char* red_key = ",\"redirect\":\"";
        for (int i = 0; red_key[i]; i++) new_payload[len++] = red_key[i];
        for (int i = 0; g_redir_target[i]; i++) {
            if (g_redir_target[i] == '"' || g_redir_target[i] == '\\') new_payload[len++] = '\\';
            new_payload[len++] = g_redir_target[i];
        }
        new_payload[len++] = '"';
        
        if (g_redir_append) {
            const char* app_key = ",\"append\":\"true\"";
            for (int i = 0; app_key[i]; i++) new_payload[len++] = app_key[i];
        }
    }
    new_payload[len++] = '}';
    new_payload[len] = '\0';
    hal->net_send(new_payload);
}

static void execute_command(char* input)
{
    g_pipe_target[0] = '\0';
    g_redir_target[0] = '\0';
    g_redir_append = false;

    char* pipe_pos = nullptr;
    char* redir_pos = nullptr;

    // Scan backwards so we don't mess up strings, simple for now
    for (int i = 0; input[i]; i++) {
        if (input[i] == '|' && !pipe_pos) pipe_pos = &input[i];
        if (input[i] == '>' && !redir_pos) redir_pos = &input[i];
    }

    if (redir_pos) {
        if (redir_pos[1] == '>') {
            g_redir_append = true;
            char resolved[256];
            str::resolve_path(cwd, str::trim(redir_pos + 2), resolved, 256);
            str::copy(g_redir_target, resolved, 256);
        } else {
            g_redir_append = false;
            char resolved[256];
            str::resolve_path(cwd, str::trim(redir_pos + 1), resolved, 256);
            str::copy(g_redir_target, resolved, 256);
        }
        *redir_pos = '\0';
    }
    else if (pipe_pos) {
        str::copy(g_pipe_target, str::trim(pipe_pos + 1), 256);
        *pipe_pos = '\0';
    }

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

    if (str::eq(trimmed, "tldr"))
    {
        cmd_tldr("");
        return;
    }
    if (str::starts_with(trimmed, "tldr "))
    {
        cmd_tldr(str::trim(trimmed + 5));
        return;
    }

    if (str::eq(trimmed, "version"))
    {
        hal->print(VERSION);
        return;
    }

    if (str::eq(trimmed, "whoami"))
    {
        dispatch_cmd(json::cmd("whoami_user"));
        return;
    }

    if (str::eq(trimmed, "pwd"))
    {
        hal->print(cwd);
        return;
    }

    if (str::starts_with(trimmed, "echo "))
    {
        const char* msg = str::trim(trimmed + 5);
        
        // Remove quotes if present
        char unquoted[256];
        str::copy(unquoted, msg, 256);
        if (unquoted[0] == '"') {
            int len = 0; while(unquoted[len]) len++;
            if (len > 1 && unquoted[len-1] == '"') {
                unquoted[len-1] = '\0';
                for(int i=0; unquoted[i]; i++) unquoted[i] = unquoted[i+1];
            }
        } else if (unquoted[0] == '\'') {
            int len = 0; while(unquoted[len]) len++;
            if (len > 1 && unquoted[len-1] == '\'') {
                unquoted[len-1] = '\0';
                for(int i=0; unquoted[i]; i++) unquoted[i] = unquoted[i+1];
            }
        }

        dispatch_cmd(json::cmd_path("echo", unquoted));
        return;
    }

    if (str::starts_with(trimmed, "cd "))
    {
        const char* target = str::trim(trimmed + 3);
        char resolved[256];
        str::resolve_path(cwd, target, resolved, 256);
        str::copy(pending_cd, resolved, 256);
        cd_pending = true;
        dispatch_cmd(json::cmd_path("stat", resolved));
        return;
    }

    if (str::eq(trimmed, "ls"))
    {
        dispatch_cmd(json::cmd_path("ls", cwd));
        return;
    }

    if (str::starts_with(trimmed, "ls "))
    {
        const char* path = str::trim(trimmed + 3);
        char resolved[256];
        str::resolve_path(cwd, path, resolved, 256);
        dispatch_cmd(json::cmd_path("ls", resolved));
        return;
    }

    if (str::starts_with(trimmed, "mkdir "))
    {
        const char* name = str::trim(trimmed + 6);
        char resolved[256];
        str::resolve_path(cwd, name, resolved, 256);
        dispatch_cmd(json::cmd_path("mkdir", resolved));
        return;
    }

    if (str::starts_with(trimmed, "cat "))
    {
        const char* file = str::trim(trimmed + 4);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        dispatch_cmd(json::cmd_path("cat", resolved));
        return;
    }

    if (str::starts_with(trimmed, "rm "))
    {
        const char* path = str::trim(trimmed + 3);
        char resolved[256];
        str::resolve_path(cwd, path, resolved, 256);
        dispatch_cmd(json::cmd_path("rm", resolved));
        return;
    }

    if (str::starts_with(trimmed, "touch "))
    {
        const char* file = str::trim(trimmed + 6);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        dispatch_cmd(json::cmd_path("touch", resolved));
        return;
    }

    if (str::starts_with(trimmed, "edit "))
    {
        const char* file = str::trim(trimmed + 5);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        dispatch_cmd(json::cmd_path("edit", resolved));
        return;
    }

    if (str::starts_with(trimmed, "run "))
    {
        const char* file = str::trim(trimmed + 4);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        dispatch_cmd(json::cmd_path("run", resolved));
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
        dispatch_cmd(json::cmd("logout"));
        return;
    }

    if (str::starts_with(trimmed, "share "))
    {
        const char* args = str::trim(trimmed + 6);
        char target_user[128] = {0};
        char file[256] = {0};

        if (args[0] == '@')
        {
            int i = 0;
            while (args[i] && args[i] != ' ')
            {
                if (i > 0 && i < 127)
                    target_user[i - 1] = args[i];
                i++;
            }
            if (args[i] == ' ')
            {
                str::copy(file, str::trim(args + i), 256);
            }
        }
        else
        {
            str::copy(file, args, 256);
        }

        if (file[0])
        {
            char resolved[256];
            str::resolve_path(cwd, file, resolved, 256);

            char buf[1024];
            int pos = str::copy(buf, "{\"cmd\":\"share\",\"path\":\"", 1024);
            pos = str::append(buf, pos, resolved, 1024);
            pos = str::append(buf, pos, "\"", 1024);
            if (target_user[0])
            {
                pos = str::append(buf, pos, ",\"target\":\"", 1024);
                pos = str::append(buf, pos, target_user, 1024);
                pos = str::append(buf, pos, "\"", 1024);
            }
            pos = str::append(buf, pos, "}", 1024);
            hal->net_send(buf);
        }
        else
        {
            hal->print("usage: share [@user] <file>");
        }
        return;
    }

    if (str::starts_with(trimmed, "unshare "))
    {
        const char* file = str::trim(trimmed + 8);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        dispatch_cmd(json::cmd_path("unshare", resolved));
        return;
    }

    if (str::eq(trimmed, "shared"))
    {
        dispatch_cmd(json::cmd("shared"));
        return;
    }

    if (str::starts_with(trimmed, "chat "))
    {
        const char* msg = str::trim(trimmed + 5);
        if (msg[0])
        {
            dispatch_cmd(json::cmd_path("chat", msg)); // We safely escape using path
        }
        else
        {
            hal->print("Usage: chat [@user] <msg>");
        }
        return;
    }

    if (str::starts_with(trimmed, "zpm "))
    {
        const char* args = str::trim(trimmed + 4);
        if (str::eq(args, "list"))
        {
            dispatch_cmd(json::cmd("shared"));
        }
        else if (str::starts_with(args, "publish "))
        {
            const char* file = str::trim(args + 8);
            if (file[0])
            {
                char resolved[256];
                str::resolve_path(cwd, file, resolved, 256);
                dispatch_cmd(json::cmd_path("share", resolved));
            }
            else
            {
                hal->print("Usage: zpm publish <file>");
            }
        }
        else if (str::starts_with(args, "install "))
        {
            const char* pkg = str::trim(args + 8);
            if (pkg[0])
            {
                char payload[512];
                int idx = 0;
                const char* p1 = "{\"cmd\":\"zpm_install\",\"package\":\"";
                while (*p1 && idx < 500)
                    payload[idx++] = *p1++;

                for (int i = 0; pkg[i] && idx < 500; i++)
                {
                    if (pkg[i] == '"' || pkg[i] == '\\')
                        payload[idx++] = '\\';
                    payload[idx++] = pkg[i];
                }

                const char* p2 = "\",\"cwd\":\"";
                while (*p2 && idx < 500)
                    payload[idx++] = *p2++;

                for (int i = 0; cwd[i] && idx < 500; i++)
                {
                    if (cwd[i] == '"' || cwd[i] == '\\')
                        payload[idx++] = '\\';
                    payload[idx++] = cwd[i];
                }

                payload[idx++] = '"';
                payload[idx++] = '}';
                payload[idx] = '\0';

                dispatch_cmd(payload);
            }
            else
            {
                hal->print("Usage: zpm install <package>");
            }
        }
        else
        {
            hal->print("ZeroRing Package Manager (zpm)");
            hal->print("Usage:");
            hal->print("  zpm list               - List all available packages");
            hal->print("  zpm publish <file>     - Publish a script to the global registry");
            hal->print("  zpm install <package>  - Install a package to current directory");
        }
        return;
    }

    if (str::starts_with(trimmed, "download "))
    {
        const char* file = str::trim(trimmed + 9);
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        dispatch_cmd(json::cmd_path("download", resolved));
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