#include <stdio.h>   // printf, fgets
#include <string.h>  // strlen
#include <unistd.h>  // execlp
#include <stdbool.h> // true
#include <sys/wait.h> //wait
#include <stdlib.h> //?malloc
#include <limits.h> //realpath
#include <fcntl.h> //O_RDONLY
#include <signal.h>



int main(){
  int fd[2];
  if(pipe(fd) == -1){
    perror("failed to open pipe");
    exit(1);
  }

  if(fork() == 0){
    dup2(fd[1], stdout);
    close(fd[1]);
    close(fd[0]);
    printf("hello");
  }

}
