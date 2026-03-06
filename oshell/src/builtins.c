#include "oshell.h"

// Forward declaration of internal function to update path
void set_search_path(int argc, char **argv);
// Forward declaration for alias (simple linked list for now)
int handle_alias(int argc, char **argv);

extern char **environ; // Standard environment variable array

int builtin_exit(int argc, char **argv) {
    if (argc > 1) {
        exit(atoi(argv[1]));
    }
    exit(0);
    return 0; // Unreachable
}

int builtin_cd(int argc, char **argv) {
    char *target = NULL;
    if (argc == 1) {
        // cd -> HOME
        target = getenv("HOME");
    } else {
        if (strcmp(argv[1], "-") == 0) {
            target = getenv("OLDPWD");
            if (target) printf("%s\n", target);
        } else if (strcmp(argv[1], "--") == 0) {
            target = getenv("OLDPWD");
        } else {
            target = argv[1];
        }
    }

    if (!target) {
        // If HOME or OLDPWD not set, usually error?
        print_error(); 
        return 1;
    }

    char cwd[MAX_LINE];
    getcwd(cwd, sizeof(cwd)); // Get current before changing to set OLDPWD later

    if (chdir(target) != 0) {
        print_error();
        return 1;
    }

    // Update PWD and OLDPWD
    setenv("OLDPWD", cwd, 1);
    
    char new_cwd[MAX_LINE];
    getcwd(new_cwd, sizeof(new_cwd));
    setenv("PWD", new_cwd, 1);

    return 0;
}

int builtin_env(int argc, char **argv) {
    (void)argc;
    (void)argv;
    char **s = environ;
    while (*s) {
        printf("%s\n", *s);
        s++;
    }
    return 0;
}

int builtin_setenv(int argc, char **argv) {
    if (argc < 2) {
        print_error();
        return 1;
    }
    // Usage: setenv NAME VALUE (or empty)
    if (argc == 2) {
        setenv(argv[1], "", 1);
    } else {
        setenv(argv[1], argv[2], 1);
    }
    return 0;
}

int builtin_unsetenv(int argc, char **argv) {
    if (argc < 2) {
        print_error();
        return 1;
    }
    unsetenv(argv[1]);
    return 0;
}

// Alias implementation (Simple linked list)
typedef struct AliasNode {
    char *name;
    char *value;
    struct AliasNode *next;
} AliasNode;

static AliasNode *alias_head = NULL;

char *get_alias(char *name) {
    AliasNode *curr = alias_head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }
    return NULL;
}

int handle_alias(int argc, char **argv) {
    if (argc == 1) {
        // Print all aliases
        AliasNode *curr = alias_head;
        while (curr) {
            printf("%s='%s'\n", curr->name, curr->value);
            curr = curr->next;
        }
    } else {
        // Process arguments
        for (int i = 1; i < argc; i++) {
            char *arg = argv[i];
            char *eq = strchr(arg, '=');
            
            if (eq) {
                // Define alias NAME=VALUE
                *eq = '\0';
                char *name = arg;
                char *value = eq + 1;
                
                // Check if value is quoted (simple check)
                if ((value[0] == '\'' && value[strlen(value)-1] == '\'') || 
                    (value[0] == '"' && value[strlen(value)-1] == '"')) {
                    value[strlen(value)-1] = '\0';
                    value++;
                }
                
                // Update or Add
                AliasNode *curr = alias_head;
                int found = 0;
                while (curr) {
                    if (strcmp(curr->name, name) == 0) {
                        free(curr->value);
                        curr->value = my_strdup(value);
                        found = 1;
                        break;
                    }
                    curr = curr->next;
                }
                if (!found) {
                    AliasNode *new_node = malloc(sizeof(AliasNode));
                    new_node->name = my_strdup(name);
                    new_node->value = my_strdup(value);
                    new_node->next = alias_head;
                    alias_head = new_node;
                }
            } else {
                // Print specific alias
                char *name = arg;
                AliasNode *curr = alias_head;
                while (curr) {
                    if (strcmp(curr->name, name) == 0) {
                        printf("%s='%s'\n", curr->name, curr->value);
                        break;
                    }
                    curr = curr->next;
                }
            }
        }
    }
    return 0;
}

// Builtin dispatcher
// Returns 0 if success, 1 if error, -1 if not a builtin
int execute_builtin(Command *cmd) {
    if (!cmd->argv[0]) return -1;
    
    if (strcmp(cmd->argv[0], "exit") == 0) {
        return builtin_exit(cmd->argc, cmd->argv);
    }
    if (strcmp(cmd->argv[0], "cd") == 0) {
        return builtin_cd(cmd->argc, cmd->argv);
    }
    if (strcmp(cmd->argv[0], "env") == 0) {
        return builtin_env(cmd->argc, cmd->argv);
    }
    if (strcmp(cmd->argv[0], "setenv") == 0) {
        return builtin_setenv(cmd->argc, cmd->argv);
    }
    if (strcmp(cmd->argv[0], "unsetenv") == 0) {
        return builtin_unsetenv(cmd->argc, cmd->argv);
    }
    if (strcmp(cmd->argv[0], "path") == 0) {
        set_search_path(cmd->argc, cmd->argv);
        return 0;
    }
    if (strcmp(cmd->argv[0], "alias") == 0) {
        return handle_alias(cmd->argc, cmd->argv);
    }
    
    return -1; // Not a builtin
}
