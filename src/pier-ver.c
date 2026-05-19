/*
 * pier-ver.c - Package Installer Version Checker
 * 检查远程版本并提示更新
 * XP兼容，UTF-8支持
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define BUFFER_SIZE 4096
#define MAX_PATH_LEN 512
#define MAX_CMD_LEN 2048
#define VER_URL "https://steve372a.github.io/pier/ver.sque"

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

int run_vecho(const char *pier_root, const char *message) {
    char exe_path[MAX_PATH_LEN];
    char cmdline[MAX_CMD_LEN];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    snprintf(exe_path, sizeof(exe_path), "%s\\bin\\vecho.exe", pier_root);
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe_path, message);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    if (!CreateProcessA(exe_path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return 0;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
}

/* 获取程序所在目录 */
void get_exe_dir(char *output, int output_size) {
    char exe_path[MAX_PATH];
    char *last_slash;
    
    GetModuleFileName(NULL, exe_path, MAX_PATH);
    last_slash = strrchr(exe_path, '\\');
    if (last_slash) {
        *last_slash = '\0';
        /* 返回上级目录（bin的父目录） */
        last_slash = strrchr(exe_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
        }
    }
    strncpy(output, exe_path, output_size - 1);
    output[output_size - 1] = '\0';
}

/* 从lang.ini读取指定字段的值 */
int get_lang_value(const char *lang_file, const char *section, char *output, int output_size) {
    FILE *fp;
    char line[BUFFER_SIZE];
    char section_tag[256];
    int found = 0;
    
    fp = fopen(lang_file, "r");
    if (!fp) return -1;
    
    snprintf(section_tag, sizeof(section_tag), "[%s]", section);
    
    while (fgets(line, sizeof(line), fp)) {
        /* 移除换行符 */
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[len-1] = '\0';
            len--;
        }
        
        if (found) {
            /* 找到字段后的下一行就是值 */
            strncpy(output, line, output_size - 1);
            output[output_size - 1] = '\0';
            utf8_to_console_cp(output, output_size);  /* UTF-8 to console CP */
            fclose(fp);
            return 0;
        }
        
        if (strcmp(line, section_tag) == 0) {
            found = 1;
        }
    }
    
    fclose(fp);
    return -1;
}

/* 获取缓存文件路径 */
void get_cache_path(const char *pier_root, char *cache_path, int cache_size) {
    snprintf(cache_path, cache_size, "%s\\share\\cache\\ver.sque", pier_root);
}

/* 获取当前时间（秒，从系统启动开始，64位） */
ULONGLONG get_current_time_sec(void) {
    FILETIME ft;
    ULARGE_INTEGER uli;
    
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    
    /* 转换为秒（从 1601-01-01 开始的 100 纳秒间隔 -> 秒） */
    return uli.QuadPart / 10000000ULL;
}

/* 检查缓存是否有效（24小时内） */
int is_cache_valid(const char *cache_path) {
    FILE *fp;
    char line[256];
    ULONGLONG cache_time = 0;
    ULONGLONG now;
    double diff_hours;
    
    fp = fopen(cache_path, "r");
    if (!fp) return 0;
    
    /* 第一行是时间戳 */
    if (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%llu", &cache_time);
    }
    fclose(fp);
    
    if (cache_time == 0) return 0;
    
    /* 获取当前时间 */
    now = get_current_time_sec();
    
    /* 计算时间差（小时） */
    diff_hours = (double)(now - cache_time) / 3600.0;
    
    /* 24小时内有效 */
    return (diff_hours < 24.0) ? 1 : 0;
}

/* 保存缓存 */
void save_cache(const char *cache_path, const char *ver_data) {
    FILE *fp;
    ULONGLONG now;
    
    /* 确保缓存目录存在 */
    {
        char cache_dir[MAX_PATH];
        strncpy(cache_dir, cache_path, sizeof(cache_dir) - 1);
        cache_dir[sizeof(cache_dir) - 1] = '\0';
        
        char *last_slash = strrchr(cache_dir, '\\');
        if (last_slash) {
            *last_slash = '\0';
            /* 简单创建目录，忽略错误 */
            CreateDirectory(cache_dir, NULL);
        }
    }
    
    fp = fopen(cache_path, "w");
    if (!fp) return;
    
    /* 获取当前时间 */
    now = get_current_time_sec();
    
    /* 第一行：时间戳（64位） */
    fprintf(fp, "%I64u\n", now);
    /* 第二行：版本数据 */
    fprintf(fp, "%s\n", ver_data);
    
    fclose(fp);
}

/* 读取缓存 */
int read_cache(const char *cache_path, char *output, int output_size) {
    FILE *fp;
    char line[256];
    
    fp = fopen(cache_path, "r");
    if (!fp) return -1;
    
    /* 跳过第一行（时间戳） */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    
    /* 第二行是版本数据 */
    if (fgets(line, sizeof(line), fp)) {
        /* 移除换行符 */
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[len-1] = '\0';
            len--;
        }
        
        strncpy(output, line, output_size - 1);
        output[output_size - 1] = '\0';
        fclose(fp);
        return 0;
    }
    
    fclose(fp);
    return -1;
}

/* 使用curl获取远程版本信息（带24小时缓存） */
int get_remote_version(const char *pier_root, char *output, int output_size) {
    FILE *fp;
    char buffer[256];
    char cache_path[MAX_PATH];
    char tmp_file[MAX_PATH];
    
    /* 获取缓存路径 */
    get_cache_path(pier_root, cache_path, sizeof(cache_path));
    
    snprintf(tmp_file, sizeof(tmp_file), "%s\\ver.tmp",
             getenv("TEMP") ? getenv("TEMP") : "C:\\Windows\\Temp");
    
    /* 检查缓存是否有效 */
    if (is_cache_valid(cache_path)) {
        /* 使用缓存 */
        if (read_cache(cache_path, output, output_size) == 0) {
            return 0;
        }
    }
    
    /* 缓存无效或不存在，从网络获取 */
    DeleteFileA(tmp_file);
    
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char pg_cmd[MAX_CMD_LEN];
        
        /* OLD (pier-get): snprintf(pg_cmd, sizeof(pg_cmd), "pier-get.exe \".\" \"%s\" \"%s\"", VER_URL, tmp_file); */
        {
            char proxy_opts[512];
            build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
            snprintf(pg_cmd, sizeof(pg_cmd), "uma-get.exe -q --timeout=15 --tries=1 --no-check-certificate%s -O \"%s\" \"%s\"", proxy_opts, tmp_file, VER_URL);
        }
        
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        
        if (CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE,
                          CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 15000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            /* CREATE_NO_WINDOW requires Vista+, fallback for XP */
            if (CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE,
                              0, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 15000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
        
        fp = fopen(tmp_file, "r");
        if (!fp) return -1;
    }
    
    if (fgets(buffer, sizeof(buffer), fp)) {
        /* 移除换行符 */
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
            buffer[len-1] = '\0';
            len--;
        }
        
        strncpy(output, buffer, output_size - 1);
        output[output_size - 1] = '\0';
        
        fclose(fp);
        DeleteFileA(tmp_file);
        
        /* 保存到缓存 */
        save_cache(cache_path, output);
        
        return 0;
    }
    
    fclose(fp);
    DeleteFileA(tmp_file);
    return -1;
}

/* 解析版本字符串 */
/* 支持格式: "2.2.0" 或 "2.2.0,>=2.1.0" */
int parse_version(const char *ver_str, char *latest_ver, char *min_ver, int *force_update) {
    char temp[256];
    char *comma_pos;
    char *paren_pos;
    
    *force_update = 0;
    strncpy(temp, ver_str, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    /* 检查 (FORCE) 标记 */
    paren_pos = strstr(temp, "(FORCE)");
    if (paren_pos) {
        *force_update = 1;
        *paren_pos = '\0';  /* 移除 (FORCE) 标记 */
    }
    
    comma_pos = strchr(temp, ',');
    if (comma_pos) {
        *comma_pos = '\0';
        strncpy(latest_ver, temp, 63);
        latest_ver[63] = '\0';
        
        /* 解析最小版本要求 (>=x.x.x) */
        if (strncmp(comma_pos + 1, ">=", 2) == 0) {
            strncpy(min_ver, comma_pos + 3, 63);
            min_ver[63] = '\0';
        } else {
            min_ver[0] = '\0';
        }
    } else {
        strncpy(latest_ver, temp, 63);
        latest_ver[63] = '\0';
        min_ver[0] = '\0';
    }
    
    return 0;
}

/* 将版本字符串转为数字数组便于比较 */
/* 返回版本号数量，ver[0]=主版本, ver[1]=次版本, ver[2]=修订号 */
int version_to_array(const char *ver_str, int *ver) {
    char temp[64];
    char *token;
    int count = 0;
    
    strncpy(temp, ver_str, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    token = strtok(temp, ".");
    while (token && count < 3) {
        ver[count] = atoi(token);
        count++;
        token = strtok(NULL, ".");
    }
    
    /* 补齐到3位 */
    while (count < 3) {
        ver[count] = 0;
        count++;
    }
    
    return count;
}

/* 比较两个版本 */
/* 返回: -1=ver1<ver2, 0=相等, 1=ver1>ver2 */
int compare_version(const char *ver1, const char *ver2) {
    int v1[3], v2[3];
    int i;
    
    version_to_array(ver1, v1);
    version_to_array(ver2, v2);
    
    for (i = 0; i < 3; i++) {
        if (v1[i] < v2[i]) return -1;
        if (v1[i] > v2[i]) return 1;
    }
    
    return 0;
}

/* 输出高亮版本号 */
void print_highlight_ver(const char *pier_root, const char *ver) {
    /* 直接调用 vecho 显示带颜色的版本号 */
    char message[256];
    snprintf(message, sizeof(message), "$brightyellow$%s$write$", ver);
    run_vecho(pier_root, message);
}

/* 主函数 */
int main(int argc, char *argv[]) {
    char local_ver[64] = {0};
    char remote_raw[256] = {0};
    char latest_ver[64] = {0};
    char min_ver[64] = {0};
    char lang_file[MAX_PATH] = {0};
    char lang_dir[MAX_PATH] = {0};
    char lang_value[512] = {0};
    char pier_root[MAX_PATH] = {0};
    char *env_root;
    int need_update = 0;
    int need_warning = 0;
    int force_update = 0;
    
    /* 检查参数 */
    if (argc < 2) {
        char *env_ver = getenv("PIER_VERSION");
        if (env_ver) {
            strncpy(local_ver, env_ver, sizeof(local_ver) - 1);
        } else {
            fprintf(stderr, "Usage: pier-ver.exe <local_version>\n");
            fprintf(stderr, "   or: set PIER_VERSION=x.x.x && pier-ver.exe\n");
            return 1;
        }
    } else {
        strncpy(local_ver, argv[1], sizeof(local_ver) - 1);
    }
    
    /* 获取PIER_ROOT */
    env_root = getenv("PIER_ROOT");
    if (env_root) {
        strncpy(pier_root, env_root, sizeof(pier_root) - 1);
    } else {
        /* 从程序路径推断 */
        get_exe_dir(pier_root, sizeof(pier_root));
    }
    
    /* 构建lang.ini路径 */
    /* 优先使用 PIER_LANG_DIR 环境变量（由 piec.bat 设置） */
    {
        char *env_lang_dir = getenv("PIER_LANG_DIR");
        if (env_lang_dir) {
            strncpy(lang_dir, env_lang_dir, sizeof(lang_dir) - 1);
            lang_dir[sizeof(lang_dir) - 1] = '\0';
        } else {
            /* 默认使用 zh-CN */
            snprintf(lang_dir, sizeof(lang_dir),
                     "%s\\share\\language\\zh-CN", pier_root);
        }
        snprintf(lang_file, sizeof(lang_file), "%s\\lang.ini", lang_dir);
    }
    
    /* 获取远程版本（带24小时缓存） */
    if (get_remote_version(pier_root, remote_raw, sizeof(remote_raw)) != 0) {
        /* 无法获取远程版本，静默退出 */
        return 0;
    }
    
    /* 解析版本 */
    parse_version(remote_raw, latest_ver, min_ver, &force_update);
    
    /* 比较版本 */
    if (compare_version(local_ver, latest_ver) < 0) {
        need_update = 1;
    }
    
    /* 检查最低版本要求 */
    if (min_ver[0] != '\0') {
        if (compare_version(local_ver, min_ver) < 0) {
            need_warning = 1;
        }
    }
    
    /* 输出提示 */
    if (need_update || need_warning) {
        /* 更新提示 */
        if (need_update) {
            char vecho_cmd[1024];
            /* 读取 [version_check_update] */
            if (get_lang_value(lang_file, "version_check_update", lang_value, sizeof(lang_value)) == 0) {
                /* 合并 [version_check_update] 和版本号一起输出 */
                snprintf(vecho_cmd, sizeof(vecho_cmd), "%s$brightyellow$%s$write$", lang_value, latest_ver);
                run_vecho(pier_root, vecho_cmd);
            } else {
                /* 如果没有 [version_check_update]，只显示版本号 */
                print_highlight_ver(pier_root, latest_ver);
            }
            
            /* 读取 [version_check_suffix] */
            if (get_lang_value(lang_file, "version_check_suffix", lang_value, sizeof(lang_value)) == 0 && lang_value[0] != '\0') {
                snprintf(vecho_cmd, sizeof(vecho_cmd), "%s", lang_value);
                run_vecho(pier_root, vecho_cmd);
            }
        }
        
        /* 最低版本警告 */
        if (need_warning) {
            /* 如果是强制更新，使用 vecho 显示 [pier_force_update] */
            if (force_update) {
                char vecho_cmd[1024];
                char processed_value[1024];
                char *placeholder;
                /* 读取 [pier_force_update] */
                if (get_lang_value(lang_file, "pier_force_update", lang_value, sizeof(lang_value)) == 0) {
                    /* 替换 {version} 为当前版本号 */
                    strncpy(processed_value, lang_value, sizeof(processed_value) - 1);
                    processed_value[sizeof(processed_value) - 1] = '\0';
                    placeholder = strstr(processed_value, "{version}");
                    if (placeholder) {
                        char temp[1024];
                        *placeholder = '\0';
                        snprintf(temp, sizeof(temp), "%s%s%s", 
                                 processed_value, local_ver, placeholder + 9);
                        strncpy(processed_value, temp, sizeof(processed_value) - 1);
                        processed_value[sizeof(processed_value) - 1] = '\0';
                    }
                    snprintf(vecho_cmd, sizeof(vecho_cmd), "%s", processed_value);
                } else {
                    snprintf(vecho_cmd, sizeof(vecho_cmd),
                             "$brightred$强制更新要求：$write$您的版本过低，请立即更新！");
                }
                run_vecho(pier_root, vecho_cmd);
                printf("\n");
                
                /* 复制 pier-upd.exe 到 temp 并调用 */
                {
                    char src_exe[MAX_PATH_LEN];
                    char dst_exe[MAX_PATH_LEN];
                    char temp_dir[MAX_PATH_LEN];
                    char cmd[MAX_CMD_LEN];
                    FILE *fp_src, *fp_dst;
                    char buffer[4096];
                    size_t bytes_read;
                    
                    /* 获取 temp 目录 */
                    GetTempPathA(sizeof(temp_dir), temp_dir);
                    /* 移除末尾的反斜杠 */
                    size_t len = strlen(temp_dir);
                    if (len > 0 && temp_dir[len - 1] == '\\') {
                        temp_dir[len - 1] = '\0';
                    }
                    
                    /* 构建路径 */
                    snprintf(src_exe, sizeof(src_exe), "%s\\bin\\pier-upd.exe", pier_root);
                    snprintf(dst_exe, sizeof(dst_exe), "%s\\pier-upd.exe", temp_dir);
                    
                    /* 复制文件 */
                    fp_src = fopen(src_exe, "rb");
                    if (fp_src) {
                        fp_dst = fopen(dst_exe, "wb");
                        if (fp_dst) {
                            while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp_src)) > 0) {
                                fwrite(buffer, 1, bytes_read, fp_dst);
                            }
                            fclose(fp_dst);
                        }
                        fclose(fp_src);
                    }
                    
                    /* 调用 temp 中的 pier-upd.exe（使用 CreateProcess 同步执行，等待完成） */
                    {
                        STARTUPINFOA si;
                        PROCESS_INFORMATION pi;
                        char cmd_line[MAX_CMD_LEN];
                        DWORD exit_code = 0;
                        
                        ZeroMemory(&si, sizeof(si));
                        si.cb = sizeof(si);
                        ZeroMemory(&pi, sizeof(pi));
                        
                        /* 构建命令行：exe_path arg1 arg2 arg3 */
                        snprintf(cmd_line, sizeof(cmd_line), "\"%s\" \"%s\" \"%s\" %s", 
                                 dst_exe, pier_root, lang_dir, latest_ver);
                        
                        if (CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                            /* 等待子进程完成 */
                            WaitForSingleObject(pi.hProcess, INFINITE);
                            GetExitCodeProcess(pi.hProcess, &exit_code);
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        }
                    }
                    /* 退出，不返回 piec.bat（返回2表示已触发强制更新） */
                    exit(2);
                }
            } else {
                /* 普通最低版本警告 */
                char vecho_cmd[1024];
                /* 读取 [version_check_minver] */
                if (get_lang_value(lang_file, "version_check_minver", lang_value, sizeof(lang_value)) == 0) {
                    snprintf(vecho_cmd, sizeof(vecho_cmd), "%s (>=", lang_value);
                    run_vecho(pier_root, vecho_cmd);
                }
                
                print_highlight_ver(pier_root, min_ver);
                
                /* 读取 [version_check_minver_suffix] */
                if (get_lang_value(lang_file, "version_check_minver_suffix", lang_value, sizeof(lang_value)) == 0) {
                    snprintf(vecho_cmd, sizeof(vecho_cmd), ")%s", lang_value);
                    run_vecho(pier_root, vecho_cmd);
                }
            }
        }
        
        printf("\n");
    }
    
    return 0;
}
