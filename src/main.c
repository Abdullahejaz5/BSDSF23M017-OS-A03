#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

// Completion callback (required)
char **my_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 0;
    return rl_completion_matches(text, rl_filename_completion_function);
}

int main() {
    char* cmdline;
    char** arglist;

    // Enable history
    using_history();

    // Enable tab completion
    rl_attempted_completion_function = my_completion;

    while ((cmdline = readline(PROMPT)) != NULL) {

        if (strlen(cmdline) == 0) {
            free(cmdline);
            continue;
        }

        // Handle !n
        if (cmdline[0] == '!') {
            int n = atoi(cmdline + 1);
            HIST_ENTRY **list = history_list();

            if (n <= 0 || n > history_length || !list) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }

            char* prev = list[n - 1]->line;
            printf("%s\n", prev);
            free(cmdline);
            cmdline = strdup(prev);
        }

        // Add to history
        add_history(cmdline);

        // Tokenize
        arglist = tokenize(cmdline);

        if (arglist != NULL) {
            if (!handle_builtin(arglist))
                execute(arglist);

            for (int i = 0; arglist[i] != NULL; i++)
                free(arglist[i]);
            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
