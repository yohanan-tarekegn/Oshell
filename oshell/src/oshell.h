#ifndef OSHELL_H
#define OSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

#define MAX_LINE 1024
#define MAX_ARGS 128
#define WHITESPACE " \t\r\n\a"

// Error Message
#define ERROR_MSG "An error has occurred\n"

// Global definition regarding builtins return (custom protocol)
// 0: success, -1: not a builtin, >0: error in builtin

typedef enum {
    OP_NONE = 0,
    OP_SEQ, // ;
    OP_AND, // &&
    OP_OR,  // ||
    OP_BG   // &
} Operator;

typedef struct Command {
    char **argv;     // Null-terminated array of arguments
    int argc;
    char *input_file;  // For < (not requested but good to have) or pipe input placeholder
    char *output_file; // For >
    Operator next_op;  // Operator connecting to the next command
    struct Command *next;
} Command;

// Function Prototypes

// function to print error
void print_error();

// utils.c
char *my_strdup(const char *s);
void trim_whitespace(char *str);
char *expand_variables(char *token);

// main.c
// void print_error(); // Already above


// parser.c
Command *parse_line(char *line);
void free_command(Command *cmd);

// executor.c
void execute_line(Command *cmd);

// builtins.c
char *get_alias(char *name);

// Global status
extern int last_status;

// To be defined

#endif
