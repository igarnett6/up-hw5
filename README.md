# A simple shell (in C)

## Description
This shell currently supports:
- I/O redirection (<,>, >>, 2>, 2>>, &>, &>>)
- custom prompts can be used by setting the PS1 env var
- cd and exit built-in commands
- SIGINT and SIGKILL behavior (kills child processes, not main shell)
- piping for a single pair of commands

Not currently supported:
- quotations
- globbing
- background processing


## How to run
To compile, cd into the directory containing shell.c and run:

`make shell` or `make`

To remove old executables run:

`make clean`

The shell can be started with `./shell` and used as normal.
The shell can be terminated by entering `exit`.
