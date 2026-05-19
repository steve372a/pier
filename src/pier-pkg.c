/*
 * pier-pkg-simple.c - Simplified version using direct file parsing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "sque.h"

#define MAX_PATH_LEN 1024
#define MAX_LINE 4096
#define MAX_NAME 256
#define MAX_PACKAGES 32

/* ==================== SHA256 Implementation ==================== */
/* SHA256 context structure */
typedef struct {
    unsigned int state[8];
    unsigned long long bitcount;
    unsigned char buffer[64];
    unsigned int bufferlen;
} SHA256_CTX;

/* SHA256 constants */
static const unsigned int K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Right rotate */
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

/* SHA256 functions */
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/* Initialize SHA256 context */
void SHA256_Init(SHA256_CTX *ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->bitcount = 0;
    ctx->bufferlen = 0;
}

/* Process a 64-byte chunk */
void SHA256_Transform(SHA256_CTX *ctx, const unsigned char *data) {
    unsigned int a, b, c, d, e, f, g, h;
    unsigned int W[64];
    int i;
    
    for (i = 0; i < 16; i++) {
        W[i] = ((unsigned int)data[i * 4] << 24) |
               ((unsigned int)data[i * 4 + 1] << 16) |
               ((unsigned int)data[i * 4 + 2] << 8) |
               ((unsigned int)data[i * 4 + 3]);
    }
    
    for (i = 16; i < 64; i++) {
        W[i] = SIG1(W[i - 2]) + W[i - 7] + SIG0(W[i - 15]) + W[i - 16];
    }
    
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    
    for (i = 0; i < 64; i++) {
        unsigned int T1 = h + EP1(e) + CH(e, f, g) + K[i] + W[i];
        unsigned int T2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }
    
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/* Update SHA256 with data */
void SHA256_Update(SHA256_CTX *ctx, const unsigned char *data, size_t len) {
    size_t i;
    
    for (i = 0; i < len; i++) {
        ctx->buffer[ctx->bufferlen++] = data[i];
        ctx->bitcount += 8;
        
        if (ctx->bufferlen == 64) {
            SHA256_Transform(ctx, ctx->buffer);
            ctx->bufferlen = 0;
        }
    }
}

/* Finalize SHA256 */
void SHA256_Final(SHA256_CTX *ctx, unsigned char *hash) {
    unsigned long long bitcount;
    int i;
    
    ctx->buffer[ctx->bufferlen++] = 0x80;
    
    if (ctx->bufferlen > 56) {
        while (ctx->bufferlen < 64) {
            ctx->buffer[ctx->bufferlen++] = 0;
        }
        SHA256_Transform(ctx, ctx->buffer);
        ctx->bufferlen = 0;
    }
    
    while (ctx->bufferlen < 56) {
        ctx->buffer[ctx->bufferlen++] = 0;
    }
    
    bitcount = ctx->bitcount;
    for (i = 7; i >= 0; i--) {
        ctx->buffer[56 + i] = (unsigned char)(bitcount & 0xFF);
        bitcount >>= 8;
    }
    
    SHA256_Transform(ctx, ctx->buffer);
    
    for (i = 0; i < 8; i++) {
        hash[i * 4] = (unsigned char)(ctx->state[i] >> 24);
        hash[i * 4 + 1] = (unsigned char)(ctx->state[i] >> 16);
        hash[i * 4 + 2] = (unsigned char)(ctx->state[i] >> 8);
        hash[i * 4 + 3] = (unsigned char)(ctx->state[i]);
    }
}

/* Calculate SHA256 of file, returns 0 on success, -1 on error */
int sha256_file(const char *filepath, char *output) {
    FILE *fp;
    SHA256_CTX ctx;
    unsigned char buffer[8192];
    unsigned char hash[32];
    size_t bytes_read;
    int i;
    
    fp = fopen(filepath, "rb");
    if (!fp) {
        return -1;
    }
    
    SHA256_Init(&ctx);
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        SHA256_Update(&ctx, buffer, bytes_read);
    }
    
    fclose(fp);
    
    SHA256_Final(&ctx, hash);
    
    for (i = 0; i < 32; i++) {
        sprintf(output + i * 2, "%02X", hash[i]);
    }
    output[64] = '\0';
    
    return 0;
}
/* ==================== End of SHA256 Implementation ==================== */

/* Global paths */
char g_pier_root[MAX_PATH_LEN];
char g_language_dir[MAX_PATH_LEN];
char g_source_url[MAX_PATH_LEN];
char g_pies_url[MAX_PATH_LEN];
char g_sys_arch[MAX_NAME];
char g_current_lang[MAX_NAME] = "zh-CN";  /* Current language code */
int g_autoyes = 0;

/* Language strings - Install */
char g_str_loading[MAX_LINE];
char g_str_package_name[MAX_LINE];
char g_str_version[MAX_LINE];
char g_str_os_req[MAX_LINE];
char g_str_description[MAX_LINE];
char g_str_author[MAX_LINE];
char g_str_distributor[MAX_LINE];
char g_str_arch[MAX_LINE];
char g_str_confirm[MAX_LINE];
char g_str_downloading[MAX_LINE];
char g_str_installing[MAX_LINE];
char g_str_installed[MAX_LINE];
char g_str_error_not_found[MAX_LINE];

/* Language strings - Remove */
char g_str_choiceremove[MAX_LINE];
char g_str_uninstall_progress[MAX_LINE];
char g_str_uninstall_success[MAX_LINE];
char g_str_package_installed_info[MAX_LINE];
char g_str_remove_warning[MAX_LINE];
char g_str_remove_space_usage[MAX_LINE];
char g_str_remove_space_unit[MAX_LINE];
char g_str_package_not_installed[MAX_LINE];
char g_str_scut_created[MAX_LINE];
char g_str_scut_not_in_path[MAX_LINE];
char g_str_scut_removed[MAX_LINE];
char g_str_error_protected_lang[MAX_LINE];
char g_str_install_hint[MAX_LINE];

/* Language strings - Errors */
char g_str_error_package_not_found[MAX_LINE];
char g_str_error_no_packages[MAX_LINE];
char g_str_error_unknown_command[MAX_LINE];
char g_str_error_install_dir_failed[MAX_LINE];
char g_str_error_install_unzip_failed[MAX_LINE];
char g_str_error_install_empty[MAX_LINE];
char g_str_error_hash_failed[MAX_LINE];
char g_str_error_verify_failed[MAX_LINE];
char g_str_usage_general[MAX_LINE];
char g_str_usage_install[MAX_LINE];
char g_str_usage_remove[MAX_LINE];

/* Search related strings */
char g_str_search_title_1[MAX_LINE];
char g_str_search_title_2[MAX_LINE];
char g_str_search_installed[MAX_LINE];
char g_str_search_available[MAX_LINE];
char g_str_search_not_found[MAX_LINE];
char g_str_search_download_failed[MAX_LINE];

/* Info command strings */
char g_str_info_alias_label[MAX_LINE];
char g_str_error_no_package_info[MAX_LINE];

/* Package info */
typedef struct {
    char name[MAX_NAME];
    char installer_name[MAX_NAME];
    char display_name[MAX_NAME];
    char version[MAX_NAME];
    char os_req[MAX_NAME];
    char description[MAX_LINE];
    char author[MAX_NAME];
    char distributor[MAX_NAME];
    char pie_file[MAX_NAME];
    char arch[MAX_NAME];
    double size_mb;
    char alias_list[MAX_LINE];
    char notice[MAX_LINE];
    char description_en[MAX_LINE];
    int is_language;
    char sha[65];
    int metadata_downloaded;
} PackageInfo;

PackageInfo g_packages[MAX_PACKAGES];
int g_package_count = 0;

/* Check if file exists */
int file_exists(const char *path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

void build_wget_proxy_opts(char *buf, int buf_size) {
    char *http_proxy, *https_proxy;
    buf[0] = '\0';
    http_proxy = getenv("http_proxy");
    if (http_proxy && http_proxy[0]) {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e use_proxy=on -e http_proxy=%s", http_proxy);
    }
    https_proxy = getenv("https_proxy");
    if (https_proxy && https_proxy[0]) {
        if (buf[0] == '\0') {
            snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e use_proxy=on");
        }
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e https_proxy=%s", https_proxy);
    }
}

/* Run process silently - redirects stdout/stderr to NUL */
int run_silent(const char *exe, const char *args) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmdline[MAX_PATH_LEN * 4];
    HANDLE hNul;
    SECURITY_ATTRIBUTES sa;
    int result;
    
    result = -1;
    
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    
    hNul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    if (hNul == INVALID_HANDLE_VALUE) return -1;
    
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hNul;
    si.hStdError = hNul;
    
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe, args);
    if (CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &result);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else if (GetLastError() == ERROR_INVALID_PARAMETER) {
        /* CREATE_NO_WINDOW requires Vista+, fallback for XP */
        if (CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            GetExitCodeProcess(pi.hProcess, &result);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
    
    CloseHandle(hNul);
    return result;
}

/* Run process visibly - stdout/stderr pass through to console */
int run_visible(const char *exe, const char *args) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmdline[MAX_PATH_LEN * 4];
    
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe, args);
    if (CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 1;
    }
    return 0;
}

/* Capture stdout output from a process into a buffer */
int capture_output(const char *exe, const char *args, char *output, int out_size) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmdline[MAX_PATH_LEN * 4];
    DWORD bytesRead;
    
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return 0;
    
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe, args);
    output[0] = '\0';
    bytesRead = 0;
    
    if (CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hWritePipe);
        ReadFile(hReadPipe, output, out_size - 1, &bytesRead, NULL);
        if (bytesRead > 0 && bytesRead < (DWORD)out_size) {
            output[bytesRead] = '\0';
            while (bytesRead > 0 && (output[bytesRead-1] == '\n' || output[bytesRead-1] == '\r')) {
                output[--bytesRead] = '\0';
            }
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else if (GetLastError() == ERROR_INVALID_PARAMETER) {
        /* CREATE_NO_WINDOW requires Vista+, fallback for XP */
        if (CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(hWritePipe);
            ReadFile(hReadPipe, output, out_size - 1, &bytesRead, NULL);
            if (bytesRead > 0 && bytesRead < (DWORD)out_size) {
                output[bytesRead] = '\0';
                while (bytesRead > 0 && (output[bytesRead-1] == '\n' || output[bytesRead-1] == '\r')) {
                    output[--bytesRead] = '\0';
                }
            }
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            CloseHandle(hWritePipe);
        }
    } else {
        CloseHandle(hWritePipe);
    }
    CloseHandle(hReadPipe);
    return (bytesRead > 0) ? 1 : 0;
}

/* Recursively remove a directory and all its contents */
void dir_remove(const char *path) {
    WIN32_FIND_DATAA fd;
    HANDLE hFind;
    char search[MAX_PATH_LEN];
    char full_path[MAX_PATH_LEN];
    
    snprintf(search, sizeof(search), "%s\\*", path);
    hFind = FindFirstFileA(search, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        RemoveDirectoryA(path);
        return;
    }
    
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;
        
        snprintf(full_path, sizeof(full_path), "%s\\%s", path, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            dir_remove(full_path);
        } else {
            SetFileAttributesA(full_path, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(full_path);
        }
    } while (FindNextFileA(hFind, &fd));
    
    FindClose(hFind);
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    RemoveDirectoryA(path);
}

/* Remove lines from file that start with a given prefix */
void remove_line_from_file(const char *filepath, const char *line_prefix) {
    char temp_file[MAX_PATH_LEN];
    FILE *fin, *fout;
    char line[MAX_LINE];
    size_t prefix_len;
    
    prefix_len = strlen(line_prefix);
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", filepath);
    
    fin = fopen(filepath, "r");
    if (!fin) return;
    fout = fopen(temp_file, "w");
    if (!fout) { fclose(fin); return; }
    
    while (fgets(line, sizeof(line), fin)) {
        if (strncmp(line, line_prefix, prefix_len) != 0) {
            fputs(line, fout);
        }
    }
    fclose(fin);
    fclose(fout);
    
    remove(filepath);
    rename(temp_file, filepath);
}

/* Convert language code to suffix (e.g., "zh-CN" -> "zhcn") */
void lang_to_suffix(const char *lang, char *suffix, int max_len) {
    int i, j = 0;
    for (i = 0; lang[i] && j < max_len - 1; i++) {
        if (lang[i] != '-') {
            suffix[j++] = tolower(lang[i]);
        }
    }
    suffix[j] = '\0';
}

/* Extract filename from URL (get last component after /) */
void extract_filename_from_url(const char *url, char *filename, int filename_size) {
    const char *last_slash = strrchr(url, '/');
    if (last_slash != NULL) {
        strncpy(filename, last_slash + 1, filename_size - 1);
        filename[filename_size - 1] = '\0';
    } else {
        strncpy(filename, url, filename_size - 1);
        filename[filename_size - 1] = '\0';
    }
}

/* Read string from lang.ini using sque library */
void read_lang_string(const char *key, char *value, int max_len) {
    char lang_file[MAX_PATH_LEN];
    
    snprintf(lang_file, sizeof(lang_file), "%s\\lang.ini", g_language_dir);
    
    if (sque_read(lang_file, key, value, max_len) < 0) {
        strncpy(value, key, max_len - 1);
        value[max_len - 1] = '\0';
    }
}

/* Console color definitions */
#define PKG_COLOR_BLACK         0
#define PKG_COLOR_DARKBLUE      1
#define PKG_COLOR_DARKGREEN     2
#define PKG_COLOR_DARKCYAN      3
#define PKG_COLOR_DARKRED       4
#define PKG_COLOR_DARKMAGENTA   5
#define PKG_COLOR_DARKYELLOW    6
#define PKG_COLOR_GRAY          7
#define PKG_COLOR_DARKGRAY      8
#define PKG_COLOR_BLUE          9
#define PKG_COLOR_GREEN         10
#define PKG_COLOR_CYAN          11
#define PKG_COLOR_RED           12
#define PKG_COLOR_MAGENTA       13
#define PKG_COLOR_YELLOW        14
#define PKG_COLOR_WHITE         15

/* Helper to call vecho.exe via CreateProcess */
void vecho_line(const char *format, ...) {
    char exe_path[MAX_PATH_LEN];
    char cmdline[MAX_PATH_LEN * 4];
    char message[MAX_LINE];
    va_list args;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    snprintf(exe_path, sizeof(exe_path), "%s\\bin\\vecho.exe", g_pier_root);
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe_path, message);
    
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    
    if (CreateProcess(exe_path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

/* Print package info with colors using vecho.exe via CreateProcess */
void print_package_info_colored(int index) {
    char notice_copy[MAX_LINE];
    char *line_ptr;
    
    printf("\n");
    
    /* Package name: brightgreen label, brightwhite value */
    vecho_line("$brightgreen$%s: $brightwhite$%s", g_str_package_name, g_packages[index].display_name);
    
    /* Version: brightgreen label, brightwhite value */
    vecho_line("$brightgreen$%s: $brightwhite$%s", g_str_version, g_packages[index].version);
    
    /* OS Requirement: brightgreen label, brightwhite value */
    vecho_line("$brightgreen$%s: $brightwhite$Windows %s", g_str_os_req, g_packages[index].os_req);
    
    /* Description: brightyellow label, brightwhite value */
    {
        const char *desc_to_show;
        if (strstr(g_language_dir, "zh-CN") != NULL) {
            desc_to_show = g_packages[index].description;
        } else {
            desc_to_show = (strlen(g_packages[index].description_en) > 0) ? 
                          g_packages[index].description_en : g_packages[index].description;
        }
        vecho_line("$brightyellow$%s: $brightwhite$%s", g_str_description, desc_to_show);
    }
    
    /* Author: brightyellow label, brightwhite value */
    vecho_line("$brightyellow$%s: $brightwhite$%s", g_str_author, g_packages[index].author);
    
    /* Distributor: brightyellow label, brightwhite value */
    vecho_line("$brightyellow$%s: $brightwhite$%s", g_str_distributor, g_packages[index].distributor);
    
    /* Architecture: brightgreen label, brightwhite value */
    vecho_line("$brightgreen$%s: $brightwhite$%s", g_str_arch, g_packages[index].arch);
    
    printf("\n");
    
    /* Notice: white label and value (only if not empty) */
    if (strlen(g_packages[index].notice) > 0) {
        strncpy(notice_copy, g_packages[index].notice, sizeof(notice_copy) - 1);
        notice_copy[sizeof(notice_copy) - 1] = '\0';
        
        line_ptr = strtok(notice_copy, "\n");
        while (line_ptr) {
            vecho_line("$white$%s", line_ptr);
            line_ptr = strtok(NULL, "\n");
        }
        printf("\n");
    }
}

/* Load all language strings */
void load_language(void) {
    /* Install strings */
    read_lang_string("loading_metadata", g_str_loading, sizeof(g_str_loading));
    read_lang_string("list_package_name", g_str_package_name, sizeof(g_str_package_name));
    read_lang_string("list_version", g_str_version, sizeof(g_str_version));
    read_lang_string("list_os_requirement", g_str_os_req, sizeof(g_str_os_req));
    read_lang_string("list_description", g_str_description, sizeof(g_str_description));
    read_lang_string("list_author", g_str_author, sizeof(g_str_author));
    read_lang_string("list_distributor", g_str_distributor, sizeof(g_str_distributor));
    read_lang_string("list_architecture", g_str_arch, sizeof(g_str_arch));
    read_lang_string("choiceapp", g_str_confirm, sizeof(g_str_confirm));
    read_lang_string("download_progress", g_str_downloading, sizeof(g_str_downloading));
    read_lang_string("install_progress", g_str_installing, sizeof(g_str_installing));
    read_lang_string("package_installed", g_str_installed, sizeof(g_str_installed));
    read_lang_string("error_package_not_exist", g_str_error_not_found, sizeof(g_str_error_not_found));
    
    /* Remove strings */
    read_lang_string("choiceremove", g_str_choiceremove, sizeof(g_str_choiceremove));
    read_lang_string("uninstall_progress", g_str_uninstall_progress, sizeof(g_str_uninstall_progress));
    read_lang_string("uninstall_success", g_str_uninstall_success, sizeof(g_str_uninstall_success));
    read_lang_string("package_installed", g_str_package_installed_info, sizeof(g_str_package_installed_info));
    read_lang_string("remove_warning_text", g_str_remove_warning, sizeof(g_str_remove_warning));
    read_lang_string("remove_space_usage", g_str_remove_space_usage, sizeof(g_str_remove_space_usage));
    read_lang_string("remove_space_usage_unit", g_str_remove_space_unit, sizeof(g_str_remove_space_unit));
    read_lang_string("package_not_installed", g_str_package_not_installed, sizeof(g_str_package_not_installed));
    read_lang_string("error_protected_lang", g_str_error_protected_lang, sizeof(g_str_error_protected_lang));
    read_lang_string("install_hint", g_str_install_hint, sizeof(g_str_install_hint));
    
    /* Scuts shortcut strings */
    read_lang_string("scut_created", g_str_scut_created, sizeof(g_str_scut_created));
    read_lang_string("scut_not_in_path", g_str_scut_not_in_path, sizeof(g_str_scut_not_in_path));
    read_lang_string("scut_removed", g_str_scut_removed, sizeof(g_str_scut_removed));
    
    /* Error strings */
    read_lang_string("error_package_not_found", g_str_error_package_not_found, sizeof(g_str_error_package_not_found));
    read_lang_string("error_no_packages", g_str_error_no_packages, sizeof(g_str_error_no_packages));
    read_lang_string("error_unknown_command", g_str_error_unknown_command, sizeof(g_str_error_unknown_command));
    read_lang_string("usage_general", g_str_usage_general, sizeof(g_str_usage_general));
    read_lang_string("usage_install", g_str_usage_install, sizeof(g_str_usage_install));
    read_lang_string("usage_remove", g_str_usage_remove, sizeof(g_str_usage_remove));
    
    /* Install error strings */
    read_lang_string("error_install_dir_failed", g_str_error_install_dir_failed, sizeof(g_str_error_install_dir_failed));
    read_lang_string("error_install_unzip_failed", g_str_error_install_unzip_failed, sizeof(g_str_error_install_unzip_failed));
    read_lang_string("error_install_empty", g_str_error_install_empty, sizeof(g_str_error_install_empty));
    read_lang_string("error_hash_failed", g_str_error_hash_failed, sizeof(g_str_error_hash_failed));
    read_lang_string("error_verify_failed", g_str_error_verify_failed, sizeof(g_str_error_verify_failed));
    
    /* Search strings */
    read_lang_string("search_results_title_1", g_str_search_title_1, sizeof(g_str_search_title_1));
    read_lang_string("search_results_title_2", g_str_search_title_2, sizeof(g_str_search_title_2));
    read_lang_string("search_installed", g_str_search_installed, sizeof(g_str_search_installed));
    read_lang_string("search_available", g_str_search_available, sizeof(g_str_search_available));
    read_lang_string("search_not_found", g_str_search_not_found, sizeof(g_str_search_not_found));
    read_lang_string("search_download_failed", g_str_search_download_failed, sizeof(g_str_search_download_failed));

    /* Info strings */
    read_lang_string("alias_display", g_str_info_alias_label, sizeof(g_str_info_alias_label));
    read_lang_string("error_no_package_info", g_str_error_no_package_info, sizeof(g_str_error_no_package_info));
}

void get_architecture(int index) {
    char metadata_file[MAX_PATH_LEN];
    
    snprintf(metadata_file, sizeof(metadata_file), "%s\\share\\cache\\metadata.sque", g_pier_root);
    
    if (!file_exists(metadata_file)) {
        strcpy(g_packages[index].arch, "all");
        return;
    }
    
    if (sque_read(metadata_file, "arch", g_packages[index].arch, sizeof(g_packages[index].arch)) < 0) {
        strcpy(g_packages[index].arch, "all");
    }
    if (strlen(g_packages[index].arch) == 0) {
        strcpy(g_packages[index].arch, "all");
    }
}

void get_pie_file(int index) {
    char metadata_file[MAX_PATH_LEN];
    char filename[MAX_NAME] = {0};
    char url_content[MAX_LINE];
    
    snprintf(metadata_file, sizeof(metadata_file), "%s\\share\\cache\\metadata.sque", g_pier_root);
    
    if (!file_exists(metadata_file)) {
        snprintf(g_packages[index].pie_file, sizeof(g_packages[index].pie_file), "%s/%s/%s-%s.pie",
                 g_pies_url, g_packages[index].name, g_packages[index].name, g_packages[index].version);
        goto replace_version;
    }
    
    /* Try [pkgfile] first, then [URL] with arch resolution */
    sque_read(metadata_file, "pkgfile", filename, sizeof(filename));
    
    if (strlen(filename) == 0) {
        if (sque_read(metadata_file, "URL", url_content, sizeof(url_content)) >= 0) {
            char *line = strtok(url_content, "\n");
            char fallback_entry[MAX_NAME] = {0};
            
            while (line) {
                char *colon = strchr(line, ':');
                if (colon) {
                    char arch_tag[16];
                    int tag_len = (int)(colon - line);
                    char *value;
                    
                    if (tag_len > 15) tag_len = 15;
                    memcpy(arch_tag, line, tag_len);
                    arch_tag[tag_len] = '\0';
                    
                    /* Trim arch_tag */
                    {
                        char *s = arch_tag, *d = arch_tag;
                        while (*s == ' ' || *s == '\t') s++;
                        while (*s) *d++ = *s++;
                        *d = '\0';
                        while (d > arch_tag && (*(d-1) == ' ' || *(d-1) == '\t')) *--d = '\0';
                    }
                    
                    value = colon + 1;
                    while (*value == ' ' || *value == '\t') value++;
                    
                    /* Remove "(default)" suffix and trailing whitespace */
                    {
                        char *def = strstr(value, "(default)");
                        if (def) {
                            *def = '\0';
                            while (def > value && (*(def-1) == ' ' || *(def-1) == '\t')) *--def = '\0';
                        } else {
                            char *end = value + strlen(value);
                            while (end > value && (*(end-1) == ' ' || *(end-1) == '\t')) *--end = '\0';
                        }
                    }
                    
                    if (fallback_entry[0] == '\0') {
                        strncpy(fallback_entry, value, sizeof(fallback_entry) - 1);
                        fallback_entry[sizeof(fallback_entry) - 1] = '\0';
                    }
                    
                    if (_stricmp(arch_tag, g_sys_arch) == 0) {
                        strncpy(filename, value, sizeof(filename) - 1);
                        filename[sizeof(filename) - 1] = '\0';
                        break;
                    }
                } else if (strlen(line) > 0 && strcmp(line, "::end") != 0) {
                    strncpy(filename, line, sizeof(filename) - 1);
                    filename[sizeof(filename) - 1] = '\0';
                    break;
                }
                line = strtok(NULL, "\n");
            }
            
            if (strlen(filename) == 0 && fallback_entry[0] != '\0') {
                strncpy(filename, fallback_entry, sizeof(filename) - 1);
                filename[sizeof(filename) - 1] = '\0';
            }
        }
    }
    
    if (strlen(filename) == 0) {
        snprintf(filename, sizeof(filename), "%s.pie", g_packages[index].name);
    }
    
    snprintf(g_packages[index].pie_file, sizeof(g_packages[index].pie_file),
             "%s/%s/%s",
             g_pies_url,
             g_packages[index].name,
             filename);
    
replace_version:
    /* Replace {version} placeholder with actual version */
    {
        char *ver_pos;
        while ((ver_pos = strstr(g_packages[index].pie_file, "{version}")) != NULL) {
            char temp[MAX_PATH_LEN];
            int prefix_len = (int)(ver_pos - g_packages[index].pie_file);
            strncpy(temp, g_packages[index].pie_file, prefix_len);
            temp[prefix_len] = '\0';
            snprintf(temp + prefix_len, sizeof(temp) - prefix_len, "%s%s",
                     g_packages[index].version, ver_pos + 9);
            strncpy(g_packages[index].pie_file, temp, sizeof(g_packages[index].pie_file) - 1);
            g_packages[index].pie_file[sizeof(g_packages[index].pie_file) - 1] = '\0';
        }
    }
}

/* Execute command and wait with timeout (milliseconds, 0 = infinite) */
int execute_and_wait_timeout(const char *exe_path, const char *args, DWORD timeout_ms) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmd_line[MAX_PATH_LEN * 4];
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    /* Inherit parent's stdout/stderr to show progress bar */
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    
    ZeroMemory(&pi, sizeof(pi));
    
    /* Build command line - always quote exe_path */
    snprintf(cmd_line, sizeof(cmd_line), "\"%s\" %s", exe_path, args);
    
    if (!CreateProcessA(NULL, cmd_line, NULL, NULL, TRUE, 
                        0, NULL, NULL, &si, &pi)) {
        return -1;
    }
    
    /* Wait with timeout */
    DWORD wait_result = WaitForSingleObject(pi.hProcess, timeout_ms);
    
    if (wait_result == WAIT_TIMEOUT) {
        /* Timeout - terminate the process */
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return -2;  /* Timeout error */
    }
    
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (int)exit_code;
}

/* Execute command and wait (infinite timeout) */
int execute_and_wait(const char *exe_path, const char *args) {
    return execute_and_wait_timeout(exe_path, args, INFINITE);
}

/* Download metadata */
int download_metadata(const char *package_name, int index) {
    char cache_dir[MAX_PATH_LEN];
    char metadata_file[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char args[MAX_PATH_LEN * 4];
    
    printf("%s\n", g_str_loading);
    
    /* Use long path names - short paths cause issues with special characters */
    /* Create cache directory */
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
    CreateDirectoryA(cache_dir, NULL);
    
    /* Build metadata file path */
    snprintf(metadata_file, sizeof(metadata_file), "%s\\%s.metadata", cache_dir, package_name);
    
    /* Remove existing file */
    remove(metadata_file);
    
    /* OLD (pier-get): snprintf(exe_path,...,"%s\\bin\\pier-get.exe",...); snprintf(args,...,"-q \"%s\" \"%s/%c/%s/latest.metadata\"...",g_language_dir,g_source_url,package_name[0],...); */
    {
        char proxy_opts[512];
        build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
        snprintf(exe_path, sizeof(exe_path), "%s\\bin\\uma-get.exe", g_pier_root);
        snprintf(args, sizeof(args), "-q --timeout=300 --tries=3 --no-check-certificate%s -O \"%s\\%s.metadata\" \"%s/%c/%s/latest.metadata\"",
                 proxy_opts, cache_dir, package_name, g_source_url, package_name[0], package_name);
    }
    
    /* Execute with 5 minute timeout for metadata (usually small) */
    {
        int result = execute_and_wait_timeout(exe_path, args, 300000);
        if (result == -2) {
            printf("Warning: Download timeout for %s.metadata\n", package_name);
        }
        if (result != 0 && result != -2) {
            printf("Warning: Download failed (exit code %d) for %s.metadata\n",
                   result, package_name);
        }
    }
    
    /* Check if file was downloaded */
    if (!file_exists(metadata_file)) {
        printf("%s: %s\n", g_str_error_package_not_found, package_name);
        return 0;
    }
    
    /* Unzip metadata */
    {
        char sque_file[MAX_PATH_LEN];
        
        /* Remove old .sque files to avoid unzip replace prompt */
        snprintf(sque_file, sizeof(sque_file), "%s\\metadata.sque", cache_dir);
        remove(sque_file);
        snprintf(sque_file, sizeof(sque_file), "%s\\notice.sque", cache_dir);
        remove(sque_file);
        snprintf(sque_file, sizeof(sque_file), "%s\\profile.sque", cache_dir);
        remove(sque_file);
        
        /* Unzip using unzip.exe - use CreateProcess to avoid cmd.exe quote issues */
        {
            STARTUPINFO si_unzip;
            PROCESS_INFORMATION pi_unzip;
            char cmdline[MAX_PATH_LEN * 4];
            HANDLE hNull;
            
            snprintf(exe_path, sizeof(exe_path), "%s\\bin\\unzip.exe", g_pier_root);
            /* Use -o flag to overwrite without prompting, -q for quiet */
            snprintf(cmdline, sizeof(cmdline), "\"%s\" -o -q \"%s\" -d \"%s\"", exe_path, metadata_file, cache_dir);
            
            /* Open NUL device to suppress output with inheritable handle */
            {
                SECURITY_ATTRIBUTES sa;
                sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                sa.bInheritHandle = TRUE;
                sa.lpSecurityDescriptor = NULL;
                hNull = CreateFile("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
            
            memset(&si_unzip, 0, sizeof(si_unzip));
            si_unzip.cb = sizeof(si_unzip);
            if (hNull != INVALID_HANDLE_VALUE) {
                si_unzip.dwFlags = STARTF_USESTDHANDLES;
                si_unzip.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
                si_unzip.hStdOutput = hNull;
                si_unzip.hStdError = hNull;
            }
            memset(&pi_unzip, 0, sizeof(pi_unzip));
            
            if (CreateProcess(exe_path, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si_unzip, &pi_unzip)) {
                WaitForSingleObject(pi_unzip.hProcess, INFINITE);
                CloseHandle(pi_unzip.hProcess);
                CloseHandle(pi_unzip.hThread);
            }
            
            if (hNull != INVALID_HANDLE_VALUE) {
                CloseHandle(hNull);
            }
        }
        
        /* Delete temp file */
        remove(metadata_file);
        
        /* Check if metadata.sque exists */
        if (!file_exists(sque_file)) {
            printf("%s: %s\n", g_str_error_package_not_found, package_name);
            return 0;
        }
        
        /* Metadata will be saved with Chinese content after parse_metadata() */
    }
    
    g_packages[index].metadata_downloaded = 1;
    return 1;
}

/* Download all metadata files in one batch pier-get call */
static void download_metadata_batch(void) {
    char cache_dir[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    int i;
    
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
    CreateDirectoryA(cache_dir, NULL);
    
    /* OLD (pier-get): single batch call with -q -b "lang_dir" "url1" "out1" "url2" "out2" ... via execute_and_wait_timeout */
    {
        char proxy_opts[512];
        build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
        snprintf(exe_path, sizeof(exe_path), "%s\\bin\\uma-get.exe", g_pier_root);
        
        for (i = 0; i < g_package_count; i++) {
            char meta_file[MAX_PATH_LEN];
            char uma_args[MAX_PATH_LEN * 2];
            snprintf(meta_file, sizeof(meta_file), "%s\\%s.metadata", cache_dir, g_packages[i].name);
            remove(meta_file);
            
            snprintf(uma_args, sizeof(uma_args), "-q --timeout=300 --tries=3 --no-check-certificate%s -O \"%s\" \"%s/%c/%s/latest.metadata\"",
                     proxy_opts, meta_file, g_source_url, g_packages[i].name[0], g_packages[i].name);
            execute_and_wait_timeout(exe_path, uma_args, 300000);
        }
    }
    
    /* Unzip and verify each metadata file */
    for (i = 0; i < g_package_count; i++) {
        char meta_file[MAX_PATH_LEN];
        char sque_file[MAX_PATH_LEN];
        
        snprintf(meta_file, sizeof(meta_file), "%s\\%s.metadata", cache_dir, g_packages[i].name);
        snprintf(sque_file, sizeof(sque_file), "%s\\metadata.sque", cache_dir);
        remove(sque_file);
        snprintf(sque_file, sizeof(sque_file), "%s\\notice.sque", cache_dir);
        remove(sque_file);
        snprintf(sque_file, sizeof(sque_file), "%s\\profile.sque", cache_dir);
        remove(sque_file);
        
        if (!file_exists(meta_file)) {
            printf("%s: %s\n", g_str_error_package_not_found, g_packages[i].name);
            continue;
        }
        
        {
            char cmdline[MAX_PATH_LEN * 4];
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            HANDLE hNull;
            
            snprintf(cmdline, sizeof(cmdline), "\"%s\\bin\\unzip.exe\" -o \"%s\" -d \"%s\"",
                     g_pier_root, meta_file, cache_dir);
            
            hNull = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.hStdOutput = hNull;
            si.hStdError = hNull;
            
            if (CreateProcess(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } else if (GetLastError() == ERROR_INVALID_PARAMETER) {
                if (CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
            
            if (hNull != INVALID_HANDLE_VALUE) CloseHandle(hNull);
        }
        
        remove(meta_file);
        
        snprintf(sque_file, sizeof(sque_file), "%s\\metadata.sque", cache_dir);
        if (!file_exists(sque_file)) {
            printf("%s: %s\n", g_str_error_package_not_found, g_packages[i].name);
            continue;
        }
        
        {
            char installer_check[MAX_LINE];
            if (sque_read(sque_file, "InstallerName", installer_check, sizeof(installer_check)) < 0 ||
                strlen(installer_check) == 0) {
                printf("%s: %s\n", g_str_error_package_not_found, g_packages[i].name);
                continue;
            }
        }
        
        g_packages[i].metadata_downloaded = 1;
    }
}

/* Parse metadata file directly */
int parse_metadata(int index) {
    char metadata_file[MAX_PATH_LEN];
    char temp_value[MAX_LINE];
    char profile_path[MAX_PATH_LEN];
    char notice_path[MAX_PATH_LEN];
    char profile_content[MAX_LINE];
    char *line_ptr;
    
    snprintf(metadata_file, sizeof(metadata_file), "%s\\share\\cache\\metadata.sque", g_pier_root);
    snprintf(profile_path, sizeof(profile_path), "%s\\share\\cache\\profile.sque", g_pier_root);
    snprintf(notice_path, sizeof(notice_path), "%s\\share\\cache\\notice.sque", g_pier_root);
    
    /* Read all fields from metadata.sque using sque library (auto UTF-8 to ACP) */
    if (sque_read(metadata_file, "PackageName", g_packages[index].display_name, sizeof(g_packages[index].display_name)) < 0)
        g_packages[index].display_name[0] = '\0';
    
    if (sque_read(metadata_file, "InstallerName", g_packages[index].installer_name, sizeof(g_packages[index].installer_name)) < 0)
        g_packages[index].installer_name[0] = '\0';
    
    if (sque_read(metadata_file, "Version", g_packages[index].version, sizeof(g_packages[index].version)) < 0)
        g_packages[index].version[0] = '\0';
    
    if (sque_read(metadata_file, "OS", g_packages[index].os_req, sizeof(g_packages[index].os_req)) >= 0) {
        if (_stricmp(g_packages[index].os_req, "language") == 0) {
            g_packages[index].is_language = 1;
        }
    } else {
        g_packages[index].os_req[0] = '\0';
    }
    
    if (sque_read(metadata_file, "ProFile", g_packages[index].description, sizeof(g_packages[index].description)) < 0)
        g_packages[index].description[0] = '\0';
    
    if (sque_read(metadata_file, "ProFile_En", g_packages[index].description_en, sizeof(g_packages[index].description_en)) < 0)
        g_packages[index].description_en[0] = '\0';
    
    if (sque_read(metadata_file, "PackageSize", temp_value, sizeof(temp_value)) >= 0)
        g_packages[index].size_mb = atof(temp_value);
    
    if (sque_read(metadata_file, "Author", g_packages[index].author, sizeof(g_packages[index].author)) < 0)
        g_packages[index].author[0] = '\0';
    
    if (sque_read(metadata_file, "Distributor", g_packages[index].distributor, sizeof(g_packages[index].distributor)) < 0)
        g_packages[index].distributor[0] = '\0';
    
    /* Notice: read from metadata.sque first, then override from notice.sque */
    sque_read(metadata_file, "Notice", g_packages[index].notice, sizeof(g_packages[index].notice));
    
    /* Read SHA256 hash from [HASH] section (case-insensitive via updated sque_read) */
    if (sque_read(metadata_file, "HASH", temp_value, sizeof(temp_value)) >= 0) {
        strncpy(g_packages[index].sha, temp_value, sizeof(g_packages[index].sha) - 1);
        g_packages[index].sha[sizeof(g_packages[index].sha) - 1] = '\0';
    } else {
        g_packages[index].sha[0] = '\0';
    }
    
    /* Localized package name and description from profile.sque
     * profile.sque format: [language]\nPackageName: value\nProFile: value */
    if (sque_read(profile_path, g_current_lang, profile_content, sizeof(profile_content)) >= 0) {
        line_ptr = strtok(profile_content, "\n");
        while (line_ptr) {
            if (strncmp(line_ptr, "PackageName:", 12) == 0) {
                char *val = line_ptr + 12;
                while (*val == ' ' || *val == '\t') val++;
                strncpy(g_packages[index].display_name, val, sizeof(g_packages[index].display_name) - 1);
                g_packages[index].display_name[sizeof(g_packages[index].display_name) - 1] = '\0';
            } else if (strncmp(line_ptr, "ProFile:", 8) == 0) {
                char *val = line_ptr + 8;
                while (*val == ' ' || *val == '\t') val++;
                strncpy(g_packages[index].description, val, sizeof(g_packages[index].description) - 1);
                g_packages[index].description[sizeof(g_packages[index].description) - 1] = '\0';
            }
            line_ptr = strtok(NULL, "\n");
        }
    }
    
    /* Localized notice from notice.sque (multi-line, UTF-8 auto-converted)
     * notice.sque format: [language]\ncontent\ncontent\n::end */
    if (sque_read(notice_path, g_current_lang, temp_value, sizeof(temp_value)) >= 0) {
        strncpy(g_packages[index].notice, temp_value, sizeof(g_packages[index].notice) - 1);
        g_packages[index].notice[sizeof(g_packages[index].notice) - 1] = '\0';
    } else if (strcmp(g_current_lang, "en-US") != 0) {
        if (sque_read(notice_path, "en-US", temp_value, sizeof(temp_value)) >= 0) {
            strncpy(g_packages[index].notice, temp_value, sizeof(g_packages[index].notice) - 1);
            g_packages[index].notice[sizeof(g_packages[index].notice) - 1] = '\0';
        }
    }
    
    /* Get architecture and PIE file */
    get_architecture(index);
    get_pie_file(index);
    
    /* Save resolved metadata to metadata\{package}.sque with localized content */
    {
        char src_path[MAX_PATH_LEN], dst_path[MAX_PATH_LEN];
        char line[MAX_LINE];
        FILE *fin, *fout;
        int skip_section = 0;
        
        snprintf(src_path, sizeof(src_path), "%s\\share\\cache\\metadata.sque", g_pier_root);
        snprintf(dst_path, sizeof(dst_path), "%s\\metadata", g_pier_root);
        CreateDirectoryA(dst_path, NULL);
        snprintf(dst_path, sizeof(dst_path), "%s\\metadata\\%s.sque", g_pier_root, g_packages[index].name);
        
        fin = fopen(src_path, "r");
        if (fin) {
            fout = fopen(dst_path, "w");
            if (fout) {
                while (fgets(line, sizeof(line), fin)) {
                    size_t len = strlen(line);
                    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
                    
                    if (strcmp(line, "[PackageName]") == 0) {
                        fprintf(fout, "[PackageName]\n%s\n::end\n\n", g_packages[index].display_name);
                        skip_section = 1;
                        continue;
                    }
                    if (strcmp(line, "[ProFile]") == 0) {
                        fprintf(fout, "[ProFile]\n%s\n::end\n\n", g_packages[index].description);
                        skip_section = 1;
                        continue;
                    }
                    if (strcmp(line, "[ProFile_En]") == 0) {
                        fprintf(fout, "[ProFile_En]\n%s\n::end\n\n", g_packages[index].description_en);
                        skip_section = 1;
                        continue;
                    }
                    if (strcmp(line, "[Notice]") == 0) {
                        fprintf(fout, "[Notice]\n%s\n::end\n\n", g_packages[index].notice);
                        skip_section = 1;
                        continue;
                    }
                    
                    if (skip_section) {
                        if (strcmp(line, "::end") == 0) {
                            skip_section = 0;
                            continue;
                        }
                        if (line[0] == '[') {
                            skip_section = 0;
                        } else {
                            continue;
                        }
                    }
                    
                    fprintf(fout, "%s\n", line);
                }
                fclose(fout);
            }
            fclose(fin);
        }
    }
    
    return 1;
}

/* Display package info */
void display_package_info(int index) {
    print_package_info_colored(index);
}

/* Confirm installation */
int confirm_install(void) {
    char response[10];
    
    if (g_autoyes) {
        return 1;
    }
    
    printf("%s (Y/N): ", g_str_confirm);
    fflush(stdout);
    
    if (fgets(response, sizeof(response), stdin)) {
        if (response[0] == 'Y' || response[0] == 'y') {
            return 1;
        }
    }
    
    return 0;
}

/* Forward declarations for search function */
int search_packages(const char *pier_root, const char *lang_dir, const char *source_url, const char *keyword);
int is_package_installed(const char *pier_root, const char *installer_name);
char *stristr(const char *str1, const char *str2);

/* Download package */
int download_package(int index) {
    char cache_dir[MAX_PATH_LEN];
    char pie_file[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char args[MAX_PATH_LEN * 4];
    
    printf("%s %s...\n", g_str_downloading, g_packages[index].display_name);
    fflush(stdout);
    
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
    {
        char filename[MAX_NAME];
        extract_filename_from_url(g_packages[index].pie_file, filename, sizeof(filename));
        snprintf(pie_file, sizeof(pie_file), "%s\\%s", cache_dir, filename);
    }
    remove(pie_file);
    
    /* OLD (pier-get): snprintf(exe_path,...,"%s\\bin\\pier-get.exe",...); snprintf(args,...,"\"%s\" \"%s\" \"%s\"",g_language_dir,...); */
    {
        char proxy_opts[512];
        build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
        snprintf(exe_path, sizeof(exe_path), "%s\\bin\\uma-get.exe", g_pier_root);
        snprintf(args, sizeof(args), "-q --show-progress --timeout=14400 --tries=3 --no-check-certificate%s -O \"%s\" \"%s\"",
                 proxy_opts, pie_file, g_packages[index].pie_file);
    }
    
    {
        int result = execute_and_wait_timeout(exe_path, args, 14400000);
        if (result == -2) {
            printf("Error: Download timeout (4 hours) for %s\n", g_packages[index].display_name);
            printf("The file may be too large or the connection is too slow.\n");
            return 0;
        }
        if (result != 0) {
            printf("Error: Download failed (exit code %d) for %s\n",
                   result, g_packages[index].display_name);
            remove(pie_file);
            return 0;
        }
    }
    
    /* Wait a bit for file to be written */
    Sleep(500);
    
    if (!file_exists(pie_file)) {
        printf("%s: %s\n", g_str_error_not_found, g_packages[index].pie_file);
        return 0;
    }
    
    return 1;
}

/* Check if pier root is already in user PATH (HKCU) */
int check_path_has_pier(void) {
    HKEY hKey;
    char path_buf[8192];
    DWORD path_len = sizeof(path_buf);
    LONG result;
    
    result = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) return 0;
    
    path_buf[0] = '\0';
    result = RegQueryValueExA(hKey, "PATH", NULL, NULL, (BYTE *)path_buf, &path_len);
    RegCloseKey(hKey);
    
    if (result != ERROR_SUCCESS || path_len <= 1) return 0;
    
    if (strstr(path_buf, g_pier_root)) return 1;
    return 0;
}

/* Install package */
int install_package(int index) {
    char cache_dir[MAX_PATH_LEN];
    char pie_file[MAX_PATH_LEN];
    char app_dir[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char args[MAX_PATH_LEN * 4];
    
    printf("%s %s...\n", g_str_installing, g_packages[index].display_name);
    
    Sleep(200);
    
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
    {
        char filename[MAX_NAME];
        extract_filename_from_url(g_packages[index].pie_file, filename, sizeof(filename));
        snprintf(pie_file, sizeof(pie_file), "%s\\%s", cache_dir, filename);
    }
    
    /* SHA256 verification */
    if (g_packages[index].sha[0] != '\0') {
        char computed_hash[65];
        if (sha256_file(pie_file, computed_hash) == 0) {
            if (_strnicmp(computed_hash, g_packages[index].sha, 64) != 0) {
                vecho_line("$brightred$%s %s$write$", g_str_error_hash_failed, g_packages[index].display_name);
                return 0;
            }
        }
    }
    
    if (g_packages[index].is_language) {
        if (_stricmp(g_packages[index].name, "zh-CN") == 0) {
            printf("%s\n", g_str_error_protected_lang);
            return 0;
        }
        
        /* Language packages always install to share\language\{InstallerName} */
        char lang_dir[MAX_PATH_LEN];
        snprintf(lang_dir, sizeof(lang_dir), "%s\\share\\language\\%s", g_pier_root, g_packages[index].installer_name);
        if (!CreateDirectoryA(lang_dir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            vecho_line("$brightred$%s %s$write$", g_str_error_install_dir_failed, g_packages[index].display_name);
            return 0;
        }
        
        {
            char unzip_exe[MAX_PATH_LEN];
            char unzip_args[MAX_PATH_LEN * 4];
            int unzip_ret;
            snprintf(unzip_exe, sizeof(unzip_exe), "%s\\bin\\unzip.exe", g_pier_root);
            snprintf(unzip_args, sizeof(unzip_args), "-o -q \"%s\" -d \"%s\"", pie_file, lang_dir);
            unzip_ret = run_silent(unzip_exe, unzip_args);
            if (unzip_ret > 1) {
                vecho_line("$brightred$%s %s$write$", g_str_error_install_unzip_failed, g_packages[index].display_name);
                return 0;
            }
            {
                WIN32_FIND_DATAA fd;
                HANDLE hFind;
                char search[MAX_PATH_LEN];
                int has_files;
                has_files = 0;
                snprintf(search, sizeof(search), "%s\\*", lang_dir);
                hFind = FindFirstFileA(search, &fd);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
                            has_files = 1;
                            break;
                        }
                    } while (FindNextFileA(hFind, &fd));
                    FindClose(hFind);
                }
                if (!has_files) {
                    vecho_line("$brightred$%s %s$write$", g_str_error_install_unzip_failed, g_packages[index].display_name);
                    return 0;
                }
            }
        }
        
        printf("%s: %s\n", g_str_installed, lang_dir);
        return 1;
    }
    
    /* Ensure app/ parent directory exists first */
    {
        char app_root[MAX_PATH_LEN];
        snprintf(app_root, sizeof(app_root), "%s\\app", g_pier_root);
        CreateDirectoryA(app_root, NULL);
    }
    
    snprintf(app_dir, sizeof(app_dir), "%s\\app\\%s", g_pier_root, g_packages[index].installer_name);
    if (!CreateDirectoryA(app_dir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        vecho_line("$brightred$%s %s$write$", g_str_error_install_dir_failed, g_packages[index].display_name);
        return 0;
    }
    
    {
        char unzip_exe[MAX_PATH_LEN];
        char unzip_args[MAX_PATH_LEN * 4];
        int unzip_ret;
        snprintf(unzip_exe, sizeof(unzip_exe), "%s\\bin\\unzip.exe", g_pier_root);
        snprintf(unzip_args, sizeof(unzip_args), "-o -q \"%s\" -d \"%s\"", pie_file, app_dir);
        unzip_ret = run_silent(unzip_exe, unzip_args);
        if (unzip_ret > 1) {
            vecho_line("$brightred$%s %s$write$", g_str_error_install_unzip_failed, g_packages[index].display_name);
            return 0;
        }
    }
    
    /* Verify: check if app directory has files */
    {
        WIN32_FIND_DATAA fd;
        HANDLE hFind;
        char search[MAX_PATH_LEN];
        int has_files;
        
        has_files = 0;
        snprintf(search, sizeof(search), "%s\\*", app_dir);
        hFind = FindFirstFileA(search, &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
                    has_files = 1;
                    break;
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
        
        if (!has_files) {
            vecho_line("$brightred$%s %s$write$", g_str_error_install_empty, g_packages[index].display_name);
            RemoveDirectoryA(app_dir);
            return 0;
        }
    }
    
    printf("%s: %s\n", g_str_installed, app_dir);
    
    /* Register in pierlist.sque */
    {
        char etc_dir[MAX_PATH_LEN];
        char pierlist_file[MAX_PATH_LEN];
        FILE *fp;
        
        /* Create etc directory */
        snprintf(etc_dir, sizeof(etc_dir), "%s\\etc", g_pier_root);
        CreateDirectoryA(etc_dir, NULL);
        
        /* Create pierlist.sque if not exists */
        snprintf(pierlist_file, sizeof(pierlist_file), "%s\\pierlist.sque", etc_dir);
        if (!file_exists(pierlist_file)) {
            fp = fopen(pierlist_file, "w");
            if (fp) fclose(fp);
        }
        
        /* Remove existing entry using native C */
        remove_line_from_file(pierlist_file, g_packages[index].installer_name);
        
        /* Add new entry */
        fp = fopen(pierlist_file, "a");
        if (fp) {
            /* Format: InstallerName | Version | Date | InstallerName | Source */
            time_t now;
            struct tm *tm_info;
            char date_str[20];
            
            time(&now);
            tm_info = localtime(&now);
            snprintf(date_str, sizeof(date_str), "%04d.%02d.%02d", 
                     tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
            
            fprintf(fp, "%s | %s | %s | %s | %s\n", 
                    g_packages[index].installer_name,
                    g_packages[index].version,
                    date_str,
                    g_packages[index].installer_name,
                    g_source_url);
            fclose(fp);
        }
        
        /* Metadata already saved with Chinese content by parse_metadata() */
    }
    
    /* Create scuts shortcut bat */
    {
        char scuts_dir[MAX_PATH_LEN];
        char scuts_bat[MAX_PATH_LEN];
        FILE *bat_fp;
        
        snprintf(scuts_dir, sizeof(scuts_dir), "%s\\scuts", g_pier_root);
        CreateDirectoryA(scuts_dir, NULL);
        
        snprintf(scuts_bat, sizeof(scuts_bat), "%s\\scuts\\%s.bat", g_pier_root, g_packages[index].name);
        bat_fp = fopen(scuts_bat, "w");
        if (bat_fp) {
            fprintf(bat_fp, "@echo off\r\n");
            fprintf(bat_fp, "cd /d %%~dp0..\r\n");
            fprintf(bat_fp, "pier.exe o %s %%*\r\n", g_packages[index].name);
            fclose(bat_fp);
            
            vecho_line("$brightcyan$%s%s", g_str_scut_created, g_packages[index].name);
            
            if (!check_path_has_pier()) {
                vecho_line("$white$%s", g_str_scut_not_in_path);
            }
        }
    }
    
    return 1;
}

/* Calculate folder size in bytes (internal recursive) */
LONGLONG calculate_folder_size_raw(const char *path) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    char search_path[MAX_PATH_LEN];
    LARGE_INTEGER totalSize;
    totalSize.QuadPart = 0;
    
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    hFind = FindFirstFileA(search_path, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    char subdir[MAX_PATH_LEN];
                    snprintf(subdir, sizeof(subdir), "%s\\%s", path, findData.cFileName);
                    totalSize.QuadPart += calculate_folder_size_raw(subdir);
                } else {
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    totalSize.QuadPart += fileSize.QuadPart;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    return totalSize.QuadPart;
}

/* Calculate folder size in MB */
double calculate_folder_size(const char *path) {
    return (double)calculate_folder_size_raw(path) / 1048576.0;
}

/* Print package info with colors for remove */
void print_package_info_remove_colored(int index, double folder_size) {
    char size_str[64];
    
    printf("\n");
    
    /* Package name: brightgreen label, brightwhite value - same format as install */
    vecho_line("$brightgreen$%s: $brightwhite$%s", g_str_package_name, g_packages[index].display_name);
    
    /* Version: brightgreen label, brightwhite value */
    vecho_line("$brightgreen$%s: $brightwhite$%s", g_str_version, g_packages[index].version);
    
    /* Space usage: brightred for the size */
    snprintf(size_str, sizeof(size_str), "%.1f", folder_size);
    vecho_line("$brightgreen$%s: $brightred$%s %s", g_str_remove_space_usage, size_str, g_str_remove_space_unit);
}

/* Print warning with colors for remove */
void print_remove_warning_colored(void) {
    printf("\n");
    /* Use g_str_remove_warning - print as brightyellow warning */
    vecho_line("$brightyellow$%s", g_str_remove_warning);
    printf("\n");
}

/* Remove package */
int remove_package(int index) {
    char target_dir[MAX_PATH_LEN];
    int is_installed = 0;
    double folder_size = 0.0;
    
    /* Check if installed */
    if (g_packages[index].is_language) {
        snprintf(target_dir, sizeof(target_dir), "%s\\share\\language\\%s", g_pier_root, g_packages[index].name);
    } else {
        snprintf(target_dir, sizeof(target_dir), "%s\\app\\%s", g_pier_root, g_packages[index].installer_name);
    }
    
    if (file_exists(target_dir)) {
        is_installed = 1;
        folder_size = calculate_folder_size(target_dir);
    }
    
    if (!is_installed) {
        vecho_line("$brightred$%s", g_str_package_not_installed);
        vecho_line("$brightwhite$%s $brightyellow$pier install %s", g_str_install_hint, g_packages[index].name);
        return 0;
    }
    
    print_package_info_remove_colored(index, folder_size);
    print_remove_warning_colored();
    
    if (!g_autoyes) {
        char response[10];
        printf("%s %s (Y/N): ", g_str_choiceremove, g_packages[index].display_name);
        fflush(stdout);
        if (fgets(response, sizeof(response), stdin)) {
            if (response[0] != 'Y' && response[0] != 'y') {
                return 0;
            }
        }
    }
    
    printf("%s %s...\n", g_str_uninstall_progress, g_packages[index].display_name);
    
    if (g_packages[index].is_language && _stricmp(g_packages[index].name, "zh-CN") == 0) {
        printf("%s\n", g_str_error_protected_lang);
        return 0;
    }
    
    /* Remove directory */
    if (file_exists(target_dir)) {
        dir_remove(target_dir);
    }
    
    /* Remove metadata */
    {
        char metadata_file[MAX_PATH_LEN];
        snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata\\%s.sque", g_pier_root, g_packages[index].installer_name);
        if (file_exists(metadata_file)) {
            remove(metadata_file);
        }
    }
    
    /* Remove from pierlist.sque */
    {
        char pierlist_file[MAX_PATH_LEN];
        snprintf(pierlist_file, sizeof(pierlist_file), "%s\\etc\\pierlist.sque", g_pier_root);
        if (file_exists(pierlist_file)) {
            remove_line_from_file(pierlist_file, g_packages[index].installer_name);
        }
    }
    
    /* Remove scuts shortcut bat */
    {
        char scuts_bat[MAX_PATH_LEN];
        snprintf(scuts_bat, sizeof(scuts_bat), "%s\\scuts\\%s.bat", g_pier_root, g_packages[index].name);
        remove(scuts_bat);
    }
    
    printf("%s\n", g_str_uninstall_success);
    vecho_line("$brightcyan$%s%s", g_str_scut_removed, g_packages[index].name);
    
    return 1;
}

/* Main function */
int main(int argc, char *argv[]) {
    int i;
    int is_install = 0;
    int is_info = 0;
    
    if (argc < 2) {
        fprintf(stderr, "pier-pkg: install|remove|search <args...>\n");
        return 1;
    }
    
    if (_stricmp(argv[1], "install") == 0) {
        is_install = 1;
    } else if (_stricmp(argv[1], "remove") == 0) {
        is_install = 0;
    } else if (_stricmp(argv[1], "search") == 0) {
        /* Search mode */
        if (argc < 6) {
            fprintf(stderr, "pier-pkg: search <PIER_ROOT> <LANGUAGE_DIR> <source_url> <keyword>\n");
            return 1;
        }
        return search_packages(argv[2], argv[3], argv[4], argv[5]);
    } else if (_stricmp(argv[1], "info") == 0) {
        if (argc < 6) {
            fprintf(stderr, "pier-pkg: info <PIER_ROOT> <LANGUAGE_DIR> <source_url> <package>\n");
            return 1;
        }
        is_info = 1;
    } else {
        fprintf(stderr, "pier-pkg: unknown command %s\n", argv[1]);
        return 1;
    }
    
    if (is_info) {
        strncpy(g_pier_root, argv[2], sizeof(g_pier_root) - 1);
        g_pier_root[sizeof(g_pier_root) - 1] = '\0';
        
        strncpy(g_language_dir, argv[3], sizeof(g_language_dir) - 1);
        g_language_dir[sizeof(g_language_dir) - 1] = '\0';
        
        {
            char *last_slash = strrchr(g_language_dir, '\\');
            if (!last_slash) last_slash = strrchr(g_language_dir, '/');
            if (last_slash) {
                strncpy(g_current_lang, last_slash + 1, sizeof(g_current_lang) - 1);
                g_current_lang[sizeof(g_current_lang) - 1] = '\0';
            }
        }
        
        strncpy(g_source_url, argv[4], sizeof(g_source_url) - 1);
        g_source_url[sizeof(g_source_url) - 1] = '\0';
        
        strncpy(g_packages[0].name, argv[5], sizeof(g_packages[0].name) - 1);
        g_packages[0].name[sizeof(g_packages[0].name) - 1] = '\0';
        g_package_count = 1;
    }

    if (is_install && argc < 9) {
        fprintf(stderr, "pier-pkg: install <PIER_ROOT> <LANGUAGE_DIR> <source_url> <pies_url> <autoyes> <SYS_ARCH> <package>...\n");
        return 1;
    }
    
    if (!is_install && !is_info && argc < 7) {
        fprintf(stderr, "pier-pkg: remove <PIER_ROOT> <LANGUAGE_DIR> <source_url> <autoyes> <package>...\n");
        return 1;
    }
    
    /* Parse arguments */
    strncpy(g_pier_root, argv[2], sizeof(g_pier_root) - 1);
    g_pier_root[sizeof(g_pier_root) - 1] = '\0';
    
    strncpy(g_language_dir, argv[3], sizeof(g_language_dir) - 1);
    g_language_dir[sizeof(g_language_dir) - 1] = '\0';
    
    /* Extract language code from LANGUAGE_DIR (e.g., "zh-CN" from "...\share\language\zh-CN") */
    {
        char *last_slash = strrchr(g_language_dir, '\\');
        if (!last_slash) last_slash = strrchr(g_language_dir, '/');
        if (last_slash) {
            strncpy(g_current_lang, last_slash + 1, sizeof(g_current_lang) - 1);
            g_current_lang[sizeof(g_current_lang) - 1] = '\0';
        }
    }
    
    strncpy(g_source_url, argv[4], sizeof(g_source_url) - 1);
    g_source_url[sizeof(g_source_url) - 1] = '\0';
    
    if (is_install) {
        strncpy(g_pies_url, argv[5], sizeof(g_pies_url) - 1);
        g_pies_url[sizeof(g_pies_url) - 1] = '\0';
        
        /* Check autoyes */
        if (_stricmp(argv[6], "-y") == 0 || _stricmp(argv[6], "y") == 0 || _stricmp(argv[6], "yes") == 0) {
            g_autoyes = 1;
        }
        
        strncpy(g_sys_arch, argv[7], sizeof(g_sys_arch) - 1);
        g_sys_arch[sizeof(g_sys_arch) - 1] = '\0';
        
        for (i = 8; i < argc && g_package_count < MAX_PACKAGES; i++) {
            strncpy(g_packages[g_package_count].name, argv[i], sizeof(g_packages[g_package_count].name) - 1);
            g_packages[g_package_count].name[sizeof(g_packages[g_package_count].name) - 1] = '\0';
            g_package_count++;
        }
    } else {
        if (_stricmp(argv[5], "-y") == 0 || _stricmp(argv[5], "y") == 0 || _stricmp(argv[5], "yes") == 0) {
            g_autoyes = 1;
        }
        
        for (i = 6; i < argc && g_package_count < MAX_PACKAGES; i++) {
            strncpy(g_packages[g_package_count].name, argv[i], sizeof(g_packages[g_package_count].name) - 1);
            g_packages[g_package_count].name[sizeof(g_packages[g_package_count].name) - 1] = '\0';
            g_package_count++;
        }
    }
    
    if (g_package_count == 0) {
        fprintf(stderr, "pier-pkg: no packages specified\n");
        return 1;
    }
    
    /* Load language strings */
    load_language();
    
    /* Process packages */
    if (is_info) {
        /* Download metadata */
        printf("%s\n", g_str_loading);
        download_metadata_batch();
        
        if (g_packages[0].metadata_downloaded) {
            parse_metadata(0);
        }
        
        /* Check if package was found */
        if (!g_packages[0].metadata_downloaded) {
            printf("%s %s\n", g_str_error_not_found, g_packages[0].name);
            return 1;
        }
        
        /* Display package info */
        display_package_info(0);
        
        /* Display aliases */
        {
            char metadata_file[MAX_PATH_LEN];
            char alias_content[MAX_LINE * 8];
            snprintf(metadata_file, sizeof(metadata_file), "%s\\share\\cache\\metadata.sque", g_pier_root);
            if (sque_read(metadata_file, "Alias", alias_content, sizeof(alias_content)) >= 0 && alias_content[0] != '\0') {
                printf("\n");
                vecho_line("$brightyellow$%s", g_str_info_alias_label);
                char *line_ptr = strtok(alias_content, "\n");
                while (line_ptr) {
                    char *trimmed = line_ptr;
                    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
                    if (strlen(trimmed) > 0 && strchr(trimmed, ':')) {
                        char line_copy[MAX_LINE];
                        strncpy(line_copy, trimmed, sizeof(line_copy) - 1);
                        line_copy[sizeof(line_copy) - 1] = '\0';
                        char *colon = strchr(line_copy, ':');
                        *colon = '\0';
                        printf("  %-10s - %s\n", line_copy, colon + 1);
                    }
                    line_ptr = strtok(NULL, "\n");
                }
            }
        }
        
        printf("\n");
        
        /* Cleanup cache */
        {
            char cache_dir[MAX_PATH_LEN];
            snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
            dir_remove(cache_dir);
        }
        
        return 0;
    }

    if (is_install) {
        /* Download all metadata in one batch */
        printf("%s\n", g_str_loading);
        download_metadata_batch();
        
        for (i = 0; i < g_package_count; i++) {
            if (!g_packages[i].metadata_downloaded) continue;
            parse_metadata(i);
        }
        
        /* Check if any package was found */
        int found_count = 0;
        for (i = 0; i < g_package_count; i++) {
            if (g_packages[i].metadata_downloaded) {
                found_count++;
            }
        }
        
        /* No packages found, exit */
        if (found_count == 0) {
            return 1;
        }
        
        /* Display info */
        for (i = 0; i < g_package_count; i++) {
            if (g_packages[i].metadata_downloaded) {
                display_package_info(i);
            }
        }
        
        /* Confirm */
        if (!confirm_install()) {
            {
                char cache_dir[MAX_PATH_LEN];
                snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
                dir_remove(cache_dir);
            }
            return 2;
        }
        
        /* Download and install */
        for (i = 0; i < g_package_count; i++) {
            if (!g_packages[i].metadata_downloaded) {
                continue;
            }
            
            if (!download_package(i)) {
                continue;
            }
            
            install_package(i);
        }
        
        /* Cleanup cache after installation */
        {
            char cache_dir[MAX_PATH_LEN];
            snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
            dir_remove(cache_dir);
        }
    } else {
        /* Remove mode */
        /* Download all metadata in one batch */
        printf("%s\n", g_str_loading);
        download_metadata_batch();
        
        for (i = 0; i < g_package_count; i++) {
            if (!g_packages[i].metadata_downloaded) continue;
            parse_metadata(i);
        }
        
        /* Remove packages */
        for (i = 0; i < g_package_count; i++) {
            if (!g_packages[i].metadata_downloaded) {
                continue;
            }
            
            remove_package(i);
        }
        
        /* Cleanup cache after removal */
        {
            char cache_dir[MAX_PATH_LEN];
            snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
            dir_remove(cache_dir);
        }
    }
    
    return 0;
}

/* Check if package is installed by checking pierlist.sque or app dir */
int is_package_installed(const char *pier_root, const char *installer_name) {
    char pierlist_path[MAX_PATH_LEN];
    char install_path[MAX_PATH_LEN];
    FILE *fp;
    char line[MAX_LINE];
    
    /* Method 1: Check pierlist.sque */
    snprintf(pierlist_path, sizeof(pierlist_path), "%s\\etc\\pierlist.sque", pier_root);
    fp = fopen(pierlist_path, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            /* Check if line contains installer_name */
            if (strstr(line, installer_name)) {
                fclose(fp);
                return 1;
            }
        }
        fclose(fp);
    }
    
    /* Method 2: Check if app dir exists */
    snprintf(install_path, sizeof(install_path), "%s\\app\\%s", pier_root, installer_name);
    if (GetFileAttributesA(install_path) != INVALID_FILE_ATTRIBUTES) {
        return 1;
    }
    
    return 0;
}

/* Search packages in db.sque */
int search_packages(const char *pier_root, const char *lang_dir, const char *source_url, const char *keyword) {
    char db_url[MAX_PATH_LEN];
    char db_file[MAX_PATH_LEN];
    FILE *fp;
    char line[MAX_LINE];
    int found_count = 0;
    
    /* Load language strings */
    strncpy(g_pier_root, pier_root, sizeof(g_pier_root) - 1);
    g_pier_root[sizeof(g_pier_root) - 1] = '\0';
    strncpy(g_language_dir, lang_dir, sizeof(g_language_dir) - 1);
    g_language_dir[sizeof(g_language_dir) - 1] = '\0';
    load_language();
    
    /* Build paths */
    {
        char cache_dir[MAX_PATH_LEN];
        snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", pier_root);
        CreateDirectoryA(cache_dir, NULL);
    }
    snprintf(db_file, sizeof(db_file), "%s\\share\\cache\\db.sque", pier_root);
    
    /* Remove existing file */
    remove(db_file);
    
    /* Download db.sque */
    {
        char pg_exe[MAX_PATH_LEN];
        char pg_args[MAX_PATH_LEN * 4];
        int dl_ret;
        /* OLD (pier-get): snprintf(pg_exe,...,"%s\\bin\\pier-get.exe",...); snprintf(pg_args,...,"-q \"%s\"...",lang_dir,...); */
        {
            char proxy_opts[512];
            build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
            snprintf(pg_exe, sizeof(pg_exe), "%s\\bin\\uma-get.exe", pier_root);
            snprintf(pg_args, sizeof(pg_args), "-q --timeout=60 --tries=3 --no-check-certificate%s -O \"%s\\share\\cache\\db.sque\" \"%s/db.sque\"",
                     proxy_opts, pier_root, source_url);
        }
        dl_ret = run_silent(pg_exe, pg_args);
        if (dl_ret != 0) {
            remove(db_file);
            vecho_line("$brightred$%s", g_str_search_download_failed);
            return 1;
        }
    }
    
    /* Check if file was downloaded */
    if (!file_exists(db_file)) {
        vecho_line("$brightred$%s", g_str_search_download_failed);
        return 1;
    }
    
    /* Print search title */
    printf("%s %s %s\n", g_str_search_title_1, keyword, g_str_search_title_2);
    
    /* Open and search db.sque */
    fp = fopen(db_file, "r");
    if (!fp) {
        printf("%s\n", g_str_search_not_found);
        return 1;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        /* Skip empty lines */
        if (strlen(line) == 0) continue;
        
        /* Check if line contains keyword (case insensitive) */
        if (stristr(line, keyword)) {
            /* Parse line: InstallerName | Version | OS | Description | InstallerName */
            char installer_name[MAX_NAME] = {0};
            char version[MAX_NAME] = {0};
            char os_req[MAX_NAME] = {0};
            char description[MAX_LINE] = {0};
            char *token;
            char *rest = line;
            int field = 0;
            
            /* Parse fields separated by | */
            token = strtok(rest, "|");
            while (token && field < 4) {
                /* Trim whitespace */
                while (*token == ' ') token++;
                char *end = token + strlen(token) - 1;
                while (end > token && *end == ' ') *end-- = '\0';
                
                switch (field) {
                    case 0: strncpy(installer_name, token, sizeof(installer_name) - 1); break;
                    case 1: strncpy(version, token, sizeof(version) - 1); break;
                    case 2: strncpy(os_req, token, sizeof(os_req) - 1); break;
                    case 3: strncpy(description, token, sizeof(description) - 1); break;
                }
                field++;
                token = strtok(NULL, "|");
            }
            
            if (strlen(installer_name) > 0) {
                int installed = is_package_installed(pier_root, installer_name);
                
                /* Print with colors using vecho_line:
                 * - Package name: bright green
                 * - Version: bright yellow
                 * - [installed/available]: bright green for installed, bright yellow for available
                 */
                if (installed) {
                    vecho_line("- $brightgreen$%s ($brightyellow$%s) $white$[$brightgreen$%s$white$]", 
                               installer_name, version, g_str_search_installed);
                } else {
                    vecho_line("- $brightgreen$%s ($brightyellow$%s) $white$[$brightyellow$%s$white$]", 
                               installer_name, version, g_str_search_available);
                }
                
                /* Print description */
                printf("  %s\n", description);
                
                found_count++;
            }
        }
    }
    
    fclose(fp);
    
    /* Cleanup */
    remove(db_file);
    
    if (found_count == 0) {
        printf("%s\n", g_str_search_not_found);
        return 1;
    }
    
    return 0;
}

/* Case insensitive strstr */
char *stristr(const char *str1, const char *str2) {
    char *pptr, *sptr, *start;
    size_t slen = strlen(str2);
    
    for (start = (char *)str1; *start; start++) {
        for (; (*start && (toupper(*start) != toupper(*str2))); start++);
        if (!*start) return NULL;
        
        pptr = (char *)str2;
        sptr = (char *)start;
        
        while (toupper(*sptr) == toupper(*pptr)) {
            sptr++;
            pptr++;
            if (--slen == 0) return start;
        }
        slen = strlen(str2);
    }
    return NULL;
}
