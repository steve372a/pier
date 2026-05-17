/*
 * pier-get.c - Multi-threaded segmented downloader for Pier
 * Uses libcurl Schannel backend for TLS 1.2+ on XP~Win11
 * Usage: pier-get.exe [-q] <lang_dir> <url> <outfile> [threads]
 * C89 compatible, TCC build, XP SP3+
 *
 * Replaces: uma-get.exe (wget), CURL.EXE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "sque.h"

/* ============================================================
 * Minimal libcurl declarations (curl.h subset, no header needed)
 * ============================================================ */
typedef void CURL;
typedef int CURLcode;
typedef int CURLoption;

#define CURLE_OK                    0
#define CURL_GLOBAL_ALL             3
#define CURLOPT_URL                 10002
#define CURLOPT_NOBODY              44
#define CURLOPT_HEADER              42
#define CURLOPT_FOLLOWLOCATION      52
#define CURLOPT_RANGE               10007
#define CURLOPT_WRITEFUNCTION       20011
#define CURLOPT_WRITEDATA           10001
#define CURLOPT_ERRORBUFFER         10010
#define CURLOPT_FAILONERROR         45
#define CURLOPT_USERAGENT           10018
#define CURLOPT_TIMEOUT             13
#define CURLOPT_CONNECTTIMEOUT      78
#define CURLOPT_HEADERFUNCTION      20079
#define CURLOPT_HEADERDATA          10029
#define CURLINFO_CONTENT_LENGTH_DOWNLOAD 0x1000002
#define CURL_ERROR_SIZE             256
#define CURLOPT_SSLVERSION          32
#define CURLOPT_SSL_VERIFYPEER      64
#define CURLOPT_SSL_VERIFYHOST      81
#define CURLOPT_SSL_ENABLE_ALPN     226
#define CURLOPT_CAINFO              10065
#define CURL_SSLVERSION_DEFAULT     0
#define CURL_SSLVERSION_TLSv1       1
#define CURL_SSLVERSION_TLSv1_2     6

typedef size_t (*curl_write_cb)(char *ptr, size_t size, size_t nmemb, void *userdata);

CURLcode curl_global_init(long flags);
CURL     *curl_easy_init(void);
CURLcode  curl_easy_setopt(CURL *curl, CURLoption option, ...);
CURLcode  curl_easy_perform(CURL *curl);
CURLcode  curl_easy_getinfo(CURL *curl, int info, ...);
void      curl_easy_cleanup(CURL *curl);
const char *curl_easy_strerror(CURLcode code);

/* ============================================================
 * Global progress counters (shared across threads)
 * ============================================================ */
static volatile LONG g_total_done = 0;
static volatile LONG g_total_size = 0;
static volatile LONG g_seg_active = 0;
static volatile LONG g_seg_error = 0;
static DWORD g_start_tick = 0;
static int g_insecure = 0;
static char g_cacert_path[768] = {0};
static int g_cacert_warned = 0;

/* Detect Windows XP: version 5.1 or 5.2 */
static int is_windows_xp(void) {
    OSVERSIONINFO osvi;
    memset(&osvi, 0, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi)) return 0;
    return (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1);
}

/* Find CA certificate bundle next to the EXE */
static void find_ca_bundle(void) {
    char exe_dir[512];
    char *p;
    DWORD len;

    len = GetModuleFileNameA(NULL, exe_dir, sizeof(exe_dir) - 1);
    if (len == 0 || len >= sizeof(exe_dir) - 1) return;
    exe_dir[len] = '\0';

    p = strrchr(exe_dir, '\\');
    if (p) *p = '\0';

    snprintf(g_cacert_path, sizeof(g_cacert_path), "%s\\cacert.pem", exe_dir);
    if (GetFileAttributesA(g_cacert_path) != INVALID_FILE_ATTRIBUTES) return;

    g_cacert_path[0] = '\0';
}

/* ============================================================
 * Language strings (loaded from lang.ini at runtime)
 * ============================================================ */
static char g_lang_eta_prefix[32];
static char g_lang_min_suffix[16];
static char g_lang_sec_suffix[16];
static char g_lang_downloaded[32];

/* ============================================================
 * Per-segment download task
 * ============================================================ */
typedef struct {
    char    url[1024];
    char    outfile[512];
    int     seg_index;
    int     seg_start;
    int     seg_end;
    char    errbuf[CURL_ERROR_SIZE];
} seg_task_t;

/* Write callback: append data at current file position */
static size_t write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *fp = (FILE *)userdata;
    size_t written = fwrite(ptr, size, nmemb, fp);
    InterlockedExchangeAdd(&g_total_done, (LONG)(written * size));
    return written * size;
}

/* Header callback: capture Content-Length from response headers */
static size_t header_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    int *pcl = (int *)userdata;
    if (total > 15 && (*pcl <= 0)) {
        const char prefix[] = "Content-Length:";
        if (strncmp(ptr, prefix, sizeof(prefix) - 1) == 0) {
            *pcl = atoi(ptr + sizeof(prefix) - 1);
            InterlockedExchange(&g_total_size, *pcl);
        }
    }
    return total;
}

/* Single segment download thread */
static DWORD WINAPI seg_thread(void *arg) {
    seg_task_t *t = (seg_task_t *)arg;
    CURL *curl;
    CURLcode res;
    FILE *fp;
    char range[64];
    int has_range;
    int header_cl = 0;

    InterlockedIncrement(&g_seg_active);

    curl = curl_easy_init();
    if (!curl) {
        InterlockedDecrement(&g_seg_active);
        InterlockedIncrement(&g_seg_error);
        return 1;
    }

    has_range = (t->seg_start > 0 || t->seg_end > 0);
    if (has_range) {
        snprintf(range, sizeof(range), "%d-%d", t->seg_start, t->seg_end);
    }

    fp = fopen(t->outfile, "r+b");
    if (!fp) {
        curl_easy_cleanup(curl);
        InterlockedDecrement(&g_seg_active);
        InterlockedIncrement(&g_seg_error);
        return 1;
    }
    fseek(fp, t->seg_start, SEEK_SET);

    curl_easy_setopt(curl, CURLOPT_URL, t->url);
    if (has_range) {
        curl_easy_setopt(curl, CURLOPT_RANGE, range);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "pier-get/2.4.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, t->errbuf);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_cl);

    /* SSL: force TLS 1.2 (required by GitHub Pages), disable ALPN */
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_SSL_ENABLE_ALPN, 0L);
    if (g_cacert_path[0]) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, g_cacert_path);
    } else {
        if (!g_cacert_warned) {
            g_cacert_warned = 1;
            fprintf(stderr, "pier-get: cacert.pem not found, HTTPS cert verify disabled\n"
                            "         place cacert.pem next to pier-get.exe to enable\n");
        }
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    InterlockedDecrement(&g_seg_active);

    if (res != CURLE_OK) {
        InterlockedIncrement(&g_seg_error);
        fprintf(stderr, "\rpier-get: %s (%s)                \n",
                curl_easy_strerror(res), t->errbuf);
        return 1;
    }
    return 0;
}

static int get_content_length(const char *url) {
    CURL *curl;
    CURLcode res;
    double cl = 0;
    char errbuf[CURL_ERROR_SIZE];

    memset(errbuf, 0, sizeof(errbuf));

    curl = curl_easy_init();
    if (!curl) return -1;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "pier-get/2.4.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_SSL_ENABLE_ALPN, 0L);
    if (g_cacert_path[0]) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, g_cacert_path);
    } else {
        if (!g_cacert_warned) {
            g_cacert_warned = 1;
            fprintf(stderr, "pier-get: cacert.pem not found, HTTPS cert verify disabled\n"
                            "         place cacert.pem next to pier-get.exe to enable\n");
        }
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);
    }
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && cl > 0) return (int)cl;
    return -1;
}

/* Format bytes to human-readable string */
static void fmt_size(char *buf, int bufsz, int bytes) {
    if (bytes < 1024) {
        snprintf(buf, bufsz, "%d B", bytes);
    } else if (bytes < 1048576) {
        snprintf(buf, bufsz, "%.1f KB", bytes / 1024.0);
    } else {
        snprintf(buf, bufsz, "%.1f MB", bytes / 1048576.0);
    }
}

static void load_lang_strings(const char *lang_dir) {
    char ini_path[560];

    if (!lang_dir || !lang_dir[0] || strcmp(lang_dir, ".") == 0) {
        strncpy(g_lang_eta_prefix, "ETA ", sizeof(g_lang_eta_prefix));
        strncpy(g_lang_min_suffix, "m", sizeof(g_lang_min_suffix));
        strncpy(g_lang_sec_suffix, "s", sizeof(g_lang_sec_suffix));
        strncpy(g_lang_downloaded, "downloaded", sizeof(g_lang_downloaded));
        return;
    }

    snprintf(ini_path, sizeof(ini_path), "%s\\lang.ini", lang_dir);
    strncpy(g_lang_eta_prefix, "ETA ", sizeof(g_lang_eta_prefix));
    sque_read(ini_path, "pg_eta_prefix", g_lang_eta_prefix, sizeof(g_lang_eta_prefix));
    strncpy(g_lang_min_suffix, "m", sizeof(g_lang_min_suffix));
    sque_read(ini_path, "pg_min_suffix", g_lang_min_suffix, sizeof(g_lang_min_suffix));
    strncpy(g_lang_sec_suffix, "s", sizeof(g_lang_sec_suffix));
    sque_read(ini_path, "pg_sec_suffix", g_lang_sec_suffix, sizeof(g_lang_sec_suffix));
    strncpy(g_lang_downloaded, "downloaded", sizeof(g_lang_downloaded));
    sque_read(ini_path, "pg_downloaded", g_lang_downloaded, sizeof(g_lang_downloaded));
}

/* ============================================================
 * Progress bar with color, speed, ETA (like aria2)
 * ============================================================ */
static void draw_bar(const char *label, int done, int quiet) {
    HANDLE hStdout;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int con_width, bar_width;
    int pct, i, filled;
    char sz_speed[32];
    DWORD elapsed;
    int speed_bps, eta, eta_min, eta_sec;
    static int spinner = 0;
    static const char spin_chars[] = "|/-\\";
    int total;

    if (quiet) return;

    total = InterlockedCompareExchange(&g_total_size, 0, 0);

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    con_width = 80;
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        con_width = csbi.dwSize.X;
    }

    if (total <= 0) {
        if (done > 0) {
            char sz_done[32];
            fmt_size(sz_done, sizeof(sz_done), done);
            if (g_start_tick > 0) {
                elapsed = (GetTickCount() - g_start_tick) / 1000;
                if (elapsed < 1) elapsed = 1;
                speed_bps = done / elapsed;
                fmt_size(sz_speed, sizeof(sz_speed), speed_bps);
                printf("\r%-14s %s %s  %s/s %c         ",
                       label, g_lang_downloaded, sz_done,
                       sz_speed, spin_chars[spinner & 3]);
            } else {
                printf("\r%-14s %s %s  %c         ",
                       label, g_lang_downloaded, sz_done,
                       spin_chars[spinner & 3]);
            }
            spinner++;
        } else {
            printf("\r%-14s %c            ", label, spin_chars[spinner & 3]);
            spinner++;
        }
        fflush(stdout);
        return;
    }

    pct = (int)(((long long)done * 100) / total);
    if (pct > 100) pct = 100;

    fmt_size(sz_speed, sizeof(sz_speed), 0);
    elapsed = (GetTickCount() - g_start_tick) / 1000;
    if (elapsed < 1) elapsed = 1;
    speed_bps = done / elapsed;
    if (speed_bps < 1) speed_bps = 1;
    fmt_size(sz_speed, sizeof(sz_speed), speed_bps);

    eta = (total - done) / speed_bps;
    eta_min = eta / 60;
    eta_sec = eta % 60;

    bar_width = con_width - 55 - 10;
    if (bar_width < 5) bar_width = 5;
    if (bar_width > 100) bar_width = 100;

    filled = pct * bar_width / 100;
    if (filled > bar_width) filled = bar_width;

    printf("\r%-14s %3d%% [", label, pct);

    SetConsoleTextAttribute(hStdout, 10);
    for (i = 0; i < filled; i++) putchar('#');

    SetConsoleTextAttribute(hStdout, 7);
    for (i = filled; i < bar_width; i++) putchar('-');

    printf("] %s/s %s", sz_speed, g_lang_eta_prefix);
    if (eta_min > 0) {
        printf("%d%s%02d%s", eta_min, g_lang_min_suffix,
               eta_sec, g_lang_sec_suffix);
    } else {
        printf("%d%s", eta_sec, g_lang_sec_suffix);
    }
    for (i = 0; i < 10; i++) putchar(' ');
    fflush(stdout);
}

/* ============================================================
 * Main download function
 * ============================================================ */
int pier_get_download(const char *url, const char *outfile,
                      int num_threads, int quiet) {
    int content_len;
    HANDLE hThreads[16];
    seg_task_t tasks[16];
    int i, chunk;
    char label[256];
    char *fname;

    if (num_threads < 1) num_threads = 1;
    if (num_threads > 16) num_threads = 16;

    /* XP single-core or threading races: force single thread */
    if (num_threads > 1 && is_windows_xp()) {
        num_threads = 1;
    }

    fname = strrchr(outfile, '\\');
    if (fname) fname++; else fname = (char *)outfile;
    snprintf(label, sizeof(label), "%.14s", fname);

    g_total_done = 0;
    g_total_size = 0;
    g_seg_active = 0;
    g_seg_error = 0;
    g_start_tick = GetTickCount();

    content_len = get_content_length(url);
    if (content_len <= 0) {
        num_threads = 1;
    }
    InterlockedExchange(&g_total_size, content_len);

    {
        FILE *fp = fopen(outfile, "wb");
        if (!fp) {
            fprintf(stderr, "pier-get: cannot create %s\n", outfile);
            return 1;
        }
        fclose(fp);
    }

    if (num_threads == 1 || content_len <= 0) {
        seg_task_t t;
        HANDLE h;
        DWORD exitCode;

        memset(&t, 0, sizeof(t));
        strncpy(t.url, url, sizeof(t.url) - 1);
        strncpy(t.outfile, outfile, sizeof(t.outfile) - 1);
        t.seg_index = 0;
        t.seg_start = 0;
        t.seg_end = 0;

        h = CreateThread(NULL, 0, seg_thread, &t, 0, NULL);

        while (WaitForSingleObject(h, 100) == WAIT_TIMEOUT) {
            int cur = InterlockedCompareExchange(&g_total_done, 0, 0);
            draw_bar(label, cur, quiet);
        }
        GetExitCodeThread(h, &exitCode);
        CloseHandle(h);
        draw_bar(label, InterlockedCompareExchange(&g_total_done, 0, 0), quiet);
        printf("\n");
        return exitCode;
    }

    chunk = content_len / num_threads;

    for (i = 0; i < num_threads; i++) {
        memset(&tasks[i], 0, sizeof(seg_task_t));
        strncpy(tasks[i].url, url, sizeof(tasks[i].url) - 1);
        strncpy(tasks[i].outfile, outfile, sizeof(tasks[i].outfile) - 1);
        tasks[i].seg_index = i;
        tasks[i].seg_start = i * chunk;
        tasks[i].seg_end = (i == num_threads - 1) ? content_len - 1
                           : (i + 1) * chunk - 1;

        hThreads[i] = CreateThread(NULL, 0, seg_thread, &tasks[i], 0, NULL);
    }

    while (1) {
        int cur = InterlockedCompareExchange(&g_total_done, 0, 0);
        int active = InterlockedCompareExchange(&g_seg_active, 0, 0);
        int errors = InterlockedCompareExchange(&g_seg_error, 0, 0);

        if (active == 0) break;
        if (errors > 0) {
            if (!quiet) fprintf(stderr, "\rpier-get: %d segment(s) failed\n",
                                errors);
            break;
        }

        draw_bar(label, cur, quiet);
        Sleep(100);
    }

    WaitForMultipleObjects(num_threads, hThreads, TRUE, 5000);
    for (i = 0; i < num_threads; i++) CloseHandle(hThreads[i]);

    draw_bar(label, InterlockedCompareExchange(&g_total_done, 0, 0), quiet);
    printf("\n");

    if (InterlockedCompareExchange(&g_seg_error, 0, 0) > 0) return 1;
    return 0;
}

/* ============================================================
 * Single download with shared CURL handle (for batch reuse)
 * ============================================================ */
static int pier_get_shared(CURL *curl, const char *url, const char *outfile,
                           int segments, int quiet) {
    int content_len;
    HANDLE hThreads[16];
    seg_task_t tasks[16];
    int i, chunk;
    char label[256];
    char *fname;
    CURL *seg_curls[16];

    if (segments < 1) segments = 1;
    if (segments > 16) segments = 16;

    /* XP single-core or threading races: force single thread */
    if (segments > 1 && is_windows_xp()) {
        segments = 1;
    }

    fname = strrchr(outfile, '\\');
    if (fname) fname++; else fname = (char *)outfile;
    snprintf(label, sizeof(label), "%.14s", fname);

    g_total_done = 0;
    g_total_size = 0;
    g_seg_active = 0;
    g_seg_error = 0;
    g_start_tick = GetTickCount();

    content_len = get_content_length(url);
    if (content_len <= 0) {
        segments = 1;
    }
    InterlockedExchange(&g_total_size, content_len);

    {
        FILE *fp = fopen(outfile, "wb");
        if (!fp) {
            fprintf(stderr, "pier-get: cannot create %s\n", outfile);
            return 1;
        }
        fclose(fp);
    }

    if (segments == 1) {
        seg_task_t t;
        HANDLE h;
        DWORD exitCode;

        memset(&t, 0, sizeof(t));
        strncpy(t.url, url, sizeof(t.url) - 1);
        strncpy(t.outfile, outfile, sizeof(t.outfile) - 1);
        t.seg_index = 0;
        t.seg_start = 0;
        t.seg_end = 0;

        h = CreateThread(NULL, 0, seg_thread, &t, 0, NULL);

        while (WaitForSingleObject(h, 100) == WAIT_TIMEOUT) {
            int cur = InterlockedCompareExchange(&g_total_done, 0, 0);
            draw_bar(label, cur, quiet);
        }
        GetExitCodeThread(h, &exitCode);
        CloseHandle(h);
        draw_bar(label, InterlockedCompareExchange(&g_total_done, 0, 0), quiet);
        if (!quiet) printf("\n");
        return exitCode;
    }

    chunk = content_len / segments;

    for (i = 0; i < segments; i++) {
        memset(&tasks[i], 0, sizeof(seg_task_t));
        strncpy(tasks[i].url, url, sizeof(tasks[i].url) - 1);
        strncpy(tasks[i].outfile, outfile, sizeof(tasks[i].outfile) - 1);
        tasks[i].seg_index = i;
        tasks[i].seg_start = i * chunk;
        tasks[i].seg_end = (i == segments - 1) ? content_len - 1
                           : (i + 1) * chunk - 1;

        hThreads[i] = CreateThread(NULL, 0, seg_thread, &tasks[i], 0, NULL);
    }

    while (1) {
        int cur = InterlockedCompareExchange(&g_total_done, 0, 0);
        int active = InterlockedCompareExchange(&g_seg_active, 0, 0);
        int errors = InterlockedCompareExchange(&g_seg_error, 0, 0);

        if (active == 0) break;
        if (errors > 0) {
            if (!quiet) fprintf(stderr, "\rpier-get: %d segment(s) failed\n",
                                errors);
            break;
        }

        draw_bar(label, cur, quiet);
        Sleep(100);
    }

    WaitForMultipleObjects(segments, hThreads, TRUE, 5000);
    for (i = 0; i < segments; i++) CloseHandle(hThreads[i]);

    draw_bar(label, InterlockedCompareExchange(&g_total_done, 0, 0), quiet);
    if (!quiet) printf("\n");

    if (InterlockedCompareExchange(&g_seg_error, 0, 0) > 0) return 1;
    return 0;
}

/* ============================================================
 * Single-threaded download reusing a persistent curl handle
 * for batch mode. Eliminates TLS handshake on requests 2..N.
 * Uses header callback to capture Content-Length during GET,
 * no separate HEAD round-trip needed.
 * ============================================================ */
int pier_get_download_reuse(CURL *curl, const char *url,
                             const char *outfile, int quiet) {
    CURLcode res;
    FILE *fp;
    int header_cl = 0;
    char errbuf[CURL_ERROR_SIZE];
    char label[256];
    char *fname;

    memset(errbuf, 0, sizeof(errbuf));

    fname = strrchr(outfile, '\\');
    if (fname) fname++; else fname = (char *)outfile;
    snprintf(label, sizeof(label), "%.14s", fname);

    g_total_done = 0;
    g_total_size = 0;
    g_seg_active = 1;
    g_seg_error = 0;
    g_start_tick = GetTickCount();

    fp = fopen(outfile, "wb");
    if (!fp) {
        g_seg_active = 0;
        if (!quiet) fprintf(stderr, "pier-get: cannot create %s\n", outfile);
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_cl);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

    res = curl_easy_perform(curl);
    fclose(fp);

    g_seg_active = 0;

    if (res != CURLE_OK) {
        DeleteFileA(outfile);
        if (!quiet)
            fprintf(stderr, "pier-get: %s (%s)\n",
                    curl_easy_strerror(res),
                    errbuf[0] ? errbuf : "");
        return 1;
    }

    if (!quiet) printf("\n");
    return 0;
}

/* ============================================================
 * Main entry
 *   Single:  pier-get.exe [-q] <lang_dir> <url> <outfile> [threads]
 *   Batch:   pier-get.exe [-q] -b <lang_dir> <url> <outfile> [<url2> <out2> ...]
 * ============================================================ */
int main(int argc, char *argv[]) {
    int threads = 4;
    int ret;
    int quiet = 0;
    int batch = 0;
    int arg_start = 1;
    char *lang_dir;
    char *url;
    char *outfile;

    if (argc > 1 && strcmp(argv[1], "-q") == 0) {
        quiet = 1;
        arg_start = 2;
    }

    if (arg_start < argc && (strcmp(argv[arg_start], "-k") == 0 ||
                         strcmp(argv[arg_start], "--insecure") == 0)) {
        g_insecure = 1;
        arg_start++;
    }

    if (arg_start < argc && strcmp(argv[arg_start], "-b") == 0) {
        batch = 1;
        arg_start++;
    }

    find_ca_bundle();

    if (argc - arg_start < 3) {
        printf("pier-get - Multi-threaded segmented downloader v%s\n", "2.4.0");
        printf("Usage:\n");
        printf("  pier-get.exe [-q] [-k] <lang_dir> <url> <output> [threads]\n");
        printf("  pier-get.exe [-q] [-k] -b <lang_dir> <url> <output> [<url2> <out2> ...]\n");
        printf("  -q          quiet mode (no progress output)\n");
        printf("  -k --insecure  disable SSL verify (for XP+old systems)\n");
        printf("  -b          batch mode: download multiple files reusing TLS session\n");
        printf("  lang_dir    language directory or \".\" for English fallback\n");
        printf("  threads     parallel segments (default: 4, max: 16)\n");
        return 1;
    }

    lang_dir = argv[arg_start];

    curl_global_init(CURL_GLOBAL_ALL);
    load_lang_strings(lang_dir);

    if (batch) {
        /* Batch mode: download multiple URL+outfile pairs sequentially,
         * reusing a single curl handle for TLS session persistence. */
        CURL *shared_curl;
        int pair_count = (argc - arg_start - 1) / 2;
        int i;
        int failures = 0;

        if ((argc - arg_start - 1) % 2 != 0 || pair_count < 1) {
            fprintf(stderr, "pier-get: -b needs even number of <url> <output> pairs\n");
            return 1;
        }

        shared_curl = curl_easy_init();
        if (!shared_curl) return 1;

        curl_easy_setopt(shared_curl, CURLOPT_SSLVERSION, (long)CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(shared_curl, CURLOPT_SSL_ENABLE_ALPN, 0L);
        curl_easy_setopt(shared_curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(shared_curl, CURLOPT_USERAGENT, "pier-get/2.4.0");
        curl_easy_setopt(shared_curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(shared_curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(shared_curl, CURLOPT_FAILONERROR, 1L);

        if (g_cacert_path[0]) {
            curl_easy_setopt(shared_curl, CURLOPT_CAINFO, g_cacert_path);
        } else {
            if (!g_cacert_warned) {
                g_cacert_warned = 1;
                fprintf(stderr, "pier-get: cacert.pem not found, HTTPS cert verify disabled\n"
                                "         place cacert.pem next to pier-get.exe to enable\n");
            }
            curl_easy_setopt(shared_curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(shared_curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        for (i = 0; i < pair_count; i++) {
            url = argv[arg_start + 1 + i * 2];
            outfile = argv[arg_start + 2 + i * 2];
            ret = pier_get_download_reuse(shared_curl, url, outfile, quiet);
            if (ret != 0) {
                if (!quiet) fprintf(stderr, "pier-get: failed: %s\n", url);
                failures++;
            }
        }
        curl_easy_cleanup(shared_curl);
        return failures > 0 ? 1 : 0;
    }

    /* Single download mode (backward compatible) */
    url = argv[arg_start + 1];
    outfile = argv[arg_start + 2];

    if (argc - arg_start >= 4 && !batch) {
        threads = atoi(argv[arg_start + 3]);
        if (threads < 1) threads = 1;
        if (threads > 16) threads = 16;
    }

    ret = pier_get_download(url, outfile, threads, quiet);
    return ret;
}