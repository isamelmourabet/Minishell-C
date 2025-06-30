# 🐚 Minishell in C

A lightweight educational shell implemented in **C**. It supports:

- 🚀 Execution of external commands using `execvp()`
- 🔧 Built-in commands: `cd`, `jobs`, `fg`, `salir`
- 🎯 Background process control with `waitpid()` (non-blocking)
- ➡️ I/O redirection: input, output, and error
- 🧵 Pipe handling between child processes
- 🛡️ Signal management to prevent shell interruption

## 🔧 How to Compile

```bash
gcc -o minishell minishell.c parser.c -Wall
./minishell
