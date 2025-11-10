#include "shell.h"

static void run_redirect(Command cmd) {
    int fd;
    if (cmd.outfile) {
        fd = open(cmd.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { perror("open"); exit(1); }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    if (cmd.infile) {
        fd = open(cmd.infile, O_RDONLY);
        if (fd < 0) { perror("open"); exit(1); }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    execvp(cmd.argv[0], cmd.argv);
    perror("execvp");
    exit(1);
}

static void run_pipe(Command cmd) {
    int fd[2];
    pipe(fd);

    pid_t p1 = fork();
    if (p1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]); close(fd[1]);
        execvp(cmd.argv[0], cmd.argv);
        perror("execvp left");
        exit(1);
    }

    pid_t p2 = fork();
    if (p2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]); close(fd[1]);
        execvp(cmd.pipe_argv[0], cmd.pipe_argv);
        perror("execvp right");
        exit(1);
    }

    close(fd[0]); close(fd[1]);
    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);
}

int execute(Command cmd) {
    if (cmd.argv[0] == NULL) return 0;

    if (handle_builtin(cmd.argv)) return 0;

    if (cmd.has_pipe) {
        run_pipe(cmd);
        return 0;
    }

    pid_t pid = fork();
    if (pid == 0) run_redirect(cmd);
    waitpid(pid, NULL, 0);
    return 0;
}
