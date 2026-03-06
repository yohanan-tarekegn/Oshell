#include "oshell.h"

void print_error() {
    write(STDERR_FILENO, ERROR_MSG, strlen(ERROR_MSG));
}

// Helper to remove whitespace from end of string
void trim_whitespace(char *str) {
    char *end;
    
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';
}

// Custom strdup since it's not always standard C (it is POSIX)
char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *new = malloc(len);
    if (new == NULL) return NULL;
    return (char *)memcpy(new, s, len);
}

// Split line into tokens, respecting quotes could be complex, 
// for "simplified" shell maybe just space logic is enough unless quotes are required.
// Requirement says "Unix-like", usually implies quotes support.
// "echo 'hello world'" should be one arg.
// Requirement example: echo "echo hello" | oshell
// So quotes ARE needed.

char **tokenize(char *line) {
    int bufsize = MAX_ARGS;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;
    
    // Basic tokenizer that doesn't fully handle quotes yet - 
    // we need to implement a smarter one for "echo 'foo bar'"
    // But let's start with strtok as a baseline
    
    if (!tokens) {
        fprintf(stderr, "oshell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, WHITESPACE);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            // bufsize += MAX_ARGS;
            // tokens = realloc(tokens, bufsize * sizeof(char*));
            // Just cap it for now as per robust requirements
            break;
        }

        token = strtok(NULL, WHITESPACE);
    }
    tokens[position] = NULL;
    return tokens;
}

char *expand_variables(char *token) {
    if (!token) return NULL;
    
    // Check for $
    char *dollar = strchr(token, '$');
    if (!dollar) return my_strdup(token);

    // Simple expansion: assumes token IS the variable or contains it.
    // Handles $VAR, $?, $$ in a simple way (replacing the whole token if it matches exactly, or handling substrings?)
    // Spec says "Variable Replacement", "$NAME expands to its value". "Example: Echo $HOME".
    // Usually means substrings too "a$HOME", but for simplified shell, maybe just if token starts with $?
    // Let's implement robust substring expansion.
    
    char buffer[MAX_LINE] = {0};
    char *p = token;
    char *b = buffer;
    
    while (*p) {
        if (*p == '$') {
            p++; // Skip $
            if (*p == '?') {
                // Expand $?
                char temp[16];
                snprintf(temp, sizeof(temp), "%d", last_status);
                strcpy(b, temp);
                b += strlen(temp);
                p++;
            } else if (*p == '$') {
                // Expand $$
                char temp[16];
                snprintf(temp, sizeof(temp), "%d", getpid());
                strcpy(b, temp);
                b += strlen(temp);
                p++;
            } else if (isalnum((unsigned char)*p) || *p == '_') {
                // Environment variable name
                char var_name[MAX_ARGS];
                int i = 0;
                while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
                    var_name[i++] = *p++;
                }
                var_name[i] = '\0';
                
                char *val = getenv(var_name);
                if (val) {
                    strcpy(b, val);
                    b += strlen(val);
                }
                // If undefined, expand to empty string (advance b by 0)
            } else {
                // Just a $
                *b++ = '$';
            }
        } else {
            *b++ = *p++;
        }
    }
    *b = '\0';
    
    return my_strdup(buffer);
}
