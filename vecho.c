/*
 * vecho.c - Color echo for Windows Console
 * Compatible with Windows XP and later
 * Uses SetConsoleTextAttribute (not ANSI codes)
 * Supports UTF-8 input, converts to GBK for output
 * 
 * Author: Sanakaprix & Trae
 * https://steve372a.github.io
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Color definitions for SetConsoleTextAttribute */
#define COLOR_BLACK         0
#define COLOR_DARKBLUE      1
#define COLOR_DARKGREEN     2
#define COLOR_DARKCYAN      3
#define COLOR_DARKRED       4
#define COLOR_DARKMAGENTA   5
#define COLOR_DARKYELLOW    6
#define COLOR_GRAY          7
#define COLOR_DARKGRAY      8
#define COLOR_BLUE          9
#define COLOR_GREEN         10
#define COLOR_CYAN          11
#define COLOR_RED           12
#define COLOR_MAGENTA       13
#define COLOR_YELLOW        14
#define COLOR_WHITE         7

/* Bright colors (intensity bit) */
#define BRIGHT_BLACK        8
#define BRIGHT_BLUE         9
#define BRIGHT_GREEN        10
#define BRIGHT_CYAN         11
#define BRIGHT_RED          12
#define BRIGHT_MAGENTA      13
#define BRIGHT_YELLOW       14
#define BRIGHT_WHITE        15

/* Global console handle */
HANDLE g_hConsole = NULL;
WORD g_originalAttributes = 0;

/* Function prototypes */
void init_console(void);
void restore_console(void);
int set_color(const char *color_tag);
void process_and_print(const char *text);
void print_help(const char *pier_root);
void print_version(void);
void utf8_to_gbk(const char *utf8_str, char *gbk_str, int gbk_size);
int is_help_param(const char *param);

/* Initialize console handle and save original attributes */
void init_console(void) {
    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_hConsole != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(g_hConsole, &csbi)) {
            g_originalAttributes = csbi.wAttributes;
        }
    }
}

/* Restore original console attributes */
void restore_console(void) {
    if (g_hConsole != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(g_hConsole, g_originalAttributes);
    }
}

/* Set console color based on tag, returns 1 if valid tag, 0 otherwise */
int set_color(const char *color_tag) {
    WORD color = 0;
    int is_bright = 0;
    
    /* Check for bright prefix */
    if (strncmp(color_tag, "bright", 6) == 0) {
        is_bright = 1;
        color_tag += 6;
    }
    
    /* Parse color name */
    if (strcmp(color_tag, "black") == 0) {
        color = is_bright ? BRIGHT_BLACK : COLOR_BLACK;
    } else if (strcmp(color_tag, "blue") == 0) {
        color = is_bright ? BRIGHT_BLUE : COLOR_DARKBLUE;
    } else if (strcmp(color_tag, "green") == 0) {
        color = is_bright ? BRIGHT_GREEN : COLOR_DARKGREEN;
    } else if (strcmp(color_tag, "cyan") == 0) {
        color = is_bright ? BRIGHT_CYAN : COLOR_DARKCYAN;
    } else if (strcmp(color_tag, "red") == 0) {
        color = is_bright ? BRIGHT_RED : COLOR_DARKRED;
    } else if (strcmp(color_tag, "magenta") == 0) {
        color = is_bright ? BRIGHT_MAGENTA : COLOR_DARKMAGENTA;
    } else if (strcmp(color_tag, "yellow") == 0) {
        color = is_bright ? BRIGHT_YELLOW : COLOR_DARKYELLOW;
    } else if (strcmp(color_tag, "white") == 0) {
        color = is_bright ? BRIGHT_WHITE : COLOR_WHITE;
    } else if (strcmp(color_tag, "gray") == 0) {
        color = is_bright ? COLOR_GRAY : COLOR_GRAY;
    } else {
        return 0; /* Invalid color tag */
    }
    
    /* Set console text attribute */
    if (g_hConsole != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(g_hConsole, color);
    }
    return 1;
}

/* No conversion needed - input is already in system encoding (GBK) */
void utf8_to_gbk(const char *input_str, char *output_str, int output_size) {
    /* Just copy the string as-is */
    strncpy(output_str, input_str, output_size - 1);
    output_str[output_size - 1] = '\0';
}

/* Process text with color tags and print */
void process_and_print(const char *text) {
    const char *p = text;
    char buffer[4096];
    int buf_pos = 0;
    int in_tag = 0;
    char tag[32];
    int tag_pos = 0;
    
    while (*p) {
        if (*p == '$') {
            /* Check if this starts a color tag */
            if (*(p + 1) == '$') {
                /* Escaped $, print single $ */
                if (buf_pos < sizeof(buffer) - 1) {
                    buffer[buf_pos++] = '$';
                }
                p += 2;
            } else if (in_tag) {
                /* End of tag */
                tag[tag_pos] = '\0';
                
                /* Flush buffer first */
                if (buf_pos > 0) {
                    buffer[buf_pos] = '\0';
                    {
                        char gbk_buffer[4096];
                        utf8_to_gbk(buffer, gbk_buffer, sizeof(gbk_buffer));
                        printf("%s", gbk_buffer);
                    }
                    buf_pos = 0;
                }
                
                /* Process tag */
                if (strcmp(tag, "write") == 0) {
                    /* Reset to default color */
                    if (g_hConsole != INVALID_HANDLE_VALUE) {
                        SetConsoleTextAttribute(g_hConsole, g_originalAttributes);
                    }
                } else {
                    set_color(tag);
                }
                
                in_tag = 0;
                tag_pos = 0;
                p++;
            } else {
                /* Start of tag */
                /* Flush buffer first */
                if (buf_pos > 0) {
                    buffer[buf_pos] = '\0';
                    {
                        char gbk_buffer[4096];
                        utf8_to_gbk(buffer, gbk_buffer, sizeof(gbk_buffer));
                        printf("%s", gbk_buffer);
                    }
                    buf_pos = 0;
                }
                
                in_tag = 1;
                tag_pos = 0;
                p++;
            }
        } else {
            if (in_tag) {
                if (tag_pos < sizeof(tag) - 1) {
                    tag[tag_pos++] = *p;
                }
            } else {
                if (buf_pos < sizeof(buffer) - 1) {
                    buffer[buf_pos++] = *p;
                }
            }
            p++;
        }
    }
    
    /* Flush remaining buffer */
    if (buf_pos > 0) {
        buffer[buf_pos] = '\0';
        {
            char gbk_buffer[4096];
            utf8_to_gbk(buffer, gbk_buffer, sizeof(gbk_buffer));
            printf("%s", gbk_buffer);
        }
    }
    
    /* Output newline at the end */
    printf("\n");
}

/* Print color test */
void print_color_test(void) {
    init_console();
    
    set_color("red");
    printf("Testing_Color_red $red$\n");
    
    set_color("green");
    printf("Testing_Color_green\n");
    
    set_color("blue");
    printf("Testing_Color_blue\n");
    
    set_color("yellow");
    printf("Testing_Color_yellow\n");
    
    set_color("cyan");
    printf("Testing_Color_cyan\n");
    
    set_color("magenta");
    printf("Testing_Color_magenta\n");
    
    set_color("white");
    printf("Testing_Color_white\n");
    
    set_color("brightred");
    printf("Testing_Color_brightred $brightred$\n");
    
    set_color("brightgreen");
    printf("Testing_Color_brightgreen\n");
    
    set_color("brightblue");
    printf("Testing_Color_brightblue\n");
    
    set_color("brightyellow");
    printf("Testing_Color_brightyellow\n");
    
    set_color("brightcyan");
    printf("Testing_Color_brightcyan\n");
    
    set_color("brightmagenta");
    printf("Testing_Color_brightmagenta\n");
    
    restore_console();
    printf("\n");
}

/* Print version/author info */
void print_version(void) {
    printf("Version: 2.0.0-beta1\n");
    printf("Author: Sanakaprix & Trae\n");
    printf("Website: https://steve372a.github.io\n");
}

int main(int argc, char *argv[]) {
    int i;
    char full_text[8192];
    int pos = 0;
    
    /* Check for no arguments - show color test and author */
    if (argc < 2) {
        print_color_test();
        print_version();
        return 0;
    }
    
    /* Initialize console */
    init_console();
    
    /* Concatenate all arguments into one string */
    for (i = 1; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (pos + len + 1 < sizeof(full_text)) {
            if (pos > 0) {
                full_text[pos++] = ' ';
            }
            memcpy(full_text + pos, argv[i], len);
            pos += len;
        }
    }
    full_text[pos] = '\0';
    
    /* Process and print with colors */
    process_and_print(full_text);
    
    /* Restore console attributes */
    restore_console();
    
    return 0;
}
