#ifndef SQUE_EVAL_H
#define SQUE_EVAL_H

typedef struct {
    char systemver[32];
    char os[64];
    char version[64];
    char packagesize[64];
} SqueEvalContext;

/* ---- System Version Detection ---- */

/* Get Windows version string ("xp","7","8","8.1","10","11") using GetVersionExA.
 * XP compatible. Returns length or 0 on failure. */
int sque_get_systemver(char *buf, int buf_size);

/* ---- Conditional Line Parser ---- */

/* Parse a single-line conditional: if <cond> then <action>[else <action>];
 * Case-insensitive for keywords, variable names, and quoted comparison values.
 * Read-only variables: {systemver}, {os} - cannot be assigned.
 * Writable variables: {version}, {packagesize} - can be assigned.
 * Returns 1 if condition matched and action was executed,
 *          0 if skipped or parse error (ctx unchanged). */
int sque_eval_line(const char *line, SqueEvalContext *ctx);

/* ---- Variable Interpolation ---- */

/* Replace {var} placeholders in input with values from ctx.
 * Case-insensitive variable matching.
 * Supports: {systemver}, {os}, {version}, {packagesize}
 * Unknown placeholders are left as-is.
 * Returns length of output, or -1 if output truncated. */
int sque_interpolate(const char *input, const SqueEvalContext *ctx,
                     char *output, int out_size);

/* Run all conditional 'if' lines inside [Script] sections of a .sque file.
 * Executes lines in order; later lines see context modified by earlier ones.
 * Only the [Script] section(s) are processed; all other sections are skipped.
 * Returns number of lines executed (including those that didn't match). */
int sque_eval_script(const char *filepath, SqueEvalContext *ctx);

#endif