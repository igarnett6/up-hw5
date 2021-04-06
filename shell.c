// #define _POSIX_C_SOURCE
#include <stdio.h>   // printf, fgets
#include <string.h>  // strlen
#include <unistd.h>  // execlp
#include <stdbool.h> // true
#include <sys/wait.h> //wait
#include <stdlib.h> //?malloc
#include <limits.h> //realpath
#include <fcntl.h> //O_RDONLY
#include <signal.h>


//prototypes
void handleExit(char* command, struct sigaction old_quit_act, struct sigaction old_int_act);
bool handleCD(char *command);
void handle_fork(char *command,
                 struct sigaction handle_kill,
                 bool isPiped,
                 int rw_pipe_id,
                 int pfd[]);
bool handleForkIfPipe(char *command, struct sigaction handle_kill);
void checkRedirStdin(char *command);
void checkRedirStdout(char *command);
void checkRedirAppend(char *command);
void checkRedirStderr(char *command);
void checkRedirOutErr(char *command);;
void checkAppendOutErr(char *command);
void execCommand(char *command);


//signal handler for sigquit and sigint
void kill_handler(int sigNum){
  perror("process killed by signal");
  exit(1);
}


int main()
{

    //handle kill signals
    struct sigaction handle_kill, old_int_act, old_quit_act;
    handle_kill.sa_handler = SIG_IGN;
    sigemptyset(&handle_kill.sa_mask);
    handle_kill.sa_flags = 0;
    sigaction(SIGINT, &handle_kill, &old_int_act);
    sigaction(SIGQUIT, &handle_kill, &old_quit_act);


    while(true) {     // loop to show shell prompt
        char *prompt = getenv("PS1");
        if(prompt != NULL){
          printf(prompt);
        }
        else{
          char pwd[4096];
          realpath(".", pwd); // used in shell prompt
          printf("╭─%s -> [PID: %d]\n", pwd, getpid()); // pid of shell included in prompt
          printf("╰─(╯°益°)╯彡┻━┻ ");
        }
        char *command = calloc(1, 4096); // user input, shell command
        fgets(command, 4096, stdin);
        int len = strlen(command); // len of command used to remove newline chars
        if (command[len-1] == '\n') command[len-1] = '\0';
        handleExit(command, old_quit_act, old_int_act);
        if(handleCD(command) == true){
          continue;
        }
        if(handleForkIfPipe(command, handle_kill) == false){
          // if no '|' char in command
          handle_fork(command, handle_kill, false, '\0', '\0'); // last 2 args null with no pipe
          if((wait(NULL)) == -1){
            perror("No children");
          }
        }
        else{
           // if '|' in command handleForkIfPipe() calls handleFork() for both piped commands
           //then wait for both forked child processes
          if((wait(NULL)) == -1){
            perror("No children");
          }
          if((wait(NULL)) == -1){
            perror("No children");
          }
        }
        free(command);
    }
}

void handleExit(char* command, struct sigaction old_quit_act, struct sigaction old_int_act){
  if((strncmp(command, "exit", 4)) == 0){ // if user entered exit close shell
    sigaction(SIGINT, &old_int_act, NULL); // reset to old sigint disposition
    sigaction(SIGQUIT, &old_quit_act, NULL); // reset to old sigquit disposition
    exit(0);
  }
}

bool handleCD(char *command){
  if((strncmp(command, "cd", 2)) == 0){
    strtok(command, " ");
    char *newDir = strtok(NULL, " ");
    chdir(newDir);
    return true;
  }
  return false;
}

bool handleForkIfPipe(char *command, struct sigaction handle_kill){
  char *cmd1;
  char *cmd2;
  if((strstr(command, "|")) != NULL){
    cmd1 = strtok(command, "|");
    cmd2 = strtok(NULL, "|");
    if(strtok(NULL, "|") != NULL){
      printf("Only one pipe allowed.");
      exit(1);
    }
    int pfd[2];
    if(pipe(pfd) == -1){
      perror("failed to open pipe");
      exit(1);
    }
    handle_fork(cmd1, handle_kill, true, 0, pfd);
    handle_fork(cmd2, handle_kill, true, 1, pfd);
    return true;
  }
  else{ return false;}
}

void handle_fork(char *command,
        struct sigaction handle_kill,
        bool isPiped,
        int rw_pipe_id,
        int pfd[]){
          if(fork() == 0){
            handle_kill.sa_handler = kill_handler;
            sigaction(SIGINT, &handle_kill, NULL);
            sigaction(SIGQUIT, &handle_kill, NULL);

            if(isPiped == true){
              if(rw_pipe_id == 0){
                close(pfd[0]);
                if(dup2(pfd[1], 1) == -1){
                  perror("dup2 failed to make pfd[1] a copy of stdout");
                  exit(1);
                }
                close(pfd[1]);
              }
              else if(rw_pipe_id == 1){
                close(pfd[1]);
                if(dup2(pfd[0], 0) == -1){
                  perror("dup2 failed to copy make pfd[0] a copy of stdin");
                  exit(1);
                }
                close(pfd[0]);
              }
            }

            char cmdCopy[4096];
            strcpy(cmdCopy, command);
            checkRedirStdin(cmdCopy);
            strcpy(cmdCopy, command);
            checkRedirStdout(cmdCopy);
            strcpy(cmdCopy, command);
            checkRedirAppend(cmdCopy);
            strcpy(cmdCopy, command);
            checkRedirStderr(cmdCopy);
            strcpy(cmdCopy, command);
            checkRedirOutErr(cmdCopy);
            strcpy(cmdCopy, command);
            checkAppendOutErr(cmdCopy);
            execCommand(command);
            _exit(0);
          }
          else{
            if(isPiped == true){
              close(pfd[1]);
            }
          }
}


void checkRedirStdin(char *command){
  char *redirFile;
  if((redirFile = strstr(command, " < ")) != NULL){
    redirFile = redirFile+3;
    redirFile = strtok(redirFile, " ");
    close(0);
    int fd = open(redirFile, O_RDONLY);
    if(fd == -1){
      perror("Failed to open \"redirFile\"");
      exit(1);
    }
  }
}

void checkRedirStdout(char *command){
  char *redirFile;
  if((redirFile = strstr(command, " > ")) != NULL){
    redirFile = redirFile+3;
    redirFile = strtok(redirFile, " ");
    close(1);
    int fd = open(redirFile, O_RDWR | O_CREAT);
    if(fd == -1){
      perror("Failed to open \"redirFile\"");
      exit(1);
    }
  }
}

void checkRedirAppend(char *command){
  char *redirFile;
  if((redirFile = strstr(command, " >> ")) != NULL){
    redirFile = redirFile+3;
    redirFile = strtok(redirFile, " ");
    close(1);
    int fd = open(redirFile, O_RDWR | O_CREAT | O_APPEND);
    if(fd == -1){
      perror("Failed to open \"redirFile\"");
      exit(1);
    }
  }
}

void checkRedirStderr(char *command){
  char *redirFile;
  if((redirFile = strstr(command, " 2> ")) != NULL){
    redirFile = redirFile+3;
    redirFile = strtok(redirFile, " ");
    close(2);
    int fd = open(redirFile, O_RDWR | O_CREAT);
    if(fd == -1){
      perror("Failed to open \"redirFile\"");
      exit(1);
    }
  }
}

void checkRedirOutErr(char *command){
  char *redirFile;
  if((redirFile = strstr(command, " &> ")) != NULL){
    redirFile = redirFile+3;
    redirFile = strtok(redirFile, " ");
    int fd = open(redirFile, O_RDWR | O_CREAT);
    if(dup2(fd, 1)){
      perror("dup 2 failed");
      exit(1);
    }
    close(fd);
    fd = open(redirFile, O_RDWR | O_CREAT);
    if(dup2(fd, 2) ==-1){
      perror("dup2 failed");
      exit(1);
    }
    if(fd == -1){
      perror("Failed to open \"redirFile\"");
      exit(1);
    }
  }
}

void checkAppendOutErr(char *command){
  char *redirFile;
  if((redirFile = strstr(command, " &> ")) != NULL){
    redirFile = redirFile+3;
    redirFile = strtok(redirFile, " ");
    int fd = open(redirFile, O_RDWR | O_CREAT | O_APPEND);
    if(dup2(fd, 1) == -1){
      perror("dup2 failed");
      exit(1);
    }
    close(fd);
    fd = open(redirFile, O_RDWR | O_CREAT | O_APPEND);
    if(dup2(fd, 2) == -1){
      perror("dup2 failed");
      exit(1);
    }
    if(fd == -1){
      perror("Failed to open \"redirFile\"");
      exit(1);
    }
  }
}

void execCommand(char *command){
  char *token = calloc(1, 4096);
  token = strtok(command, " ");
  char *args[4096];
  args[0] = token;
  int i = 1;
  while((token = strtok(NULL, " ")) != NULL){
    if((token[0] != '<') && (token[0] != '>') && !(token[0] == '2' && token[1] == '>')
  && !(token[0] == '&' && token[1] == '>')){
      args[i] = token;
      int argLen = strlen(args[i]);
      if (args[i][argLen-1] == '\n') args[i][argLen-1] = '\0';
    }
    i++;
  }
  execvp(args[0], args);
  perror("Failed to exec");
  free(token);
}
