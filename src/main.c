#include "shell.h"

int main() {
    char *cmdline;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        if (strlen(cmdline) == 0) { free(cmdline); continue; }

        // Handle !n re-execution
        if (cmdline[0] == '!') {
            int n = atoi(cmdline + 1);
            char *old = get_history_command(n);
            if (!old) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = old;
            printf("%s\n", cmdline);
        }

        add_to_history(cmdline);

        Command cmd = parse(cmdline);
        execute(cmd);

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
