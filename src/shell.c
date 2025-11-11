#include "shell.h"

/* ---------------- history ---------------- */
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
    return strdup(history[n-1]);
}

/* ---------------- jobs ---------------- */
typedef struct job {
    pid_t pid;
    char *cmdline;
    struct job *next;
} job_t;

static job_t *job_head = NULL;

void add_job(pid_t pid, const char *cmdline) {
    job_t *j = malloc(sizeof(job_t));
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
            *p = tmp->next;
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

void reap_background_jobs(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid);
    }
}

/* ---------------- read_cmd ---------------- */
char *read_cmd(const char *prompt, FILE *fp) {
    if (prompt && *prompt) {
        printf("%s", prompt);
        fflush(stdout);
    }
    char *buf = malloc(MAX_LEN);
    if (!buf) return NULL;
    if (!fgets(buf, MAX_LEN, fp)) {
        free(buf);
        return NULL;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    return buf;
}

/* ---------------- trimming and parsing ---------------- */
void trim_inplace(char *s) {
    if (!s) return;
    char *p = s;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n')) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    int len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n')) {
        s[len-1] = '\0'; len--;
    }
}

/* parse a single command (no outer ';' splitting) */
Command parse_command(char *cmdline) {
    Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.infile = NULL; cmd.outfile = NULL; cmd.has_pipe = 0; cmd.background = 0;

    if (!cmdline) return cmd;
    trim_inplace(cmdline);
    if (strlen(cmdline) == 0) return cmd;

    /* handle trailing & */
    size_t len = strlen(cmdline);
    if (len >= 1 && cmdline[len-1] == '&') {
        cmd.background = 1;
        cmdline[len-1] = '\0';
        trim_inplace(cmdline);
    }

    char *saveptr = NULL;
    char *token = strtok_r(cmdline, " \t", &saveptr);
    int argc = 0, parc = 0;
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
                if (parc < MAXARGS-1) cmd.pipe_argv[parc++] = strdup(token);
            }
        }
        token = strtok_r(NULL, " \t", &saveptr);
    }
    cmd.argv[argc] = NULL;
    cmd.pipe_argv[parc] = NULL;
    return cmd;
}

/* free just the heap parts of a Command (caller must call when appropriate) */
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

/* ---------------- builtins ---------------- */
int handle_builtin(char **arglist) {
    if (!arglist || !arglist[0]) return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    } else if (strcmp(arglist[0], "cd") == 0) {
        if (!arglist[1]) {
            char *home = getenv("HOME"); if (!home) home = "/";
            if (chdir(home) != 0) perror("cd");
        } else {
            if (chdir(arglist[1]) != 0) perror("cd");
        }
        return 1;
    }else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-ins:\n");
        printf("  cd      - Change directory\n");
        printf("  exit    - Exit shell\n");
        printf("  help    - Show this help message\n");
        printf("  history - Show history\n");
        printf("  jobs    - Show background jobs\n");
        printf("  set     - Show all shell variables (Feature-8)\n");
        return 1;
    }
     else if (strcmp(arglist[0], "history") == 0) {
        show_history();
        return 1;
    } else if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    } else if (strcmp(arglist[0], "set") == 0) {
        print_vars();
        return 1;
    }

    return 0;}

