#ifndef SHELL_H
#define SHELL_H

/* standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LEN 1024
#define MAXARGS 64
#define HISTORY_SIZE 100
#define PROMPT "FCIT> "

/* Command structure */
typedef struct {
    char *argv[MAXARGS];      /* argv for this command */
    char *infile;             /* input redirection */
    char *outfile;            /* output redirection */
    int has_pipe;             /* 0 or 1 */
    char *pipe_argv[MAXARGS]; /* argv for right side of a single pipe */
    int background;           /* 0 or 1 */
} Command;

/* core helpers */
char *read_cmd(const char *prompt, FILE *fp);
Command parse_command(char *cmdline);
void free_command(Command *cmd);

/* builtins & execution */
int handle_builtin(char **argv);       /* returns 1 if builtin handled */
int execute_command(Command cmd);      /* returns exit code (>=0) or -1 if background/error */

/* history */
void add_to_history(const char *cmd);
void show_history(void);
char *get_history_command(int n); /* returns malloc'd string or NULL */

/* jobs (background management) */
void add_job(pid_t pid, const char *cmdline);
void remove_job(pid_t pid);
void print_jobs(void);
void reap_background_jobs(void);

/* variables (feature 8) */
void set_var(const char *name, const char *value);
char *get_var(const char *name); /* returns internal pointer (do NOT free) */
void print_vars(void);
int handle_variable_assignment(const char *line); /* returns 1 if handled */
void expand_command_tokens(Command *cmd);

/* helpers */
void trim_inplace(char *s);

/* allow shell.c to call main's process function (prototype) */
void process_input_line(char *line);

#endif /* SHELL_H */
