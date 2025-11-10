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
#define MAXARGS 20
#define HISTORY_SIZE 20
#define PROMPT "FCIT> "

typedef struct {
    char *argv[MAXARGS];
    char *infile;
    char *outfile;
    int has_pipe;
    char *pipe_argv[MAXARGS];
} Command;

// core
char *read_cmd(char *prompt, FILE *fp);
Command parse(char *cmdline);
int execute(Command cmd);
int handle_builtin(char **arglist);

// history
void add_to_history(const char *cmd);
void show_history(void);
char *get_history_command(int n);

#endif
