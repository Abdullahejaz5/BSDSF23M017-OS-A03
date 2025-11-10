#include "shell.h"

static char *history[HISTORY_SIZE];
static int history_count = 0;

void add_to_history(const char *cmd) {
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmd);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
            history[i - 1] = history[i];
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

void show_history(void) {
    for (int i = 0; i < history_count; i++)
        printf("%d %s\n", i + 1, history[i]);
}

char *get_history_command(int n) {
    if (n <= 0 || n > history_count) return NULL;
    return strdup(history[n - 1]);
}

char *read_cmd(char *prompt, FILE *fp) {
    printf("%s", prompt);
    char *cmdline = malloc(MAX_LEN);
    int c, pos = 0;
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }
    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL;
    }
    cmdline[pos] = '\0';
    return cmdline;
}

Command parse(char *cmdline) {
    Command cmd = {0};
    cmd.infile = NULL;
    cmd.outfile = NULL;
    cmd.has_pipe = 0;

    int argc = 0, pipe_argc = 0;
    char *token = strtok(cmdline, " ");

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            cmd.infile = token ? strdup(token) : NULL;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            cmd.outfile = token ? strdup(token) : NULL;
        } else if (strcmp(token, "|") == 0) {
            cmd.has_pipe = 1;
        } else if (cmd.has_pipe == 0) {
            cmd.argv[argc++] = strdup(token);
        } else {
            cmd.pipe_argv[pipe_argc++] = strdup(token);
        }
        token = strtok(NULL, " ");
    }
    cmd.argv[argc] = NULL;
    cmd.pipe_argv[pipe_argc] = NULL;
    return cmd;
}

int handle_builtin(char **arglist) {
    if (arglist[0] == NULL) return 1;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    } else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL)
            chdir(getenv("HOME"));
        else if (chdir(arglist[1]) != 0)
            perror("cd failed");
        return 1;
    } else if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 1;
    }
    return 0;
}
