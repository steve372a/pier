/*
 * pier-upd.c - Pier self-updater
 * 完整的更新流程：下载 -> 解压 -> 复制
 * Compatible with C89/C90, Windows XP and later, UTF-8 support
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_PATH_LEN 512
#define MAX_CMD_LEN 2048

/* UTF-8 conversion helpers */
static int is_valid_utf8(const unsigned char *str) {
    int i = 0;
    while (str[i]) {
        if (str[i] < 0x80) {
            i++;
        } else if ((str[i] & 0xE0) == 0xC0 && str[i+1]) {
            if ((str[i+1] & 0xC0) != 0x80) return 0;
            i += 2;
        } else if ((str[i] & 0xF0) == 0xE0 && str[i+1] && str[i+2]) {
            if ((str[i+1] & 0xC0) != 0x80) return 0;
            if ((str[i+2] & 0xC0) != 0x80) return 0;
            i += 3;
        } else if ((str[i] & 0xF8) == 0xF0 && str[i+1] && str[i+2] && str[i+3]) {
            if ((str[i+1] & 0xC0) != 0x80) return 0;
            if ((str[i+2] & 0xC0) != 0x80) return 0;
            if ((str[i+3] & 0xC0) != 0x80) return 0;
            i += 4;
        } else {
            return 0;
        }
    }
    return 1;
}

static void utf8_to_console_cp(char *str, int max_len) {
    wchar_t *wide_str;
    char *out_buf;
    int wide_len, out_len;
    UINT console_cp;

    if (!str || !str[0]) return;
    if (!is_valid_utf8((const unsigned char *)str)) return;

    console_cp = GetConsoleOutputCP();
    if (console_cp == 65001) return;

    wide_len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (wide_len <= 0) return;

    wide_str = (wchar_t *)malloc(wide_len * sizeof(wchar_t));
    if (!wide_str) return;

    MultiByteToWideChar(CP_UTF8, 0, str, -1, wide_str, wide_len);

    out_len = WideCharToMultiByte(console_cp, 0, wide_str, -1, NULL, 0, NULL, NULL);
    if (out_len > 0 && out_len <= max_len) {
        out_buf = (char *)malloc(out_len);
        if (out_buf) {
            WideCharToMultiByte(console_cp, 0, wide_str, -1, out_buf, out_len, NULL, NULL);
            strncpy(str, out_buf, max_len - 1);
            str[max_len - 1] = '\0';
            free(out_buf);
        }
    }

    free(wide_str);
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

int run_process_silent(const char *exe_path, const char *args) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    HANDLE hNul;
    char cmdline[MAX_CMD_LEN];
    DWORD exit_code = (DWORD)-1;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    hNul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    if (hNul == INVALID_HANDLE_VALUE) {
        return -1;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hNul;
    si.hStdError = hNul;
    memset(&pi, 0, sizeof(pi));

    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe_path, args);

    if (CreateProcessA(exe_path, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    CloseHandle(hNul);
    return (int)exit_code;
}

void pause_for_enter(void) {
    char input[8];
    printf("Press Enter to continue . . .");
    fflush(stdout);
    fgets(input, sizeof(input), stdin);
}

/* Copy file from src to dst */
int copy_file(const char *src, const char *dst) {
    FILE *fp_src, *fp_dst;
    char buffer[4096];
    size_t bytes_read;
    
    fp_src = fopen(src, "rb");
    if (!fp_src) return 0;
    
    fp_dst = fopen(dst, "wb");
    if (!fp_dst) {
        fclose(fp_src);
        return 0;
    }
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp_src)) > 0) {
        fwrite(buffer, 1, bytes_read, fp_dst);
    }
    
    fclose(fp_src);
    fclose(fp_dst);
    return 1;
}

/* Copy directory recursively */
int copy_dir(const char *src, const char *dst) {
    char src_path[MAX_PATH_LEN];
    char dst_path[MAX_PATH_LEN];
    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    
    /* Create destination directory */
    CreateDirectoryA(dst, NULL);
    
    /* Find all files in source */
    snprintf(src_path, sizeof(src_path), "%s\\*", src);
    hFind = FindFirstFileA(src_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    
    do {
        /* Skip . and .. */
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        snprintf(src_path, sizeof(src_path), "%s\\%s", src, find_data.cFileName);
        snprintf(dst_path, sizeof(dst_path), "%s\\%s", dst, find_data.cFileName);
        
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            /* Recursively copy subdirectory */
            copy_dir(src_path, dst_path);
        } else {
            /* Copy file */
            copy_file(src_path, dst_path);
        }
    } while (FindNextFileA(hFind, &find_data));
    
    FindClose(hFind);
    return 1;
}

/* Remove directory recursively */
void remove_dir(const char *path) {
    char search_path[MAX_PATH_LEN];
    char file_path[MAX_PATH_LEN];
    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
                continue;
            }
            
            snprintf(file_path, sizeof(file_path), "%s\\%s", path, find_data.cFileName);
            
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                remove_dir(file_path);
                RemoveDirectoryA(file_path);
            } else {
                DeleteFileA(file_path);
            }
        } while (FindNextFileA(hFind, &find_data));
        
        FindClose(hFind);
    }
    
    RemoveDirectoryA(path);
}

/* Read language string from lang.ini */
int read_lang_string(const char *lang_file, const char *key, char *value, int max_len) {
    FILE *fp;
    char line[MAX_PATH_LEN];
    char section[MAX_PATH_LEN];
    int in_section = 0;
    
    fp = fopen(lang_file, "r");
    if (!fp) return -1;
    
    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        
        /* Check for section */
        if (line[0] == '[' && line[len-2] == ']') {
            strncpy(section, line + 1, len - 3);
            section[len-3] = '\0';
            if (strcmp(section, key) == 0) {
                in_section = 1;
            } else {
                in_section = 0;
            }
            continue;
        }
        
        /* Read value if in target section */
        if (in_section && line[0] != '\0' && line[0] != '[') {
            strncpy(value, line, max_len - 1);
            value[max_len - 1] = '\0';
            utf8_to_console_cp(value, max_len);  /* UTF-8 to console CP */
            fclose(fp);
            return 0;
        }
    }
    
    fclose(fp);
    return -1;
}

int main(int argc, char *argv[]) {
    char pier_root[MAX_PATH_LEN];
    char lang_dir[MAX_PATH_LEN];
    char temp_exe[MAX_PATH_LEN];
    char temp_dir[MAX_PATH_LEN];
    char new_version[MAX_PATH_LEN];
    char lang_file[MAX_PATH_LEN];
    char src_path[MAX_PATH_LEN];
    char dst_path[MAX_PATH_LEN];
    char str_progress[MAX_PATH_LEN];
    char str_downloading[MAX_PATH_LEN];
    char str_extracting[MAX_PATH_LEN];
    char str_complete[MAX_PATH_LEN];
    char str_failed[MAX_PATH_LEN];
    
    /* Get arguments */
    if (argc < 4) {
        printf("Usage: pier-upd.exe <PIER_ROOT> <LANGUAGE_DIR> <NEW_VERSION>\n");
        return 1;
    }
    
    strncpy(pier_root, argv[1], sizeof(pier_root) - 1);
    pier_root[sizeof(pier_root) - 1] = '\0';
    strncpy(lang_dir, argv[2], sizeof(lang_dir) - 1);
    lang_dir[sizeof(lang_dir) - 1] = '\0';
    strncpy(new_version, argv[3], sizeof(new_version) - 1);
    new_version[sizeof(new_version) - 1] = '\0';
    
    /* Build paths */
    snprintf(lang_file, sizeof(lang_file), "%s\\lang.ini", lang_dir);
    
    /* Read language strings */
    read_lang_string(lang_file, "update_pier_progress", str_progress, sizeof(str_progress));
    read_lang_string(lang_file, "update_pier_downloading", str_downloading, sizeof(str_downloading));
    read_lang_string(lang_file, "update_pier_extracting", str_extracting, sizeof(str_extracting));
    read_lang_string(lang_file, "update_pier_complete", str_complete, sizeof(str_complete));
    read_lang_string(lang_file, "update_pier_failed", str_failed, sizeof(str_failed));
    
    /* Get temp directory */
    GetTempPathA(sizeof(temp_dir), temp_dir);
    /* Remove trailing backslash if present */
    size_t temp_len = strlen(temp_dir);
    if (temp_len > 0 && temp_dir[temp_len - 1] == '\\') {
        temp_dir[temp_len - 1] = '\0';
    }
    
    /* Build temp exe path for cleanup */
    snprintf(temp_exe, sizeof(temp_exe), "%s\\pier-upd.exe", temp_dir);
    
    /* Perform full update */
    printf("%s\n", str_progress);
    
    /* Parent process (pier-ver.exe) has already exited */
    Sleep(1000);
    
    /* Step 1: Download */
    /* Delete existing zip file if present */
    {
        char zip_path[MAX_PATH_LEN];
        snprintf(zip_path, sizeof(zip_path), "%s\\pier-%s.zip", temp_dir, new_version);
        DeleteFileA(zip_path);
    }
    printf("%s%s...\n", str_downloading, new_version);
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char pg_cmd[MAX_CMD_LEN];
        char zip_path[MAX_PATH_LEN];
        
        snprintf(zip_path, sizeof(zip_path), "%s\\pier-%s.zip", temp_dir, new_version);
        /* OLD (pier-get): snprintf(pg_cmd, sizeof(pg_cmd), "pier-get.exe \".\" \"https://github.com/steve372a/pier/releases/download/v%s/pier-%s.zip\" \"%s\"", new_version, new_version, zip_path); */
        {
            char proxy_opts[512];
            build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
            snprintf(pg_cmd, sizeof(pg_cmd), "uma-get.exe -q --show-progress --timeout=14400 --tries=3 --no-check-certificate%s -O \"%s\" \"https://github.com/steve372a/pier/releases/download/v%s/pier-%s.zip\"",
                     proxy_opts, zip_path, new_version, new_version);
        }
        
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        
        if (!CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("%s\n", str_failed);
            pause_for_enter();
            return 1;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (GetFileAttributesA(zip_path) == INVALID_FILE_ATTRIBUTES) {
            printf("%s\n", str_failed);
            pause_for_enter();
            return 1;
        }
    }
    
    /* Step 2: Extract to pier-{version} subdirectory */
    printf("%s\n", str_extracting);
    {
        char extract_dir[MAX_PATH_LEN];
        char seven_zip_exe[MAX_PATH_LEN];
        char seven_zip_args[MAX_CMD_LEN];
        char zip_path[MAX_PATH_LEN];
        snprintf(extract_dir, sizeof(extract_dir), "%s\\pier-%s", temp_dir, new_version);
        snprintf(zip_path, sizeof(zip_path), "%s\\pier-%s.zip", temp_dir, new_version);
        CreateDirectoryA(extract_dir, NULL);
        snprintf(seven_zip_exe, sizeof(seven_zip_exe), "%s\\bin\\7za.exe", pier_root);
        snprintf(seven_zip_args, sizeof(seven_zip_args), "x \"%s\" -o\"%s\" -y",
                 zip_path, extract_dir);
        if (run_process_silent(seven_zip_exe, seven_zip_args) < 0) {
            printf("%s\n", str_failed);
            pause_for_enter();
            return 1;
        }
    }
    DeleteFileA(temp_exe);
    
    /* Step 3: Copy files */
    snprintf(src_path, sizeof(src_path), "%s\\pier-%s", temp_dir, new_version);
    snprintf(dst_path, sizeof(dst_path), "%s", pier_root);
    
    /* Debug: check if source exists */
    if (GetFileAttributesA(src_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Source directory does not exist: %s\n", src_path);
        printf("Temp dir: %s\n", temp_dir);
        printf("New version: %s\n", new_version);
        pause_for_enter();
        return 1;
    }
    
    if (!copy_dir(src_path, dst_path)) {
        printf("%s\n", str_failed);
        pause_for_enter();
        return 1;
    }
    
    /* Clean up */
    remove_dir(src_path);
    DeleteFileA(temp_exe);
    
    printf("%s\n", str_complete);
    
    return 0;
}
