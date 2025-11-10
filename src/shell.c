#include "shell.h"

// ====== HISTORY VARIABLES ======
char* history[HISTORY_SIZE];
int history_count = 0;

// ====== READ COMMAND ======
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

// ====== TOKENIZER ======
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

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
        printf("  history    - show command history\n");
        printf("  !n         - re-execute nth command from history\n");
        return 1;
    }

    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("No background jobs implemented yet.\n");
        return 1;
    }

    else if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    return 0;
}

// ====== HISTORY FUNCTIONS ======
void add_history(const char* cmd) {
    if (cmd == NULL || strlen(cmd) == 0)
        return;

    if (history_count == HISTORY_SIZE) {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
            history[i - 1] = history[i];
        history_count--;
    }

    history[history_count] = strdup(cmd);
    history_count++;
}

void print_history(void) {
    for (int i = 0; i < history_count; i++)
        printf("%d %s\n", i + 1, history[i]);
}

char* get_history_command(int index) {
    if (index < 1 || index > history_count)
        return NULL;
    return history[index - 1];
}
