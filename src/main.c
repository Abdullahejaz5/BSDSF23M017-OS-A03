#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = readline(PROMPT)) != NULL) {
        // Skip empty inputs
        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        // Handle !n re-execution
        if (cmdline[0] == '!') {
            int n = atoi(cmdline + 1);
            HIST_ENTRY **the_list = history_list();
            if (!the_list || n <= 0 || n > history_length) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            } else {
                // Get nth history command (1-indexed)
                char* prev_cmd = the_list[n - 1]->line;
                printf("%s\n", prev_cmd);
                free(cmdline);
                cmdline = strdup(prev_cmd);
            }
        }

        // Add non-empty command to history
        if (strlen(cmdline) > 0)
            add_history(cmdline);

        // Tokenize and execute
        if ((arglist = tokenize(cmdline)) != NULL) {
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
