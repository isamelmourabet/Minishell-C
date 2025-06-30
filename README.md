# 🐚 Minishell in C
A minimal Unix shell implemented in **C** with support for foreground/background job execution, internal commands (cd, jobs, fg, salir), input/output/error redirection, pipelines between processes, and robust signal handling. Designed as an educational project to deepen understanding of process management and system-level programming in Unix-like environments.

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

msh > ls -l | grep ".c" > output.txt &
msh > jobs
msh > fg 1
