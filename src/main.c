#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* forward helper declared in shell.h: trim_inplace, free_command, etc. */

/* process a single input line (handles !n, history, semicolons and executes segments) */
void process_input_line(char *rawline) {
    if (!rawline) return;
    trim_inplace(rawline);
    if (strlen(rawline) == 0) return;

    char *exec_line = NULL;

    if (rawline[0] == '!') {
        int n = atoi(rawline + 1);
        char *old = get_history_command(n);
        if (!old) {
            printf("No such command in history.\n");
            return;
        }
        exec_line = old; /* allocated by get_history_command */
        printf("%s\n", exec_line);
    } else {
        exec_line = strdup(rawline);
    }

    add_to_history(exec_line);

    char *saveptr = NULL;
    char *segment = strtok_r(exec_line, ";", &saveptr);
    while (segment) {
        trim_inplace(segment);
        if (strlen(segment) == 0) { segment = strtok_r(NULL, ";", &saveptr); continue; }

        Command cmd = parse_command(segment);

        if (!handle_builtin(cmd.argv)) {
            execute_command(cmd);
        }

        free_command(&cmd);
        segment = strtok_r(NULL, ";", &saveptr);
    }

    free(exec_line);
}

/* read an if..then..else..fi block starting from the current line that begins with 'if' */
void handle_if_block(char *start_line) {
    /* start_line contains the line that started with 'if' (caller passes it) */
    /* We must determine the test command, then capture lines for then/else until fi. */

    /* First extract test command after 'if' if present, else read next non-empty line */
    char *p = start_line;
    while (*p && (*p == ' ' || *p == '\t')) p++;
    /* skip 'if' */
    if (strncmp(p, "if", 2) == 0) p += 2;
    while (*p == ' ' || *p == '\t') p++;

    char *test_cmd = NULL;
    if (*p != '\0') {
        test_cmd = strdup(p);
    } else {
        /* read next non-empty line as test command */
        char *ln;
        while ((ln = read_cmd("", stdin)) != NULL) {
            trim_inplace(ln);
            if (strlen(ln) == 0) { free(ln); continue; }
            test_cmd = ln; /* already allocated by read_cmd */
            break;
        }
        if (!test_cmd) return; /* EOF */
    }

    /* Now read lines until 'fi' encountered, store lines between then...else/fi */
    char *then_lines[128];
    char *else_lines[128];
    int then_count = 0, else_count = 0;
    int in_then = 0, in_else = 0;

    while (1) {
        char *ln = read_cmd("", stdin);
        if (!ln) break; /* EOF -> bail */
        trim_inplace(ln);
        if (strcmp(ln, "then") == 0) {
            in_then = 1; in_else = 0;
            free(ln);
            continue;
        } else if (strcmp(ln, "else") == 0) {
            in_else = 1; in_then = 0;
            free(ln);
            continue;
        } else if (strcmp(ln, "fi") == 0) {
            free(ln);
            break;
        } else {
            if (in_then) {
                then_lines[then_count++] = strdup(ln);
            } else if (in_else) {
                else_lines[else_count++] = strdup(ln);
            } else {
                /* lines before 'then' that are not the test were ignored */
            }
            free(ln);
        }
    }

    /* Execute test command synchronously (force foreground) */
    trim_inplace(test_cmd);
    /* Only take first segment before any ';' for test decision */
    char *sc = strchr(test_cmd, ';');
    char *test_segment = NULL;
    if (sc) {
        *sc = '\0';
        test_segment = strdup(test_cmd);
    } else {
        test_segment = strdup(test_cmd);
    }

    Command test_parsed = parse_command(test_segment);
    test_parsed.background = 0; /* force foreground for test */
    int exit_code = 0;
    if (!handle_builtin(test_parsed.argv)) {
        int status = execute_command(test_parsed);
        if (status >= 0) exit_code = status;
        else exit_code = 1; /* treat background/error as non-zero */
    } else {
        exit_code = 0; /* builtin assumed success */
    }

    free_command(&test_parsed);
    free(test_segment);
    free(test_cmd);

    /* Choose block based on exit_code */
    if (exit_code == 0) {
        for (int i = 0; i < then_count; ++i) {
            process_input_line(then_lines[i]);
            free(then_lines[i]);
        }
        /* free else lines */
        for (int i = 0; i < else_count; ++i) free(else_lines[i]);
    } else {
        for (int i = 0; i < else_count; ++i) {
            process_input_line(else_lines[i]);
            free(else_lines[i]);
        }
        for (int i = 0; i < then_count; ++i) free(then_lines[i]);
    }
}

/* main loop */
int main() {
    char *line = NULL;

    while (1) {
        /* reap any finished background jobs (non-blocking) */
        reap_background_jobs();

        line = read_cmd(PROMPT, stdin);
        if (!line) {
            printf("\nShell exited.\n");
            break;
        }
        trim_inplace(line);
        if (strlen(line) == 0) { free(line); continue; }

        /* If line begins with "if" as a word, handle multi-line if block */
        char tmp[MAX_LEN];
        strncpy(tmp, line, MAX_LEN-1); tmp[MAX_LEN-1] = '\0';
        trim_inplace(tmp);
        char *first = strtok(tmp, " \t");
        if (first && strcmp(first, "if") == 0) {
            handle_if_block(line);
            free(line);
            continue;
        }

        /* otherwise normal single-line processing */
        process_input_line(line);
        free(line);
    }

    return 0;
}
