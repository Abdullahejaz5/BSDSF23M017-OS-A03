#include "shell.h"

// ====== READLINE INPUT FUNCTION ======
char* read_cmd(char* prompt) {
    char* input = readline(prompt);
    if (input == NULL) // Ctrl+D pressed
        return NULL;

    if (strlen(input) > 0)
        add_history(input);  // readline’s own history

    return input;
}

// ====== TOKENIZER ======
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
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
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

// ====== HANDLE BUILT-IN COMMANDS ======
int handle_builtin(char **arglist) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    else if (strcmp(arglist[0], "cd") == 0) {
        char *path = arglist[1];
        if (path == NULL)
            path = getenv("HOME");
        if (chdir(path) != 0)
            perror("cd failed");
        return 1;
    }

    else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  cd [dir]   - change directory\n");
        printf("  exit       - exit the shell\n");
        printf("  help       - display this help\n");
        printf("  jobs       - list background jobs\n");
        printf("  history    - show command history (readline)\n");
        return 1;
    }

    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("No background jobs implemented yet.\n");
        return 1;
    }
else if (strcmp(arglist[0], "history") == 0) {
    HIST_ENTRY **the_list = history_list();
    if (the_list) {
        for (int i = 0; the_list[i]; i++) {
            printf("%d  %s\n", i + 1, the_list[i]->line);
        }
    } else {
        printf("No history entries.\n");
    }
    return 1;
}

    return 0;
}
