Group Members
Yohanan Tarekegn

Yididiya Gebretsadik

Remedan Mohammed

Kalkidan Wudineh

Abebu Alebachew

# OShell

OShell: A Custom Unix Command-Line Interpreter

OShell is a simplified Unix shell developed in C that replicates core functionalities of modern command-line interpreters like bash. This project focuses on the fundamentals of process management, command parsing, and file I/O operations.

## Features

### Core Requirements

- **Invocation Modes**: Supports Interactive mode (with custom prompt), Batch mode (reading from files), and Pipe mode.
- **Execution**: Correctly searches for executables in the PATH or via absolute/relative paths.
- **Redirection**: Supports standard output redirection using the `>` operator.
- **Signal Handling**: Ignores `Ctrl+C` in interactive mode to maintain session stability; handles `Ctrl+D` for graceful exits.
- **Error Handling**: Implements a unified error message ("An error has occurred") for all failure states as specified.
- **Documentation**: Includes comprehensive man pages for all built-in commands.

### Logical Operators

- **Sequential Execution (`;`)**: Run commands one after another.
- **Conditional AND (`&&`)**: Execute the next command only if the previous succeeds.
- **Conditional OR (`||`)**: Execute the next command only if the previous fails.
- **Parallel Execution (`&`)**: Run commands simultaneously and wait for all to complete before returning etc.

### Built-in Commands

- `exit`: Terminate the shell with an optional status code.
- `cd`: Change the current working directory, supporting `-` and `--` for OLDPWD.
- `env`: Display current environment variables.
- `setenv`: Create or update environment variables.
- `unsetenv`: Remove environment variables.
- `path`: Manage the shell's internal executable search path.
- `alias`: Define, view, or remove command aliases.

### Bonus / Advanced Features

- **Recursive Alias Expansion**: Aliases are expanded recursively until a base command is found (with cycle detection).
- **Variable Expansion**: Supports environment variables (`$VAR`), last exit status (`$?`), and current process ID (`$$`).
- **Robust Tokenizer**: Handles quotes (`'` and `"`) anywhere in a token, allowing spaces inside quoted values (critical for complex aliases).

## Compilation

The project uses a standard `Makefile` for easy compilation. Ensure you are in a Unix-like environment (WSL, Linux, macOS).

To compile the shell, run:

```bash
make
```

This generates the `oshell` executable with appropriate flags (`-Wall -Wextra -Werror -g`).

To clean build artifacts:

```bash
make clean
```

## Execution

### Interactive Mode

Launch the shell directly:

```bash
./oshell
```

### Batch Mode

Provide a script file as an argument:

```bash
./oshell scripts/test_commands.txt
```

### Pipe Mode

Pipe commands directly into the shell:

```bash
echo "ls -l; exit" | ./oshell
```
