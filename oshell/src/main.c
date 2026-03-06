#include "oshell.h"

int last_status = 0;

void sigint_handler(int signo) {
    (void)signo;
    // Write newline and prompt again
    write(STDOUT_FILENO, "\n$ ", 3);
}

void loop() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read_len;

    // Ignore Ctrl+C
    // Actually, handling it to re-print prompt is friendlier than just ignoring?
    // Spec says "ignores ... alternatives ... exit program".
    // Example:
    // $ ^C
    // $ ^C
    // $ 
    // This implies it prints a new prompt.
    // So we need a handler that does nothing but maybe newline + prompt if we were reading.
    // But `getline` restarts if interrupted?
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart syscalls if possible
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        if (isatty(STDIN_FILENO)) {
            // Check if we just printed prompt in handler? 
            // It's tricky to handle prompt nicely with signals.
            // Simplified: Just SIG_IGN properly or handler.
            // If I use SIG_IGN, getline won't return.
            // If I use handler, getline might exit or restart.
            // Let's print prompt.
            printf("$ ");
            fflush(stdout);
        }

        read_len = getline(&line, &len, stdin);
        if (read_len == -1) {
            // EOF
            if (isatty(STDIN_FILENO)) printf("\n");
            break;
        }

        // Parse
        Command *cmd = parse_line(line);
        if (cmd) {
            execute_line(cmd);
            free_command(cmd);
        }
    }
    
    free(line);
}

int main(int argc, char *argv[]) {
    // Check arguments
    if (argc > 2) {
        print_error();
        exit(1);
    }

    // Batch mode
    if (argc == 2) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            print_error();
            exit(1);
        }
        // Redirect stdin to read from file
        // Or we can just process the file line by line here
        // But dup2 is easier to reuse the loop
        int fd = fileno(file);
        if (dup2(fd, STDIN_FILENO) == -1) {
            print_error();
            exit(1);
        }
        // Don't close file here yet, let clean up happen naturally or close fd later
    }

    loop();

    return 0;
}
