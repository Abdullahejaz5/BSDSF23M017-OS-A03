#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 40
#define HISTORY_SIZE 20
#define PROMPT "FCIT> "

typedef struct {
    char *argv[MAXARGS];
    char *infile;
    char *outfile;
    int has_pipe;
    char *pipe_argv[MAXARGS];
    int background; /* 1 if trailing & */
} Command;

typedef struct job {
    pid_t pid;
    char *cmdline;
    struct job *next;
} job_t;

/* core */
char *read_cmd(const char *prompt, FILE *fp);
Command parse_command(char *cmdline);
int execute_command(Command cmd); /* returns exit code for foreground, -1 for background/error */

/* builtins */
int handle_builtin(char **arglist);

/* helpers (implemented in shell.c) */
void trim_inplace(char *s);
void free_command(Command *cmd);

/* history */
void add_to_history(const char *cmd);
void show_history(void);
char *get_history_command(int n); /* returns newly allocated string or NULL */

/* jobs */
void add_job(pid_t pid, const char *cmdline);
void remove_job(pid_t pid);
void print_jobs(void);
void reap_background_jobs(void);

#endif /* SHELL_H */
