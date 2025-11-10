#include "shell.h"

// ==== READ COMMAND USING READLINE ====
char* read_cmd(char* prompt) {
    char* cmdline = readline(prompt);
    if (cmdline && *cmdline)
        add_history(cmdline); // Readline built-in history
    return cmdline;
}

// ==== TOKENIZER ====
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n')
        return NULL;

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0') break;
        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) len++;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

// ==== BUILT-IN COMMAND HANDLER ====
int handle_builtin(char **arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    // exit
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    // cd
    else if (strcmp(arglist[0], "cd") == 0) {
        char *path = arglist[1];
        if (path == NULL)
            path = getenv("HOME");
        if (chdir(path) != 0)
            perror("cd failed");
        return 1;
    }

    // help
    else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd [dir]     – change directory\n");
        printf("  exit         – exit shell\n");
        printf("  help         – show help menu\n");
        printf("  jobs         – show background jobs (not yet implemented)\n");
        printf("  history      – show command history\n");
        printf("  !n           – re-execute nth history command\n");
        return 1;
    }

    // jobs
    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("No background jobs implemented yet.\n");
        return 1;
    }

    // history (Readline built-in)
    else if (strcmp(arglist[0], "history") == 0) {
        HIST_ENTRY **the_list = history_list();
        if (the_list) {
            for (int i = 0; the_list[i]; i++)
                printf("%d  %s\n", i + 1, the_list[i]->line);
        } else {
            printf("No history entries.\n");
        }
        return 1;
    }

    return 0; // not a built-in
}
