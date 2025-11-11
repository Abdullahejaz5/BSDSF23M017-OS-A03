#include "shell.h"

/* Note: execute_command() does NOT free the Command structure.
   Caller must call free_command(&cmd) when appropriate. */

/* child runner for simple command (with possible redirection) */
static void child_run_redirect(Command *pcmd) {
    Command cmd = *pcmd; /* copy small struct (pointers) */
    if (cmd.infile) {
        int fd = open(cmd.infile, O_RDONLY);
        if (fd < 0) { perror("open infile"); exit(127); }
        if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2"); close(fd); exit(127); }
        close(fd);
    }
    if (cmd.outfile) {
        int fd = open(cmd.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) { perror("open outfile"); exit(127); }
        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); close(fd); exit(127); }
        close(fd);
    }
    execvp(cmd.argv[0], cmd.argv);
    perror("execvp");
    exit(127);
}

/* two-side pipeline (left | right) */
static int run_pipeline(Command *pcmd) {
    Command cmd = *pcmd;
    int fds[2];
    if (pipe(fds) < 0) { perror("pipe"); return -1; }

    pid_t p1 = fork();
    if (p1 < 0) { perror("fork"); close(fds[0]); close(fds[1]); return -1; }
    if (p1 == 0) {
        if (dup2(fds[1], STDOUT_FILENO) < 0) { perror("dup2"); exit(127); }
        close(fds[0]); close(fds[1]);
        if (cmd.infile) {
            int fd = open(cmd.infile, O_RDONLY);
            if (fd < 0) { perror("open infile"); exit(127); }
            dup2(fd, STDIN_FILENO); close(fd);
        }
        execvp(cmd.argv[0], cmd.argv);
        perror("execvp left");
        exit(127);
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror("fork"); return -1; }
    if (p2 == 0) {
        if (dup2(fds[0], STDIN_FILENO) < 0) { perror("dup2"); exit(127); }
        close(fds[1]); close(fds[0]);
        if (cmd.outfile) {
            int fd = open(cmd.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("open outfile"); exit(127); }
            dup2(fd, STDOUT_FILENO); close(fd);
        }
        execvp(cmd.pipe_argv[0], cmd.pipe_argv);
        perror("execvp right");
        exit(127);
    }

    close(fds[0]); close(fds[1]);

    if (!pcmd->background) {
        int status;
        waitpid(p1, NULL, 0);
        waitpid(p2, &status, 0);
        if (WIFEXITED(status)) return WEXITSTATUS(status);
        return -1;
    } else {
        add_job(p2, "pipeline (background)");
        printf("[%d] pipeline started\n", p2);
        return -1;
    }
}

int execute_command(Command cmd) {
    if (!cmd.argv[0]) return 0;

    /* handle pipe */
    if (cmd.has_pipe) {
        int rc = run_pipeline(&cmd);
        return rc >= 0 ? rc : -1;
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return -1; }
    if (pid == 0) {
        child_run_redirect(&cmd);
        exit(127);
    } else {
        if (cmd.background) {
            add_job(pid, cmd.argv[0] ? cmd.argv[0] : "background");
            printf("[%d] started\n", pid);
            return -1;
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            return -1;
        }
    }
}
