#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        // Handle !n before adding to history
        if (cmdline[0] == '!') {
            int n = atoi(cmdline + 1);
            char* prev_cmd = get_history_command(n);
            if (prev_cmd == NULL) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            } else {
                printf("%s\n", prev_cmd);
                free(cmdline);
                cmdline = strdup(prev_cmd);
            }
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            // Add to history
            add_history(cmdline);

            // Built-in or external execution
            if (!handle_builtin(arglist))
                execute(arglist);

            // Free memory
            for (int i = 0; arglist[i] != NULL; i++)
                free(arglist[i]);
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
