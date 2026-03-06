#include "oshell.h"

// Extern from builtins.c
int execute_builtin(Command *cmd);

// Global path search list
// Default: /bin
static char **search_paths = NULL; // NULL-terminated array

void init_search_path() {
    if (search_paths) return;
    search_paths = malloc(2 * sizeof(char*));
    search_paths[0] = my_strdup("/bin");
    search_paths[1] = NULL;
}

void set_search_path(int argc, char **argv) {
    // Free old
    if (search_paths) {
        for (int i = 0; search_paths[i]; i++) free(search_paths[i]);
        free(search_paths);
    }
    // Copy new
    search_paths = malloc((argc + 1) * sizeof(char*));
    for (int i = 1; i < argc; i++) {
        search_paths[i-1] = my_strdup(argv[i]);
    }
    search_paths[argc-1] = NULL;
}

char *find_executable(char *cmd) {
    // If absolute or relative path
    if (strchr(cmd, '/')) {
        if (access(cmd, X_OK) == 0) return my_strdup(cmd);
        return NULL;
    }
    
    // Search in path
    if (!search_paths) init_search_path();
    
    for (int i = 0; search_paths[i]; i++) {
        char path[MAX_LINE];
        snprintf(path, sizeof(path), "%s/%s", search_paths[i], cmd);
        if (access(path, X_OK) == 0) {
            return my_strdup(path);
        }
    }
    return NULL;
}

int execute_command_node(Command *cmd) {
    if (!cmd || !cmd->argv || !cmd->argv[0]) return 0;

    // Handle redirections
    int saved_stdout = -1;
    if (cmd->output_file) {
        saved_stdout = dup(STDOUT_FILENO);
        int fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            print_error();
            return 1; // Error
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    // Check builtin
    int builtin_res = execute_builtin(cmd);
    if (builtin_res != -1) {
        // It was a builtin
        // Restore stdout if redirected
        if (saved_stdout != -1) {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        return builtin_res; // 0 success, >0 error
    }

    // External command
    char *exe_path = find_executable(cmd->argv[0]);
    if (!exe_path) {
        print_error(); // Command not found
        if (saved_stdout != -1) {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        return 127; // Command not found
    }

    int status = 0;
    pid_t pid = fork();
    if (pid == 0) {
        // Child
        signal(SIGINT, SIG_DFL); // Restore signal handling
        execv(exe_path, cmd->argv);
        print_error();
        exit(1);
    } else if (pid > 0) {
        // Parent
        if (saved_stdout != -1) {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }
        
        if (cmd->next_op != OP_BG) {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) status = WEXITSTATUS(status);
        }
    } else {
        print_error();
        status = 1;
    }
    
    free(exe_path);
    return status;
}


void execute_line(Command *head) {
    Command *cmd = head;
    
    while (cmd) {
        last_status = execute_command_node(cmd);
        
        if (cmd->next_op == OP_AND && last_status != 0) {
            // Failure: skip all following && nodes
            while (cmd && (cmd->next_op == OP_AND || (cmd->next && cmd->next->next_op == OP_AND))) {
                 // We need to skip the WHOLE chain of ANDs.
                 // Correct logic: skip until we find a node NOT connected by &&.
                 if (cmd->next_op != OP_AND) {
                     // This was the last one in the chain (it succeeded but was followed by something else)
                     // But wait, the condition is (cmd->next_op == OP_AND)
                     break;
                 }
                 cmd = cmd->next;
            }
            // Move past the node that was skipped
            if (cmd) cmd = cmd->next;
        } else if (cmd->next_op == OP_OR && last_status == 0) {
            // Success: skip all following || nodes
            while (cmd && cmd->next_op == OP_OR) {
                cmd = cmd->next;
            }
            if (cmd) cmd = cmd->next;
        } else {
            cmd = cmd->next;
        }
    }
    
    // Wait for all background processes spawned in this line
    while (wait(NULL) > 0);
}
