#include "oshell.h"

// Helper to free command list
void free_command(Command *cmd) {
    while (cmd) {
        Command *next = cmd->next;
        if (cmd->argv) {
            for (int i = 0; cmd->argv[i]; i++) {
                free(cmd->argv[i]);
            }
            free(cmd->argv);
        }
        if (cmd->input_file) free(cmd->input_file);
        if (cmd->output_file) free(cmd->output_file);
        free(cmd);
        cmd = next;
    }
}

// Function to determine if a character is a special operator char
int is_special(char c) {
    return (c == ';' || c == '&' || c == '|' || c == '>');
}

// Function to skip whitespace
char *skip_whitespace(char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

// Main parsing function
Command *parse_line(char *line) {
    if (!line) return NULL;

    // Handle comments
    char *comment = strchr(line, '#');
    if (comment) *comment = '\0';

    Command *head = NULL;
    Command *current = NULL;
    char *p = line;

    while (*p) {
        p = skip_whitespace(p);
        if (!*p) break;

        // Allocate a new command structure if needed
        if (!current) {
            current = calloc(1, sizeof(Command));
            if (!head) head = current;
        } else {
            current->next = calloc(1, sizeof(Command));
            current = current->next;
        }

        int argv_cap = 16;
        current->argv = calloc(argv_cap, sizeof(char*));
        int argc = 0;

        while (*p) {
            p = skip_whitespace(p);
            if (!*p) break;

            // Operators
            if (*p == ';') {
                current->next_op = OP_SEQ;
                p++;
                break;
            }
            if (*p == '&') {
                if (*(p+1) == '&') {
                    current->next_op = OP_AND;
                    p += 2;
                } else {
                    current->next_op = OP_BG;
                    p++;
                }
                break;
            }
            if (*p == '|') {
                if (*(p+1) == '|') {
                    current->next_op = OP_OR;
                    p += 2;
                    break;
                } else {
                    print_error();
                    p++;
                    break;
                }
            }

            // Redirection
            if (*p == '>') {
                if (current->output_file) {
                    print_error();
                    break;
                }
                p++;
                p = skip_whitespace(p);
                char *start = p;
                while (*p && !isspace((unsigned char)*p) && !is_special(*p)) p++;
                if (p == start) {
                    print_error();
                    break;
                }
                current->output_file = calloc(p - start + 1, 1);
                memcpy(current->output_file, start, p - start);
                continue;
            }

            // Token
            char buffer[MAX_LINE];
            int b_idx = 0;
            while (*p && !isspace((unsigned char)*p) && !is_special(*p)) {
                if (*p == '\"' || *p == '\'') {
                    char quote = *p++;
                    while (*p && *p != quote) {
                        if (b_idx < MAX_LINE - 1) buffer[b_idx++] = *p++;
                        else p++;
                    }
                    if (*p == quote) p++;
                } else {
                    if (b_idx < MAX_LINE - 1) buffer[b_idx++] = *p++;
                    else p++;
                }
            }
            buffer[b_idx] = '\0';

            if (b_idx > 0) {
                char *arg = my_strdup(buffer);
                char *expanded = expand_variables(arg);
                free(arg);
                
                if (argc >= argv_cap - 1) {
                    argv_cap *= 2;
                    current->argv = realloc(current->argv, argv_cap * sizeof(char*));
                }
                current->argv[argc++] = expanded;
            }
        }
        current->argv[argc] = NULL;
        current->argc = argc;

        // Alias Expansion (Recursive)
        int expansion_depth = 0;
        while (current->argc > 0 && expansion_depth < 15) {
            char *alias_val = get_alias(current->argv[0]);
            if (!alias_val) break;
            expansion_depth++;

            int alias_tokens = 0;
            char *temp = my_strdup(alias_val);
            char *t = strtok(temp, " \t");
            while(t) { alias_tokens++; t = strtok(NULL, " \t"); }
            free(temp);

            if (alias_tokens > 0) {
                int new_argc = current->argc - 1 + alias_tokens;
                char **new_argv = calloc(new_argc + 1, sizeof(char*));
                temp = my_strdup(alias_val);
                t = strtok(temp, " \t");
                int i = 0;
                while(t) { new_argv[i++] = my_strdup(t); t = strtok(NULL, " \t"); }
                free(temp);
                for (int j = 1; j < current->argc; j++) new_argv[i++] = current->argv[j];
                new_argv[i] = NULL;
                free(current->argv[0]);
                free(current->argv);
                current->argv = new_argv;
                current->argc = new_argc;
            } else break;
        }
    }

    // Post-processing: remove empty nodes at the end
    Command *curr = head;
    Command *prev = NULL;
    while (curr) {
        if (curr->argc == 0 && !curr->output_file) {
            if (prev) prev->next = curr->next;
            else head = curr->next;
            
            Command *temp = curr;
            curr = curr->next;
            temp->next = NULL;
            free_command(temp);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }

    return head;
}
