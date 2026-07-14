#include "../interfaces/hal.h"

extern "C" HAL* hal_get(void);
extern "C" char js_scratch_buf[4096];
char js_scratch_buf[4096];
extern char g_home[256];

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

static int append(char* dst, int pos, const char* src, int max)
{
    int i = 0;
    while (src[i] && pos < max - 1)
    {
        dst[pos++] = src[i++];
    }
    dst[pos] = '\0';
    return pos;
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
    if (target[0] == '~' && (target[1] == '\0' || target[1] == '/'))
    {
        copy(resolved, g_home, max);
        if (target[1] == '/')
        {
            int l = len(resolved);
            if (l > 1 && resolved[l-1] != '/')
            {
                l = append(resolved, l, "/", max);
            }
            append(resolved, l, target + 2, max);
        }
    }
    else if (target[0] == '/')
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
            // If we are sitting on a trailing slash, step back one more
            if (l > 0 && resolved[l] == '/')
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
        if (l > 1 && resolved[l-1] != '/')
        {
            l = append(resolved, l, "/", max);
        }
        append(resolved, l, target, max);
    }

    // Clean any trailing slash (unless the path is exactly "/")
    int final_l = len(resolved);
    if (final_l > 1 && resolved[final_l-1] == '/')
    {
        resolved[final_l-1] = '\0';
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

static const char* cmd_url(const char* command, const char* url)
{
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"url\":\"", BUF_SIZE);
    pos = str::append(buf, pos, url, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

static const char* cmd_path_mode(const char* command, const char* path, const char* mode)
{
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"path\":\"", BUF_SIZE);
    pos = str::append(buf, pos, path, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"mode\":\"", BUF_SIZE);
    pos = str::append(buf, pos, mode, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

static const char* cmd_path_name(const char* command, const char* path, const char* name)
{
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"path\":\"", BUF_SIZE);
    pos = str::append(buf, pos, path, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"name\":\"", BUF_SIZE);
    pos = str::append(buf, pos, name, BUF_SIZE);
    pos = str::append(buf, pos, "\"}", BUF_SIZE);
    return buf;
}

static const char* cmd_src_dest(const char* command, const char* src, const char* dest)
{
    int pos = 0;
    pos = str::copy(buf, "{\"cmd\":\"", BUF_SIZE);
    pos = str::append(buf, pos, command, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"src\":\"", BUF_SIZE);
    pos = str::append(buf, pos, src, BUF_SIZE);
    pos = str::append(buf, pos, "\",\"dest\":\"", BUF_SIZE);
    pos = str::append(buf, pos, dest, BUF_SIZE);
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
        hal->print("  \033[33mNote:\033[0m Private shares copy the file directly into");
        hal->print("  the target user's /shared directory.");
        return;
    }
    if (str::eq(topic, "unshare"))
    {
        hal->print("\033[1;36munshare\033[0m - Remove a published file from global registry");
        hal->print("");
        hal->print("  \033[32m$\033[0m unshare game.py");
        return;
    }
    if (str::eq(topic, "shared"))
    {
        hal->print("\033[1;36mshared\033[0m - List all globally shared files");
        hal->print("");
        hal->print("  \033[32m$\033[0m shared");
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
    if (str::eq(topic, "fetch") || str::eq(topic, "curl"))
    {
        hal->print("\033[1;36mfetch / curl\033[0m - Web Fetch / HTTP Client");
        hal->print("");
        hal->print("  Fetch JSON from an API:");
        hal->print("  \033[32m$\033[0m fetch https://api.github.com/users/ifkabir");
        hal->print("");
        hal->print("  Save API response to a local file using redirection:");
        hal->print("  \033[32m$\033[0m fetch https://api.github.com/users/ifkabir > user.json");
        return;
    }
    if (str::eq(topic, "export") || str::eq(topic, "env") || str::eq(topic, "unset"))
    {
        hal->print("\033[1;36mexport / env / unset\033[0m - Environment Variables");
        hal->print("");
        hal->print("  Set an environment variable:");
        hal->print("  \033[32m$\033[0m export FOO=bar");
        hal->print("  \033[32m$\033[0m echo $FOO");
        hal->print("");
        hal->print("  List all environment variables ($USER, $HOME, $PWD, custom):");
        hal->print("  \033[32m$\033[0m env");
        hal->print("");
        hal->print("  Unset a custom variable:");
        hal->print("  \033[32m$\033[0m unset FOO");
        return;
    }
    if (str::eq(topic, "mv") || str::eq(topic, "rename"))
    {
        hal->print("\033[1;36mmv\033[0m - Move or Rename Files / Directories");
        hal->print("");
        hal->print("  Rename a file:");
        hal->print("  \033[32m$\033[0m mv old.txt new.txt");
        hal->print("");
        hal->print("  Move a file into a directory:");
        hal->print("  \033[32m$\033[0m mv file.txt docs");
        return;
    }
    if (str::eq(topic, "cp") || str::eq(topic, "copy"))
    {
        hal->print("\033[1;36mcp\033[0m - Copy / Duplicate Files");
        hal->print("");
        hal->print("  Duplicate a file:");
        hal->print("  \033[32m$\033[0m cp source.txt backup.txt");
        hal->print("");
        hal->print("  Copy a file into a directory:");
        hal->print("  \033[32m$\033[0m cp notes.txt docs");
        return;
    }
    if (str::eq(topic, "head"))
    {
        hal->print("\033[1;36mhead\033[0m - Output first part of files / pipe");
        hal->print("");
        hal->print("  Print first 10 lines of a file:");
        hal->print("  \033[32m$\033[0m head file.txt");
        hal->print("");
        hal->print("  Print first 5 lines:");
        hal->print("  \033[32m$\033[0m head -n 5 file.txt");
        return;
    }
    if (str::eq(topic, "tail"))
    {
        hal->print("\033[1;36mtail\033[0m - Output last part of files / pipe");
        hal->print("");
        hal->print("  Print last 10 lines of a file:");
        hal->print("  \033[32m$\033[0m tail file.txt");
        hal->print("");
        hal->print("  Print last 5 lines:");
        hal->print("  \033[32m$\033[0m tail -n 5 file.txt");
        return;
    }
    if (str::eq(topic, "wc"))
    {
        hal->print("\033[1;36mwc\033[0m - Print line, word, and byte counts");
        hal->print("");
        hal->print("  Count lines, words, chars in a file:");
        hal->print("  \033[32m$\033[0m wc file.txt");
        hal->print("");
        hal->print("  Count only lines (-l), words (-w), or chars (-c):");
        hal->print("  \033[32m$\033[0m wc -l file.txt");
        return;
    }
    if (str::eq(topic, "chmod"))
    {
        hal->print("\033[1;36mchmod\033[0m - Change file access permissions");
        hal->print("");
        hal->print("  Add executable permission to a script:");
        hal->print("  \033[32m$\033[0m chmod +x script.py");
        hal->print("");
        hal->print("  Set permissions using numeric mode:");
        hal->print("  \033[32m$\033[0m chmod 755 script.py");
        return;
    }
    if (str::eq(topic, "alias") || str::eq(topic, "unalias"))
    {
        hal->print("\033[1;36malias / unalias\033[0m - Command Aliasing");
        hal->print("");
        hal->print("  Create a command alias:");
        hal->print("  \033[32m$\033[0m alias ll=\"ls -l\"");
        hal->print("  \033[32m$\033[0m ll /users");
        hal->print("");
        hal->print("  List all active aliases:");
        hal->print("  \033[32m$\033[0m alias");
        hal->print("");
        hal->print("  Remove an alias:");
        hal->print("  \033[32m$\033[0m unalias ll");
        return;
    }
    if (str::eq(topic, "ls"))
    {
        hal->print("\033[1;36mls\033[0m - List directory contents");
        hal->print("");
        hal->print("  List files in current directory:");
        hal->print("  \033[32m$\033[0m ls");
        hal->print("");
        hal->print("  List files with permissions and sizes (long format):");
        hal->print("  \033[32m$\033[0m ls -l");
        return;
    }
    if (str::eq(topic, "find"))
    {
        hal->print("\033[1;36mfind\033[0m - Search for files in a directory hierarchy");
        hal->print("");
        hal->print("  Find all Python files in current directory:");
        hal->print("  \033[32m$\033[0m find . -name \"*.py\"");
        hal->print("");
        hal->print("  Find all files matching pattern anywhere inside /shared:");
        hal->print("  \033[32m$\033[0m find /shared -name \"*.txt\"");
        return;
    }
    if (str::eq(topic, "tree"))
    {
        hal->print("\033[1;36mtree\033[0m - List contents of directories in a tree-like format");
        hal->print("");
        hal->print("  Display tree of current directory:");
        hal->print("  \033[32m$\033[0m tree");
        hal->print("");
        hal->print("  Display tree of a specific directory:");
        hal->print("  \033[32m$\033[0m tree /shared");
        return;
    }
    if (str::eq(topic, "run"))
    {
        hal->print("\033[1;36mrun\033[0m - Execute a script file (.py, .js, .sh)");
        hal->print("");
        hal->print("  Execute a Python script:");
        hal->print("  \033[32m$\033[0m run script.py");
        return;
    }
    hal->print("tldr: page not found. Try 'tldr' to see available pages.");
}

static void cmd_help()
{
    hal->print("Available commands:");
    hal->print("  help              Show this help message");
    hal->print("  clear             Clear the terminal");
    hal->print("  echo <text>       Print text to terminal");
    hal->print("  version           Show kernel version");
    hal->print("  whoami            Print current user");
    hal->print("  pwd               Print working directory");
    hal->print("  cd <path>         Change directory");
    hal->print("  ls [-l] [path]    List directory contents");
    hal->print("  find [p] [-name]  Search for files recursively");
    hal->print("  tree [path]       Visual directory tree");
    hal->print("  chmod <mode> <f>  Change file permissions (+x, 755)");
    hal->print("  mkdir <path>      Create a directory");
    hal->print("  rm <path>         Remove a file or empty dir");
    hal->print("  mv <src> <dest>   Move or rename a file/dir");
    hal->print("  cp <src> <dest>   Copy a file");
    hal->print("  cat <file>        Print file contents");
    hal->print("  head [-n N] <f>   Print first N lines of a file");
    hal->print("  tail [-n N] <f>   Print last N lines of a file");
    hal->print("  wc [-l|-w|-c] <f> Count lines, words, chars");
    hal->print("  write <f> <data>  Write data to a file");
    hal->print("  touch <file>      Create an empty file");
    hal->print("  edit <file>       Open file in text editor");
    hal->print("  run <file>        Execute script (.py, .js, .sh)");
    hal->print("  fetch <url>       Fetch content via HTTP/HTTPS");
    hal->print("  curl <url>        Alias for fetch");
    hal->print("  export <v>=<val>  Set an environment variable");
    hal->print("  env               List environment variables");
    hal->print("  unset <var>       Remove an environment variable");
    hal->print("  alias <n>=<cmd>   Create a command alias");
    hal->print("  unalias <name>    Remove a command alias");
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

// --- Environment & Aliases ---
static const int MAX_ENV = 32;
static char g_env_names[MAX_ENV][64];
static char g_env_vals[MAX_ENV][256];
static int g_env_count = 0;

static const int MAX_ALIAS = 32;
static char g_alias_names[MAX_ALIAS][64];
static char g_alias_cmds[MAX_ALIAS][256];
static int g_alias_count = 0;

static char g_user[64] = "anonymous";
char g_home[256] = "/";

static const char* find_env(const char* name)
{
    if (str::eq(name, "USER")) return g_user;
    if (str::eq(name, "HOME")) return g_home;
    if (str::eq(name, "PWD")) return cwd;
    for (int i = 0; i < g_env_count; i++)
    {
        if (str::eq(g_env_names[i], name)) return g_env_vals[i];
    }
    return nullptr;
}

static void set_env(const char* name, const char* val)
{
    if (str::eq(name, "USER")) { str::copy(g_user, val, 64); return; }
    if (str::eq(name, "HOME")) { str::copy(g_home, val, 256); return; }
    if (str::eq(name, "PWD")) { str::copy(cwd, val, 256); refresh_prompt(); return; }
    for (int i = 0; i < g_env_count; i++)
    {
        if (str::eq(g_env_names[i], name))
        {
            str::copy(g_env_vals[i], val, 256);
            return;
        }
    }
    if (g_env_count < MAX_ENV)
    {
        str::copy(g_env_names[g_env_count], name, 64);
        str::copy(g_env_vals[g_env_count], val, 256);
        g_env_count++;
    }
    else
    {
        hal->print("export: environment variable limit reached (32 max)");
    }
}

static void unset_env(const char* name)
{
    if (str::eq(name, "USER") || str::eq(name, "HOME") || str::eq(name, "PWD"))
    {
        hal->print("unset: cannot unset read-only or session built-in variable");
        return;
    }
    for (int i = 0; i < g_env_count; i++)
    {
        if (str::eq(g_env_names[i], name))
        {
            for (int j = i; j < g_env_count - 1; j++)
            {
                str::copy(g_env_names[j], g_env_names[j + 1], 64);
                str::copy(g_env_vals[j], g_env_vals[j + 1], 256);
            }
            g_env_count--;
            return;
        }
    }
}

static void cmd_env()
{
    char buf[350];
    int pos = str::copy(buf, "USER=", 350);
    str::append(buf, pos, g_user, 350);
    hal->print(buf);

    pos = str::copy(buf, "HOME=", 350);
    str::append(buf, pos, g_home, 350);
    hal->print(buf);

    pos = str::copy(buf, "PWD=", 350);
    str::append(buf, pos, cwd, 350);
    hal->print(buf);

    for (int i = 0; i < g_env_count; i++)
    {
        pos = str::copy(buf, g_env_names[i], 350);
        pos = str::append(buf, pos, "=", 350);
        str::append(buf, pos, g_env_vals[i], 350);
        hal->print(buf);
    }
}

static const char* find_alias(const char* name)
{
    for (int i = 0; i < g_alias_count; i++)
    {
        if (str::eq(g_alias_names[i], name)) return g_alias_cmds[i];
    }
    return nullptr;
}

static void set_alias(const char* name, const char* cmd)
{
    for (int i = 0; i < g_alias_count; i++)
    {
        if (str::eq(g_alias_names[i], name))
        {
            str::copy(g_alias_cmds[i], cmd, 256);
            return;
        }
    }
    if (g_alias_count < MAX_ALIAS)
    {
        str::copy(g_alias_names[g_alias_count], name, 64);
        str::copy(g_alias_cmds[g_alias_count], cmd, 256);
        g_alias_count++;
    }
    else
    {
        hal->print("alias: limit reached (32 max)");
    }
}

static void unalias_cmd(const char* name)
{
    for (int i = 0; i < g_alias_count; i++)
    {
        if (str::eq(g_alias_names[i], name))
        {
            for (int j = i; j < g_alias_count - 1; j++)
            {
                str::copy(g_alias_names[j], g_alias_names[j + 1], 64);
                str::copy(g_alias_cmds[j], g_alias_cmds[j + 1], 256);
            }
            g_alias_count--;
            return;
        }
    }
    char buf[128];
    int pos = str::copy(buf, "unalias: no such alias: ", 128);
    str::append(buf, pos, name, 128);
    hal->print(buf);
}

static void cmd_alias_list()
{
    if (g_alias_count == 0)
    {
        hal->print("No aliases defined.");
        return;
    }
    for (int i = 0; i < g_alias_count; i++)
    {
        char buf[350];
        int pos = str::copy(buf, "alias ", 350);
        pos = str::append(buf, pos, g_alias_names[i], 350);
        pos = str::append(buf, pos, "=\"", 350);
        pos = str::append(buf, pos, g_alias_cmds[i], 350);
        pos = str::append(buf, pos, "\"", 350);
        hal->print(buf);
    }
}

static int g_local_filter = 0;
static int g_filter_lines = 10;
static int g_filter_wc_mode = 0;
static char g_filter_arg[128];

static int int_to_str(char* buf, int n)
{
    if (n == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    char tmp[32];
    int tpos = 0;
    while (n > 0 && tpos < 31)
    {
        tmp[tpos++] = (char)('0' + (n % 10));
        n /= 10;
    }
    for (int i = 0; i < tpos; i++)
    {
        buf[i] = tmp[tpos - 1 - i];
    }
    buf[tpos] = '\0';
    return tpos;
}

static void apply_text_filter(const char* input, char* output, int max_len)
{
    if (g_local_filter == 1) // head
    {
        if (g_filter_lines <= 0) { output[0] = '\0'; return; }
        int lines_seen = 0;
        int i = 0;
        int o = 0;
        while (input[i] && o < max_len - 1)
        {
            output[o++] = input[i];
            if (input[i] == '\n')
            {
                lines_seen++;
                if (lines_seen >= g_filter_lines)
                    break;
            }
            i++;
        }
        output[o] = '\0';
    }
    else if (g_local_filter == 2) // tail
    {
        if (g_filter_lines <= 0) { output[0] = '\0'; return; }
        int total_lines = 0;
        int len = 0;
        while (input[len])
        {
            if (input[len] == '\n')
                total_lines++;
            len++;
        }
        if (len > 0 && input[len - 1] != '\n')
            total_lines++;
            
        if (total_lines <= g_filter_lines)
        {
            str::copy(output, input, max_len);
        }
        else
        {
            int lines_to_skip = total_lines - g_filter_lines;
            int skipped = 0;
            int i = 0;
            while (input[i] && skipped < lines_to_skip)
            {
                if (input[i] == '\n')
                    skipped++;
                i++;
            }
            int o = 0;
            while (input[i] && o < max_len - 1)
            {
                output[o++] = input[i++];
            }
            output[o] = '\0';
        }
    }
    else if (g_local_filter == 3) // wc
    {
        int lines = 0;
        int words = 0;
        int chars = 0;
        bool in_word = false;
        int i = 0;
        while (input[i])
        {
            chars++;
            char c = input[i];
            if (c == '\n')
                lines++;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            {
                in_word = false;
            }
            else if (!in_word)
            {
                in_word = true;
                words++;
            }
            i++;
        }
        int o = 0;
        char nbuf[32];
        if (g_filter_wc_mode == 1 || g_filter_wc_mode == 0)
        {
            o = str::append(output, o, "  ", max_len);
            int_to_str(nbuf, lines);
            o = str::append(output, o, nbuf, max_len);
        }
        if (g_filter_wc_mode == 2 || g_filter_wc_mode == 0)
        {
            o = str::append(output, o, "  ", max_len);
            int_to_str(nbuf, words);
            o = str::append(output, o, nbuf, max_len);
        }
        if (g_filter_wc_mode == 3 || g_filter_wc_mode == 0)
        {
            o = str::append(output, o, "  ", max_len);
            int_to_str(nbuf, chars);
            o = str::append(output, o, nbuf, max_len);
        }
        if (g_filter_arg[0])
        {
            o = str::append(output, o, " ", max_len);
            o = str::append(output, o, g_filter_arg, max_len);
        }
        output[o] = '\0';
    }
    else if (g_local_filter == 4) // grep
    {
        int i = 0;
        int o = 0;
        char line[512];
        int lpos = 0;
        while (input[i] && o < max_len - 1)
        {
            char c = input[i++];
            if (c != '\n' && lpos < 511)
            {
                line[lpos++] = c;
            }
            else
            {
                line[lpos] = '\0';
                bool match = false;
                int flen = str::len(g_filter_arg);
                if (flen <= lpos)
                {
                    for (int k = 0; k <= lpos - flen; k++)
                    {
                        bool ok = true;
                        for (int m = 0; m < flen; m++)
                        {
                            if (line[k + m] != g_filter_arg[m]) { ok = false; break; }
                        }
                        if (ok) { match = true; break; }
                    }
                }
                if (match)
                {
                    o = str::append(output, o, line, max_len);
                    if (o < max_len - 1) output[o++] = '\n';
                    output[o] = '\0';
                }
                lpos = 0;
            }
        }
        if (lpos > 0 && o < max_len - 1)
        {
            line[lpos] = '\0';
            bool match = false;
            int flen = str::len(g_filter_arg);
            if (flen <= lpos)
            {
                for (int k = 0; k <= lpos - flen; k++)
                {
                    bool ok = true;
                    for (int m = 0; m < flen; m++)
                    {
                        if (line[k + m] != g_filter_arg[m]) { ok = false; break; }
                    }
                    if (ok) { match = true; break; }
                }
            }
            if (match)
            {
                o = str::append(output, o, line, max_len);
            }
        }
        output[o] = '\0';
    }
    else
    {
        str::copy(output, input, max_len);
    }
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

static void expand_variables(const char* input, char* output, int max_len)
{
    int i = 0;
    int out_pos = 0;
    while (input[i] && out_pos < max_len - 1)
    {
        if (input[i] == '$')
        {
            int j = i + 1;
            char var_name[64];
            int var_pos = 0;
            while (input[j] && var_pos < 63 &&
                  ((input[j] >= 'A' && input[j] <= 'Z') ||
                   (input[j] >= 'a' && input[j] <= 'z') ||
                   (input[j] >= '0' && input[j] <= '9') ||
                   input[j] == '_'))
            {
                var_name[var_pos++] = input[j++];
            }
            var_name[var_pos] = '\0';
            if (var_pos > 0)
            {
                const char* val = find_env(var_name);
                if (val)
                {
                    for (int k = 0; val[k] && out_pos < max_len - 1; k++)
                    {
                        output[out_pos++] = val[k];
                    }
                }
                i = j;
                continue;
            }
        }
        output[out_pos++] = input[i++];
    }
    output[out_pos] = '\0';
}

static void execute_command(char* input)
{
    g_pipe_target[0] = '\0';
    g_redir_target[0] = '\0';
    g_redir_append = false;

    char expanded_buf[512];
    expand_variables(input, expanded_buf, 512);
    str::copy(input, expanded_buf, 512);

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
        const char* pcmd = g_pipe_target;
        if (str::starts_with(pcmd, "head ") || str::eq(pcmd, "head")) {
            g_local_filter = 1;
            g_filter_lines = 10;
            const char* args = (pcmd[4] == ' ') ? str::trim(pcmd + 5) : "";
            if (str::starts_with(args, "-n ")) {
                g_filter_lines = 0;
                const char* nptr = str::trim(args + 3);
                while (*nptr >= '0' && *nptr <= '9') {
                    g_filter_lines = g_filter_lines * 10 + (*nptr - '0');
                    nptr++;
                }
                if (g_filter_lines <= 0) g_filter_lines = 10;
            }
            g_pipe_target[0] = '\0';
        }
        else if (str::starts_with(pcmd, "tail ") || str::eq(pcmd, "tail")) {
            g_local_filter = 2;
            g_filter_lines = 10;
            const char* args = (pcmd[4] == ' ') ? str::trim(pcmd + 5) : "";
            if (str::starts_with(args, "-n ")) {
                g_filter_lines = 0;
                const char* nptr = str::trim(args + 3);
                while (*nptr >= '0' && *nptr <= '9') {
                    g_filter_lines = g_filter_lines * 10 + (*nptr - '0');
                    nptr++;
                }
                if (g_filter_lines <= 0) g_filter_lines = 10;
            }
            g_pipe_target[0] = '\0';
        }
        else if (str::starts_with(pcmd, "wc ") || str::eq(pcmd, "wc")) {
            g_local_filter = 3;
            g_filter_wc_mode = 0;
            g_filter_arg[0] = '\0';
            const char* args = (pcmd[2] == ' ') ? str::trim(pcmd + 3) : "";
            if (str::eq(args, "-l") || str::starts_with(args, "-l ")) g_filter_wc_mode = 1;
            else if (str::eq(args, "-w") || str::starts_with(args, "-w ")) g_filter_wc_mode = 2;
            else if (str::eq(args, "-c") || str::starts_with(args, "-c ")) g_filter_wc_mode = 3;
            g_pipe_target[0] = '\0';
        }
        else if (str::starts_with(pcmd, "grep ")) {
            g_local_filter = 4;
            str::copy(g_filter_arg, str::trim(pcmd + 5), 128);
            g_pipe_target[0] = '\0';
        }
    }

    // Right-trim input to prevent trailing spaces from breaking command parsing
    int ilen = 0;
    while (input[ilen]) ilen++;
    while (ilen > 0 && (input[ilen-1] == ' ' || input[ilen-1] == '\t')) {
        ilen--;
        input[ilen] = '\0';
    }

    const char* trimmed = str::trim(input);
    if (trimmed[0] == '\0')
        return;

    // Alias expansion loop (up to 3 passes for chained aliases, avoiding recursion)
    for (int pass = 0; pass < 3; pass++)
    {
        char first_word[64];
        int fw_len = 0;
        int idx = 0;
        while (trimmed[idx] && trimmed[idx] != ' ' && trimmed[idx] != '\t' && fw_len < 63)
        {
            first_word[fw_len++] = trimmed[idx++];
        }
        first_word[fw_len] = '\0';

        const char* alias_target = find_alias(first_word);
        if (!alias_target)
            break;

        // Check if alias replacement starts with the exact same first_word to prevent recursion
        char target_fw[64];
        int tfw_len = 0;
        int tidx = 0;
        while (alias_target[tidx] && alias_target[tidx] != ' ' && alias_target[tidx] != '\t' && tfw_len < 63)
        {
            target_fw[tfw_len++] = alias_target[tidx++];
        }
        target_fw[tfw_len] = '\0';

        char new_cmd[512];
        int pos = str::copy(new_cmd, alias_target, 512);
        while (trimmed[idx] == ' ' || trimmed[idx] == '\t') idx++;
        if (trimmed[idx])
        {
            if (pos < 511) new_cmd[pos++] = ' ';
            str::append(new_cmd, pos, trimmed + idx, 512);
        }
        str::copy(input, new_cmd, 512);
        trimmed = str::trim(input);

        if (str::eq(first_word, target_fw))
            break;
    }

    if (str::eq(trimmed, "env") || str::eq(trimmed, "export"))
    {
        cmd_env();
        return;
    }

    if (str::starts_with(trimmed, "export ") || str::starts_with(trimmed, "setenv "))
    {
        int offset = str::starts_with(trimmed, "export ") ? 7 : 7;
        const char* arg = str::trim(trimmed + offset);
        if (arg[0] == '\0')
        {
            cmd_env();
            return;
        }
        int eq_idx = -1;
        for (int i = 0; arg[i]; i++)
        {
            if (arg[i] == '=') { eq_idx = i; break; }
        }
        if (eq_idx > 0 && eq_idx < 63)
        {
            char name_buf[64];
            for (int i = 0; i < eq_idx; i++) name_buf[i] = arg[i];
            name_buf[eq_idx] = '\0';

            const char* val_ptr = arg + eq_idx + 1;
            char val_buf[256];
            int vlen = 0;
            while (val_ptr[vlen]) vlen++;
            if (vlen >= 2 && ((val_ptr[0] == '"' && val_ptr[vlen-1] == '"') || (val_ptr[0] == '\'' && val_ptr[vlen-1] == '\'')))
            {
                int c = 0;
                for (int i = 1; i < vlen - 1 && c < 255; i++) val_buf[c++] = val_ptr[i];
                val_buf[c] = '\0';
            }
            else
            {
                str::copy(val_buf, val_ptr, 256);
            }
            set_env(name_buf, val_buf);
        }
        else if (eq_idx == -1)
        {
            if (!find_env(arg)) set_env(arg, "");
        }
        return;
    }

    if (str::starts_with(trimmed, "unset "))
    {
        const char* var_name = str::trim(trimmed + 6);
        if (var_name[0]) unset_env(var_name);
        return;
    }

    if (str::eq(trimmed, "alias"))
    {
        cmd_alias_list();
        return;
    }

    if (str::starts_with(trimmed, "alias "))
    {
        const char* arg = str::trim(trimmed + 6);
        if (arg[0] == '\0')
        {
            cmd_alias_list();
            return;
        }
        int eq_idx = -1;
        for (int i = 0; arg[i]; i++)
        {
            if (arg[i] == '=') { eq_idx = i; break; }
        }
        if (eq_idx > 0 && eq_idx < 63)
        {
            char name_buf[64];
            for (int i = 0; i < eq_idx; i++) name_buf[i] = arg[i];
            name_buf[eq_idx] = '\0';

            const char* val_ptr = arg + eq_idx + 1;
            char val_buf[256];
            int vlen = 0;
            while (val_ptr[vlen]) vlen++;
            if (vlen >= 2 && ((val_ptr[0] == '"' && val_ptr[vlen-1] == '"') || (val_ptr[0] == '\'' && val_ptr[vlen-1] == '\'')))
            {
                int c = 0;
                for (int i = 1; i < vlen - 1 && c < 255; i++) val_buf[c++] = val_ptr[i];
                val_buf[c] = '\0';
            }
            else
            {
                str::copy(val_buf, val_ptr, 256);
            }
            set_alias(name_buf, val_buf);
        }
        else
        {
            const char* cmd = find_alias(arg);
            if (cmd)
            {
                char buf[350];
                int pos = str::copy(buf, "alias ", 350);
                pos = str::append(buf, pos, arg, 350);
                pos = str::append(buf, pos, "=\"", 350);
                pos = str::append(buf, pos, cmd, 350);
                pos = str::append(buf, pos, "\"", 350);
                hal->print(buf);
            }
            else
            {
                char buf[128];
                int pos = str::copy(buf, "alias: no such alias: ", 128);
                str::append(buf, pos, arg, 128);
                hal->print(buf);
            }
        }
        return;
    }

    if (str::starts_with(trimmed, "unalias "))
    {
        const char* name = str::trim(trimmed + 8);
        if (name[0]) unalias_cmd(name);
        return;
    }

    // Check if directly setting a variable like VAR=VALUE without export
    {
        int eq_idx = -1;
        bool is_var_assign = true;
        for (int i = 0; trimmed[i]; i++)
        {
            if (trimmed[i] == ' ' || trimmed[i] == '\t')
            {
                if (eq_idx == -1) is_var_assign = false;
                break;
            }
            if (trimmed[i] == '=' && eq_idx == -1)
            {
                eq_idx = i;
            }
        }
        if (is_var_assign && eq_idx > 0 && eq_idx < 63)
        {
            bool valid_name = true;
            for (int i = 0; i < eq_idx; i++)
            {
                char c = trimmed[i];
                if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
                {
                    valid_name = false;
                    break;
                }
            }
            if (valid_name)
            {
                char name_buf[64];
                for (int i = 0; i < eq_idx; i++) name_buf[i] = trimmed[i];
                name_buf[eq_idx] = '\0';

                const char* val_ptr = trimmed + eq_idx + 1;
                char val_buf[256];
                int vlen = 0;
                while (val_ptr[vlen]) vlen++;
                if (vlen >= 2 && ((val_ptr[0] == '"' && val_ptr[vlen-1] == '"') || (val_ptr[0] == '\'' && val_ptr[vlen-1] == '\'')))
                {
                    int c = 0;
                    for (int i = 1; i < vlen - 1 && c < 255; i++) val_buf[c++] = val_ptr[i];
                    val_buf[c] = '\0';
                }
                else
                {
                    str::copy(val_buf, val_ptr, 256);
                }
                set_env(name_buf, val_buf);
                return;
            }
        }
    }

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

    if (str::eq(trimmed, "cd"))
    {
        str::copy(pending_cd, g_home, 256);
        cd_pending = true;
        dispatch_cmd(json::cmd_path("stat", g_home));
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

    if (str::eq(trimmed, "ls -l") || str::eq(trimmed, "ls -la") || str::eq(trimmed, "ls -al"))
    {
        dispatch_cmd(json::cmd_path_mode("ls", cwd, "-l"));
        return;
    }

    if (str::starts_with(trimmed, "ls "))
    {
        const char* path = str::trim(trimmed + 3);
        if (str::starts_with(path, "-l ") || str::starts_with(path, "-la ") || str::starts_with(path, "-al "))
        {
            const char* target = str::trim(path + (path[2] == ' ' ? 3 : 4));
            char resolved[256];
            str::resolve_path(cwd, target, resolved, 256);
            dispatch_cmd(json::cmd_path_mode("ls", resolved, "-l"));
            return;
        }
        if (str::eq(path, "-l") || str::eq(path, "-la") || str::eq(path, "-al"))
        {
            dispatch_cmd(json::cmd_path_mode("ls", cwd, "-l"));
            return;
        }
        char resolved[256];
        str::resolve_path(cwd, path, resolved, 256);
        dispatch_cmd(json::cmd_path("ls", resolved));
        return;
    }

    if (str::starts_with(trimmed, "chmod "))
    {
        const char* args = str::trim(trimmed + 6);
        char mode_buf[64];
        int m_idx = 0;
        while (args[m_idx] && args[m_idx] != ' ' && args[m_idx] != '\t' && m_idx < 63)
        {
            mode_buf[m_idx] = args[m_idx];
            m_idx++;
        }
        mode_buf[m_idx] = '\0';
        const char* file = (args[m_idx]) ? str::trim(args + m_idx) : "";
        if (mode_buf[0] == '\0' || file[0] == '\0')
        {
            hal->print("chmod: missing mode or file operand");
            hal->print("Usage: chmod +x <file> or chmod 755 <file>");
            return;
        }
        char resolved[256];
        str::resolve_path(cwd, file, resolved, 256);
        dispatch_cmd(json::cmd_path_mode("chmod", resolved, mode_buf));
        return;
    }

    if (str::eq(trimmed, "tree"))
    {
        dispatch_cmd(json::cmd_path("tree", cwd));
        return;
    }

    if (str::starts_with(trimmed, "tree "))
    {
        const char* path = str::trim(trimmed + 5);
        char resolved[256];
        str::resolve_path(cwd, path, resolved, 256);
        dispatch_cmd(json::cmd_path("tree", resolved));
        return;
    }

    if (str::eq(trimmed, "find"))
    {
        dispatch_cmd(json::cmd_path_name("find", cwd, ""));
        return;
    }

    if (str::starts_with(trimmed, "find "))
    {
        const char* args = str::trim(trimmed + 5);
        char path_buf[256];
        char pattern_buf[256];
        path_buf[0] = '\0';
        pattern_buf[0] = '\0';

        if (str::starts_with(args, "-name "))
        {
            str::copy(path_buf, cwd, 256);
            const char* pat = str::trim(args + 6);
            if ((pat[0] == '"' || pat[0] == '\'') && pat[str::len(pat)-1] == pat[0] && str::len(pat) >= 2)
            {
                int p_len = str::len(pat) - 2;
                if (p_len >= 256) p_len = 255;
                for (int i = 0; i < p_len; i++) pattern_buf[i] = pat[i+1];
                pattern_buf[p_len] = '\0';
            }
            else
            {
                str::copy(pattern_buf, pat, 256);
            }
        }
        else
        {
            int name_pos = -1;
            int args_len = str::len(args);
            for (int i = 0; i <= args_len - 7; i++)
            {
                if (args[i] == ' ' && args[i+1] == '-' && args[i+2] == 'n' && args[i+3] == 'a' && args[i+4] == 'm' && args[i+5] == 'e' && args[i+6] == ' ')
                {
                    name_pos = i;
                    break;
                }
            }

            if (name_pos != -1)
            {
                int copy_len = name_pos;
                if (copy_len >= 256) copy_len = 255;
                for (int i = 0; i < copy_len; i++) path_buf[i] = args[i];
                path_buf[copy_len] = '\0';

                const char* pat = str::trim(args + name_pos + 7);
                if ((pat[0] == '"' || pat[0] == '\'') && pat[str::len(pat)-1] == pat[0] && str::len(pat) >= 2)
                {
                    int p_len = str::len(pat) - 2;
                    if (p_len >= 256) p_len = 255;
                    for (int i = 0; i < p_len; i++) pattern_buf[i] = pat[i+1];
                    pattern_buf[p_len] = '\0';
                }
                else
                {
                    str::copy(pattern_buf, pat, 256);
                }
            }
            else
            {
                str::copy(path_buf, args, 256);
                pattern_buf[0] = '\0';
            }
        }

        char resolved[256];
        if (path_buf[0] == '\0') str::copy(path_buf, cwd, 256);
        str::resolve_path(cwd, str::trim(path_buf), resolved, 256);
        dispatch_cmd(json::cmd_path_name("find", resolved, pattern_buf));
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

    if (str::starts_with(trimmed, "head ") || str::eq(trimmed, "head"))
    {
        g_local_filter = 1;
        g_filter_lines = 10;
        const char* args = (trimmed[4] == ' ') ? str::trim(trimmed + 5) : "";
        if (str::starts_with(args, "-n ")) {
            g_filter_lines = 0;
            const char* nptr = str::trim(args + 3);
            while (*nptr >= '0' && *nptr <= '9') {
                g_filter_lines = g_filter_lines * 10 + (*nptr - '0');
                nptr++;
            }
            if (g_filter_lines <= 0) g_filter_lines = 10;
            args = str::trim(nptr);
        }
        if (args[0] == '\0') {
            hal->print("head: missing file operand");
            g_local_filter = 0;
            return;
        }
        char resolved[256];
        str::resolve_path(cwd, args, resolved, 256);
        dispatch_cmd(json::cmd_path("cat", resolved));
        return;
    }

    if (str::starts_with(trimmed, "tail ") || str::eq(trimmed, "tail"))
    {
        g_local_filter = 2;
        g_filter_lines = 10;
        const char* args = (trimmed[4] == ' ') ? str::trim(trimmed + 5) : "";
        if (str::starts_with(args, "-n ")) {
            g_filter_lines = 0;
            const char* nptr = str::trim(args + 3);
            while (*nptr >= '0' && *nptr <= '9') {
                g_filter_lines = g_filter_lines * 10 + (*nptr - '0');
                nptr++;
            }
            if (g_filter_lines <= 0) g_filter_lines = 10;
            args = str::trim(nptr);
        }
        if (args[0] == '\0') {
            hal->print("tail: missing file operand");
            g_local_filter = 0;
            return;
        }
        char resolved[256];
        str::resolve_path(cwd, args, resolved, 256);
        dispatch_cmd(json::cmd_path("cat", resolved));
        return;
    }

    if (str::starts_with(trimmed, "wc ") || str::eq(trimmed, "wc"))
    {
        g_local_filter = 3;
        g_filter_wc_mode = 0;
        const char* args = (trimmed[2] == ' ') ? str::trim(trimmed + 3) : "";
        if (str::starts_with(args, "-l ")) {
            g_filter_wc_mode = 1;
            args = str::trim(args + 3);
        } else if (str::eq(args, "-l")) {
            g_filter_wc_mode = 1;
            args = "";
        } else if (str::starts_with(args, "-w ")) {
            g_filter_wc_mode = 2;
            args = str::trim(args + 3);
        } else if (str::eq(args, "-w")) {
            g_filter_wc_mode = 2;
            args = "";
        } else if (str::starts_with(args, "-c ")) {
            g_filter_wc_mode = 3;
            args = str::trim(args + 3);
        } else if (str::eq(args, "-c")) {
            g_filter_wc_mode = 3;
            args = "";
        }
        if (args[0] == '\0') {
            hal->print("wc: missing file operand");
            g_local_filter = 0;
            return;
        }
        str::copy(g_filter_arg, args, 128);
        char resolved[256];
        str::resolve_path(cwd, args, resolved, 256);
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

    if (str::starts_with(trimmed, "mv "))
    {
        const char* args = str::trim(trimmed + 3);
        char src[256];
        int s_idx = 0;
        while (args[s_idx] && args[s_idx] != ' ' && args[s_idx] != '\t' && s_idx < 255)
        {
            src[s_idx] = args[s_idx];
            s_idx++;
        }
        src[s_idx] = '\0';
        const char* dest = (args[s_idx]) ? str::trim(args + s_idx) : "";

        if (src[0] == '\0' || dest[0] == '\0')
        {
            hal->print("mv: missing source or destination file operand");
            return;
        }

        char resolved_src[256];
        char resolved_dest[256];
        str::resolve_path(cwd, src, resolved_src, 256);
        str::resolve_path(cwd, dest, resolved_dest, 256);
        dispatch_cmd(json::cmd_src_dest("mv", resolved_src, resolved_dest));
        return;
    }

    if (str::starts_with(trimmed, "cp "))
    {
        const char* args = str::trim(trimmed + 3);
        char src[256];
        int s_idx = 0;
        while (args[s_idx] && args[s_idx] != ' ' && args[s_idx] != '\t' && s_idx < 255)
        {
            src[s_idx] = args[s_idx];
            s_idx++;
        }
        src[s_idx] = '\0';
        const char* dest = (args[s_idx]) ? str::trim(args + s_idx) : "";

        if (src[0] == '\0' || dest[0] == '\0')
        {
            hal->print("cp: missing source or destination file operand");
            return;
        }

        char resolved_src[256];
        char resolved_dest[256];
        str::resolve_path(cwd, src, resolved_src, 256);
        str::resolve_path(cwd, dest, resolved_dest, 256);
        dispatch_cmd(json::cmd_src_dest("cp", resolved_src, resolved_dest));
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
            hal->print("\033[31mError:\033[0m You must specify a target user (e.g., share @user file.txt).");
            hal->print("To publish a package globally, use \033[36mzpm publish <file>\033[0m.");
            return;
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
                
                char buf[1024];
                int pos = str::copy(buf, "{\"cmd\":\"share\",\"path\":\"", 1024);
                pos = str::append(buf, pos, resolved, 1024);
                pos = str::append(buf, pos, "\",\"target\":\"global\"}", 1024);
                dispatch_cmd(buf);
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

    if (str::starts_with(trimmed, "fetch ") || str::starts_with(trimmed, "curl "))
    {
        int offset = str::starts_with(trimmed, "fetch ") ? 6 : 5;
        const char* url = str::trim(trimmed + offset);
        if (url[0])
        {
            dispatch_cmd(json::cmd_url("fetch", url));
        }
        else
        {
            hal->print("Usage: fetch <http(s)://url>");
        }
        return;
    }
    if (str::eq(trimmed, "fetch") || str::eq(trimmed, "curl"))
    {
        hal->print("Usage: fetch <http(s)://url>");
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

    if (str::starts_with(json_response, "__reset__"))
    {
        const char* msg = json_response + 9;
        if (str::eq(msg, "logged out"))
        {
            str::copy(g_user, "anonymous", 64);
            str::copy(g_home, "/", 256);
            str::copy(cwd, "/", 256);
        }
        else
        {
            const char* as_ptr = nullptr;
            for (int i = 0; msg[i] && msg[i + 1]; i++)
            {
                if (msg[i] == 'a' && msg[i + 1] == 's' && msg[i + 2] == ' ')
                {
                    as_ptr = msg + i + 3;
                    break;
                }
            }
            if (as_ptr)
            {
                char ubuf[64];
                int ulen = 0;
                while (as_ptr[ulen] && as_ptr[ulen] != '\033' && as_ptr[ulen] != ' ' && ulen < 63)
                {
                    ubuf[ulen] = as_ptr[ulen];
                    ulen++;
                }
                ubuf[ulen] = '\0';
                if (ulen > 0)
                {
                    str::copy(g_user, ubuf, 64);
                    int hpos = str::copy(g_home, "/users/", 256);
                    str::append(g_home, hpos, ubuf, 256);
                    str::copy(cwd, g_home, 256);
                }
                else
                {
                    str::copy(g_user, "anonymous", 64);
                    str::copy(g_home, "/", 256);
                    str::copy(cwd, "/", 256);
                }
            }
            else
            {
                str::copy(g_user, "anonymous", 64);
                str::copy(g_home, "/", 256);
                str::copy(cwd, "/", 256);
            }
        }
        refresh_prompt();
        if (msg[0])
            hal->print(msg);
        return;
    }

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

    if (g_local_filter != 0)
    {
        apply_text_filter(json_response, js_scratch_buf, 4096);
        hal->print(js_scratch_buf);
        g_local_filter = 0;
    }
    else
    {
        hal->print(json_response);
    }
}

extern "C" void kernel_main()
{
    hal = hal_get();
    hal->print(VERSION);
    hal->print("Type 'help' for available commands.");
    hal->print("");
    refresh_prompt();
}