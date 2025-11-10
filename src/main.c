#include "shell.h"

int main() {
    char *cmdline;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        check_jobs();
        if (strlen(cmdline) == 0) { free(cmdline); continue; }

        // split on ';' for command chaining
        char *saveptr;
        char *cmdpart = strtok_r(cmdline, ";", &saveptr);

        while (cmdpart != NULL) {
            while (*cmdpart == ' ') cmdpart++;              // skip spaces
            if (strlen(cmdpart) == 0) {                     // empty command
                cmdpart = strtok_r(NULL, ";", &saveptr);
                continue;
            }

            // --- handle !n safely ---
            char *exec_line = NULL;
            if (cmdpart[0] == '!') {
                int n = atoi(cmdpart + 1);
                char *old = get_history_command(n);
                if (!old) {
                    printf("No such command.\n");
                    cmdpart = strtok_r(NULL, ";", &saveptr);
                    continue;
                }
                exec_line = old;            // newly malloc'd by get_history_command
                printf("%s\n", exec_line);
            } else {
                exec_line = strdup(cmdpart); // make a private copy
            }

            add_to_history(exec_line);

            Command cmd = parse(exec_line);
            execute(cmd);

            free(exec_line); // free once only
            cmdpart = strtok_r(NULL, ";", &saveptr);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
