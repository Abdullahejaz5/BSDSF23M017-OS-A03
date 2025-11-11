#include "shell.h"

/* forward */
void process_input_line(char *line);

/* Read a non-empty or empty line depending on user input (wrapper already in shell.c) */

/* handle multi-line if ... then ... else ... fi
   Input: the original line that started with 'if' (may include test after 'if' or just "if")
*/
void handle_if_block_from_line(const char *if_line) {
    char *test_line = NULL;

    if (if_line && strlen(if_line) > 2) {
        const char *p = if_line + 2;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p) test_line = strdup(p);
    }

    if (!test_line) {
        while (1) {
            char *ln = read_cmd("if> ", stdin);
            if (!ln) return;
            trim_inplace(ln);
            if (strlen(ln) == 0) { free(ln); continue; }
            test_line = ln;
            break;
        }
    }

    /* collect then/else */
    char *then_lines[256]; int then_n = 0;
    char *else_lines[256]; int else_n = 0;
    int in_then = 0, in_else = 0;
    while (1) {
        char *ln = read_cmd("if> ", stdin);
        if (!ln) break;
        trim_inplace(ln);
        if (strcmp(ln, "then") == 0) { in_then = 1; in_else = 0; free(ln); continue; }
        if (strcmp(ln, "else") == 0) { in_else = 1; in_then = 0; free(ln); continue; }
        if (strcmp(ln, "fi") == 0) { free(ln); break; }
        if (in_then) then_lines[then_n++] = strdup(ln);
        else if (in_else) else_lines[else_n++] = strdup(ln);
        free(ln);
    }

    /* run test */
    char *tcopy = strdup(test_line);
    Command test_cmd = parse_command(tcopy);
    free(tcopy);
    if (test_cmd.argv[0]) expand_command_tokens(&test_cmd);

    int test_status = 1;
    if (test_cmd.argv[0]) {
        if (!handle_builtin(test_cmd.argv)) {
            int rc = execute_command(test_cmd);
            if (rc >= 0) test_status = rc;
            else test_status = 1;
            /* execute_command does not free test_cmd internals in our design,
               but we must free them here since caller owns them */
            free_command(&test_cmd);
        } else {
            /* builtin handled; free parsed strings */
            free_command(&test_cmd);
            test_status = 0;
        }
    }

    free(test_line);

    if (test_status == 0) {
        for (int i = 0; i < then_n; ++i) { process_input_line(then_lines[i]); free(then_lines[i]); }
        for (int i = 0; i < else_n; ++i) free(else_lines[i]);
    } else {
        for (int i = 0; i < else_n; ++i) { process_input_line(else_lines[i]); free(else_lines[i]); }
        for (int i = 0; i < then_n; ++i) free(then_lines[i]);
    }
}

/* process one line: handle !n, assignment, ; chaining, builtins, exec */
void process_input_line(char *line_orig) {
    if (!line_orig) return;
    trim_inplace(line_orig);
    if (strlen(line_orig) == 0) return;

    if (strncmp(line_orig, "if", 2) == 0 && (line_orig[2] == ' ' || line_orig[2] == '\0')) {
        handle_if_block_from_line(line_orig);
        return;
    }

    if (line_orig[0] == '!') {
        int n = atoi(line_orig + 1);
        char *old = get_history_command(n);
        if (!old) { printf("No such command in history.\n"); return; }
        printf("%s\n", old);
        process_input_line(old);
        free(old);
        return;
    }

    /* variable assignment */
    if (handle_variable_assignment(line_orig)) return;

    add_to_history(line_orig);

    /* split by ; */
    char *buf = strdup(line_orig); /* mutated by strtok_r */
    char *saveptr = NULL;
    char *seg = strtok_r(buf, ";", &saveptr);
    while (seg) {
        trim_inplace(seg);
        if (strlen(seg) == 0) { seg = strtok_r(NULL, ";", &saveptr); continue; }

        Command cmd = parse_command(seg);

        /* expand variables */
        expand_command_tokens(&cmd);

        if (!handle_builtin(cmd.argv)) {
            execute_command(cmd);
            /* caller frees parsed memory */
            free_command(&cmd);
        } else {
            /* builtin handled -> free */
            free_command(&cmd);
        }

        seg = strtok_r(NULL, ";", &saveptr);
    }
    free(buf);
}

/* main */
int main(void) {
    char *line = NULL;
    while (1) {
        reap_background_jobs();
        line = read_cmd(PROMPT, stdin);
        if (!line) {
            printf("\nShell exited.\n");
            break;
        }
        process_input_line(line);
        free(line);
    }
    /* optional: free vars/history/jobs lists - omitted for brevity */
    return 0;
}
