#include "shell.h"

/* ---------------- History ---------------- */
static char *history[HISTORY_SIZE];
static int history_count = 0;

void add_to_history(const char *cmd) {
    if (!cmd) return;
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmd);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; ++i) history[i-1] = history[i];
        history[HISTORY_SIZE-1] = strdup(cmd);
    }
}

void show_history(void) {
    for (int i = 0; i < history_count; ++i) {
        printf("%d %s\n", i+1, history[i]);
    }
}

char *get_history_command(int n) {
    if (n <= 0 || n > history_count) return NULL;
    return strdup(history[n-1]); /* caller must free */
}

/* ---------------- Jobs (linked list) ---------------- */
static job_t *job_head = NULL;

void add_job(pid_t pid, const char *cmdline) {
    job_t *j = malloc(sizeof(job_t));
    if (!j) return;
    j->pid = pid;
    j->cmdline = strdup(cmdline ? cmdline : "");
    j->next = job_head;
    job_head = j;
}

void remove_job(pid_t pid) {
    job_t **p = &job_head;
    while (*p) {
        if ((*p)->pid == pid) {
            job_t *tmp = *p;
            *p = (*p)->next;
            free(tmp->cmdline);
            free(tmp);
            return;
        }
        p = &((*p)->next);
    }
}

void print_jobs(void) {
    job_t *p = job_head;
    while (p) {
        printf("[%d] %s\n", p->pid, p->cmdline);
        p = p->next;
    }
}

/* reap completed background jobs (non-blocking) */
void reap_background_jobs(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid);
        /* optional notification:
           printf("Job %d finished\n", pid);
        */
    }
}

/* ---------------- Read command ---------------- */
char *read_cmd(const char *prompt, FILE *fp) {
    if (prompt) printf("%s", prompt);
    fflush(stdout);

    char *buf = malloc(MAX_LEN);
    if (!buf) return NULL;
    int pos = 0, c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') break;
        if (pos < MAX_LEN - 1) buf[pos++] = (char)c;
    }
    if (c == EOF && pos == 0) {
        free(buf);
        return NULL;
    }
    buf[pos] = '\0';
    return buf;
}

/* ---------------- Parser ----------------
   parse_command: fills Command for one command string (no semicolon splitting here)
   Recognizes <, >, | and trailing &.
*/
Command parse_command(char *cmdline) {
    Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.infile = NULL;
    cmd.outfile = NULL;
    cmd.has_pipe = 0;
    cmd.background = 0;

    if (!cmdline) return cmd;

    /* trim leading/trailing whitespace */
    while (*cmdline && (*cmdline == ' ' || *cmdline == '\t')) cmdline++;
    size_t len = strlen(cmdline);
    while (len > 0 && (cmdline[len-1] == ' ' || cmdline[len-1] == '\t')) {
        cmdline[len-1] = '\0';
        len--;
    }
    if (len == 0) return cmd;

    /* detect trailing & */
    if (len >= 1 && cmdline[len-1] == '&') {
        cmd.background = 1;
        cmdline[len-1] = '\0';
        len = strlen(cmdline);
        while (len > 0 && (cmdline[len-1] == ' ' || cmdline[len-1] == '\t')) {
            cmdline[len-1] = '\0';
            len--;
        }
    }

    char *saveptr = NULL;
    char *token = strtok_r(cmdline, " \t", &saveptr);
    int argc = 0, pipe_argc = 0;
    int seen_pipe = 0;

    while (token) {
        if (strcmp(token, "<") == 0) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (token) cmd.infile = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (token) cmd.outfile = strdup(token);
        } else if (strcmp(token, "|") == 0) {
            seen_pipe = 1;
            cmd.has_pipe = 1;
        } else {
            if (!seen_pipe) {
                if (argc < MAXARGS-1) cmd.argv[argc++] = strdup(token);
            } else {
                if (pipe_argc < MAXARGS-1) cmd.pipe_argv[pipe_argc++] = strdup(token);
            }
        }
        token = strtok_r(NULL, " \t", &saveptr);
    }
    cmd.argv[argc] = NULL;
    cmd.pipe_argv[pipe_argc] = NULL;
    return cmd;
}

/* ---------------- Helpers ---------------- */
void trim_inplace(char *s) {
    if (!s) return;
    /* leading */
    char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    /* trailing */
    int len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n')) {
        s[len-1] = '\0'; len--;
    }
}

void free_command(Command *cmd) {
    if (!cmd) return;
    for (int i = 0; i < MAXARGS && cmd->argv[i]; ++i) {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }
    for (int i = 0; i < MAXARGS && cmd->pipe_argv[i]; ++i) {
        free(cmd->pipe_argv[i]);
        cmd->pipe_argv[i] = NULL;
    }
    if (cmd->infile) { free(cmd->infile); cmd->infile = NULL; }
    if (cmd->outfile) { free(cmd->outfile); cmd->outfile = NULL; }
}

/* ---------------- Built-ins ----------------
   Returns 1 if handled, 0 otherwise.
*/
int handle_builtin(char **arglist) {
    if (!arglist || !arglist[0]) return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    } else if (strcmp(arglist[0], "cd") == 0) {
        if (!arglist[1]) {
            char *home = getenv("HOME");
            if (!home) home = "/";
            if (chdir(home) != 0) perror("cd");
        } else {
            if (chdir(arglist[1]) != 0) perror("cd");
        }
        return 1;
    } else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-ins: cd, exit, help, history, jobs\n");
        return 1;
    } else if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 1;
    } else if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }
    return 0;
}
