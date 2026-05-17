/*
 * pier-hash.c - SHA256 hash calculator for Pier Package Installer
 * Compatible with Windows XP and later
 * Usage: pier-hash.exe [-c|check <file1> <file2>] [-g|gene <file>] <lang_dir>
 * Output uses vecho.exe for colored output
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>

#define MAX_PATH_LEN 1024
#define MAX_LINE 4096
#define MAX_CMD_LEN 8192

static char g_pier_root[MAX_PATH_LEN];
static char g_hash_ok[MAX_CMD_LEN];
static char g_hash_fail[MAX_CMD_LEN];
static char g_hash_match[MAX_CMD_LEN];
static char g_hash_mismatch[MAX_CMD_LEN];
static char g_hash_gene[MAX_CMD_LEN];
static char g_hash_error_open[MAX_CMD_LEN];
static char g_hash_usage[MAX_CMD_LEN];

/* SHA256 context structure */
typedef struct {
    unsigned int state[8];
    unsigned long long bitcount;
    unsigned char buffer[64];
    unsigned int bufferlen;
} SHA256_CTX;

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

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/* ---- SHA256 Implementation ---- */

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

/* ---- Pier root detection ---- */

static void detect_pier_root(void)
{
    char exe_path[MAX_PATH_LEN];
    char *last_slash;

    GetModuleFileName(NULL, exe_path, sizeof(exe_path));
    last_slash = strrchr(exe_path, '\\');
    if (last_slash) {
        *last_slash = '\0';
        last_slash = strrchr(exe_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
        }
    }
    strncpy(g_pier_root, exe_path, sizeof(g_pier_root) - 1);
    g_pier_root[sizeof(g_pier_root) - 1] = '\0';
}

/* ---- vecho output ---- */

static void vecho_line(const char *message)
{
    char exe_path[MAX_PATH_LEN];
    char cmdline[MAX_CMD_LEN];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

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

/* ---- UTF-8 to ACP conversion ---- */

static int is_utf8(const char *str, int len)
{
    int i;
    int remaining;

    if (len <= 0) return 0;
    for (i = 0; i < len; i++) {
        if ((unsigned char)str[i] <= 0x7F) {
            continue;
        } else if ((unsigned char)str[i] >= 0xC0 && (unsigned char)str[i] <= 0xDF) {
            remaining = 1;
            i++;
            while (remaining > 0 && i < len) {
                if ((unsigned char)str[i] < 0x80 || (unsigned char)str[i] > 0xBF)
                    return 0;
                remaining--;
                i++;
            }
            if (remaining > 0) return 0;
            i--;
        } else if ((unsigned char)str[i] >= 0xE0 && (unsigned char)str[i] <= 0xEF) {
            remaining = 2;
            i++;
            while (remaining > 0 && i < len) {
                if ((unsigned char)str[i] < 0x80 || (unsigned char)str[i] > 0xBF)
                    return 0;
                remaining--;
                i++;
            }
            if (remaining > 0) return 0;
            i--;
        } else if ((unsigned char)str[i] >= 0xF0 && (unsigned char)str[i] <= 0xF4) {
            remaining = 3;
            i++;
            while (remaining > 0 && i < len) {
                if ((unsigned char)str[i] < 0x80 || (unsigned char)str[i] > 0xBF)
                    return 0;
                remaining--;
                i++;
            }
            if (remaining > 0) return 0;
            i--;
        } else {
            return 0;
        }
    }
    return 1;
}

static char *utf8_to_acp(const char *utf8)
{
    int wlen;
    int alen;
    wchar_t *wbuf;
    char *abuf;
    int len;

    len = (int)strlen(utf8);
    if (len == 0 || !is_utf8(utf8, len)) {
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) {
            strcpy(abuf, utf8);
        }
        return abuf;
    }

    wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wlen == 0) {
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) strcpy(abuf, utf8);
        return abuf;
    }

    wbuf = (wchar_t *)malloc((size_t)wlen * sizeof(wchar_t));
    if (!wbuf) {
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) strcpy(abuf, utf8);
        return abuf;
    }

    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, wlen);

    alen = WideCharToMultiByte(CP_ACP, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (alen == 0) {
        free(wbuf);
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) strcpy(abuf, utf8);
        return abuf;
    }

    abuf = (char *)malloc((size_t)alen);
    if (!abuf) {
        free(wbuf);
        return NULL;
    }

    WideCharToMultiByte(CP_ACP, 0, wbuf, -1, abuf, alen, NULL, NULL);
    free(wbuf);
    return abuf;
}

static void vecho_acp(const char *message)
{
    char *acp;
    acp = utf8_to_acp(message);
    if (acp) {
        vecho_line(acp);
        free(acp);
    } else {
        vecho_line(message);
    }
}

/* ---- INI reader ---- */

static char *trim_line(char *line)
{
    int len;
    char *end;

    while (*line == ' ' || *line == '\t') line++;
    len = (int)strlen(line);
    end = line + len - 1;
    while (end > line && (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return line;
}

static int read_ini_value(const char *inipath, const char *key,
                          char *value, int value_size)
{
    FILE *fp;
    char raw[MAX_LINE];
    int found;

    fp = fopen(inipath, "r");
    if (!fp) return 0;

    found = 0;
    while (fgets(raw, sizeof(raw), fp)) {
        char *line;

        line = trim_line(raw);

        if (line[0] == '[') {
            int i;
            i = 1;
            while (line[i] && line[i] != ']') i++;
            if (line[i] == ']') {
                line[i] = '\0';
                if (strcmp(line + 1, key) == 0) {
                    if (fgets(raw, sizeof(raw), fp)) {
                        char *val_line;

                        val_line = trim_line(raw);

                        strncpy(value, val_line, value_size - 1);
                        value[value_size - 1] = '\0';
                        found = 1;
                    }
                }
            }
        }

        if (found) break;
    }

    fclose(fp);
    return found;
}

static void load_lang_strings(const char *lang_dir)
{
    char path[MAX_PATH_LEN];
    char value[MAX_CMD_LEN];
    int loaded;

    snprintf(path, sizeof(path), "%s\\languext.ini", lang_dir);
    loaded = 0;

    if (read_ini_value(path, "hash_ok", value, sizeof(value))) {
        strncpy(g_hash_ok, value, sizeof(g_hash_ok) - 1);
        loaded++;
    }
    if (read_ini_value(path, "hash_fail", value, sizeof(value))) {
        strncpy(g_hash_fail, value, sizeof(g_hash_fail) - 1);
        loaded++;
    }
    if (read_ini_value(path, "hash_match", value, sizeof(value))) {
        strncpy(g_hash_match, value, sizeof(g_hash_match) - 1);
        loaded++;
    }
    if (read_ini_value(path, "hash_mismatch", value, sizeof(value))) {
        strncpy(g_hash_mismatch, value, sizeof(g_hash_mismatch) - 1);
        loaded++;
    }
    if (read_ini_value(path, "hash_gene", value, sizeof(value))) {
        strncpy(g_hash_gene, value, sizeof(g_hash_gene) - 1);
        loaded++;
    }
    if (read_ini_value(path, "hash_error_open", value, sizeof(value))) {
        strncpy(g_hash_error_open, value, sizeof(g_hash_error_open) - 1);
        loaded++;
    }
    if (read_ini_value(path, "hash_usage", value, sizeof(value))) {
        strncpy(g_hash_usage, value, sizeof(g_hash_usage) - 1);
        loaded++;
    }

    if (!loaded) {
        strncpy(g_hash_ok, "$brightgreen$OK!$write$", sizeof(g_hash_ok) - 1);
        strncpy(g_hash_fail, "$brightred$FAIL!$write$", sizeof(g_hash_fail) - 1);
        strncpy(g_hash_match, "Both file hashes are identical (match).", sizeof(g_hash_match) - 1);
        strncpy(g_hash_mismatch, "Hash verification failed - files do not match.", sizeof(g_hash_mismatch) - 1);
        strncpy(g_hash_gene, "File SHA256 hash:", sizeof(g_hash_gene) - 1);
        strncpy(g_hash_error_open, "Error: Cannot open file", sizeof(g_hash_error_open) - 1);
        strncpy(g_hash_usage, "Usage: pier hash -c/check <file1> <file2> or pier hash -g/gene <file>", sizeof(g_hash_usage) - 1);
    }
}

/* ---- Main ---- */

int main(int argc, char *argv[])
{
    const char *mode;
    const char *file1;
    const char *file2;
    const char *lang_dir;
    char hash1[65];
    char hash2[65];
    char output[MAX_CMD_LEN];
    int is_check;
    int i;

    detect_pier_root();

    if (argc < 2) {
        load_lang_strings(".");
        vecho_acp(g_hash_usage);
        return 1;
    }

    mode = argv[1];
    lang_dir = NULL;

    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
            lang_dir = argv[i + 1];
            i++;
        }
    }

    if (!lang_dir) {
        lang_dir = ".";
    }

    g_hash_ok[0] = '\0';
    g_hash_fail[0] = '\0';
    g_hash_match[0] = '\0';
    g_hash_mismatch[0] = '\0';
    g_hash_gene[0] = '\0';
    g_hash_error_open[0] = '\0';
    g_hash_usage[0] = '\0';

    load_lang_strings(lang_dir);

    is_check = (strcmp(mode, "-c") == 0 || strcmp(mode, "check") == 0);
    if (is_check) {
        if (argc < 4) {
            vecho_acp(g_hash_usage);
            return 1;
        }
        file1 = argv[2];
        file2 = argv[3];
    } else if (strcmp(mode, "-g") == 0 || strcmp(mode, "gene") == 0) {
        if (argc < 3) {
            vecho_acp(g_hash_usage);
            return 1;
        }
        file1 = argv[2];
        file2 = NULL;
    } else {
        file1 = argv[1];
        file2 = NULL;
        is_check = 0;
    }

    if (is_check && file2) {
        if (sha256_file(file1, hash1) != 0) {
            snprintf(output, sizeof(output), "%s: %s", g_hash_error_open, file1);
            vecho_acp(output);
            return 1;
        }

        if (sha256_file(file2, hash2) != 0) {
            snprintf(output, sizeof(output), "%s: %s", g_hash_error_open, file2);
            vecho_acp(output);
            return 1;
        }

        if (_strnicmp(hash1, hash2, 64) == 0) {
            vecho_acp(g_hash_ok);
            vecho_acp(g_hash_match);
            return 0;
        } else {
            vecho_acp(g_hash_fail);
            vecho_acp(g_hash_mismatch);
            return 1;
        }
    } else {
        if (sha256_file(file1, hash1) != 0) {
            snprintf(output, sizeof(output), "%s: %s", g_hash_error_open, file1);
            vecho_acp(output);
            return 1;
        }

        snprintf(output, sizeof(output), "%s %s", g_hash_gene, hash1);
        vecho_acp(output);
        return 0;
    }
}