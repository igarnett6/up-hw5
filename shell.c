/*
  Minimal Shell
  "Looping":
     Displays a prompt
     Takes in a command
     Executes the command
  Since it execs without forking, it can only run a single command.
  Also doesn't tokenize the input...
 */
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
void checkRedirStdin(char *command);
void checkRedirStdout(char *command);
void checkRedirAppend(char *command);
void checkRedirStderr(char *command);
void checkRedirOutErr(char *command);;
void checkAppendOutErr(char *command);
void execCommand(char *command);

void kill_handler(int sigNum){
  perror("process killed by signal");
  exit(1);
}

int main()
{
    //handle sigint
    struct sigaction handle_kill, old_int_act, old_quit_act;
    handle_kill.sa_handler = SIG_IGN;
    sigemptyset(&handle_kill.sa_mask);
    handle_kill.sa_flags = 0;
    sigaction(SIGINT, &handle_kill, &old_int_act);
    sigaction(SIGQUIT, &handle_kill, &old_quit_act);

    while(true) {
        char *prompt = getenv("PS1");
        if(prompt != NULL){
          printf(prompt);
        }
        else{
          char pwd[4096];
          realpath(".", pwd);
          printf("╭─%s -> [PID: %d]\n", pwd, getpid());
          printf("╰─(╯°益°)╯彡┻━┻ ");
        }
        char *command = calloc(1, 4096);
        fgets(command, 4096, stdin);
        int len = strlen(command);
        if (command[len-1] == '\n') command[len-1] = '\0';
        if((strncmp(command, "exit", 4)) == 0){
          sigaction(SIGINT, &old_int_act, NULL);
          sigaction(SIGQUIT, &old_quit_act, NULL);
          exit(0);
        }
        int changeDir = 0; //bool for if cd in stdin
        if((strncmp(command, "cd", 2)) == 0){
          changeDir = 1;
        }
        if(changeDir == 1){
          strtok(command, " ");
          char *newDir = strtok(NULL, " ");
          chdir(newDir);
          continue;
        }

        if(fork() == 0){
          handle_kill.sa_handler = kill_handler;
          sigaction(SIGINT, &handle_kill, NULL);
          sigaction(SIGQUIT, &handle_kill, NULL);

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
          if((wait(NULL)) == -1){
            perror("No children");
          }
        }
        free(command);
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
    dup2(fd, 1);
    close(fd);
    fd = open(redirFile, O_RDWR | O_CREAT);
    dup2(fd, 2);
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
    dup2(fd, 1);
    close(fd);
    fd = open(redirFile, O_RDWR | O_CREAT | O_APPEND);
    dup2(fd, 2);
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
