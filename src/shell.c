#include "shell.h"

static char *history[HISTORY_SIZE];
static int history_count = 0;

static Job jobs[MAX_JOBS];
static int job_count = 0;

// ---------- History ----------
void add_to_history(const char *cmd) {
    if (history_count < HISTORY_SIZE)
        history[history_count++] = strdup(cmd);
    else {
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

// ---------- Jobs ----------
void add_job(pid_t pid, const char *cmdline) {
    if (job_count < MAX_JOBS) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].cmdline, cmdline, MAX_LEN);
        job_count++;
    }
}
void check_jobs(void) {
    int status;
    pid_t pid;
    for (int i = 0; i < job_count; ) {
        pid = waitpid(jobs[i].pid, &status, WNOHANG);
        if (pid > 0) {
            printf("[Done] PID %d: %s\n", jobs[i].pid, jobs[i].cmdline);
            for (int j = i + 1; j < job_count; j++)
                jobs[j - 1] = jobs[j];
            job_count--;
        } else i++;
    }
}
void show_jobs(void) {
    for (int i = 0; i < job_count; i++)
        printf("[%d] PID %d %s\n", i + 1, jobs[i].pid, jobs[i].cmdline);
}

// ---------- Reading ----------
char *read_cmd(char *prompt, FILE *fp) {
    printf("%s", prompt);
    char *cmdline = malloc(MAX_LEN);
    int c, pos = 0;
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }
    if (c == EOF && pos == 0) { free(cmdline); return NULL; }
    cmdline[pos] = '\0';
    return cmdline;
}

// ---------- Parser ----------
Command parse(char *cmdline) {
    Command cmd = {0};
    cmd.infile = NULL; cmd.outfile = NULL;
    cmd.has_pipe = 0; cmd.background = 0;

    int argc = 0, pipe_argc = 0;
    char *token = strtok(cmdline, " ");

    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " "); if (token) cmd.infile = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " "); if (token) cmd.outfile = strdup(token);
        } else if (strcmp(token, "|") == 0) {
            cmd.has_pipe = 1;
        } else if (strcmp(token, "&") == 0) {
            cmd.background = 1;
        } else if (!cmd.has_pipe) {
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

// ---------- Built-ins ----------
int handle_builtin(char **arglist) {
    if (!arglist[0]) return 1;
    if (strcmp(arglist[0], "exit") == 0) { printf("Shell exited.\n"); exit(0); }
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1]) chdir(arglist[1]);
        else chdir(getenv("HOME"));
        return 1;
    }
    if (strcmp(arglist[0], "history") == 0) { show_history(); return 1; }
    if (strcmp(arglist[0], "jobs") == 0) { show_jobs(); return 1; }
    return 0;
}
