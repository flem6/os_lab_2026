#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

static void show_process(pid_t pid) {
  char command[64];
  snprintf(command, sizeof(command), "ps -o pid,ppid,stat,comm -p %d", (int)pid);
  fflush(stdout);
  system(command);
}

int main(void) {
  pid_t child_pid = fork();

  if (child_pid < 0) {
    perror("fork");
    return 1;
  }

  if (child_pid == 0) {
    printf("Child %d: exiting now\n", (int)getpid());
    exit(0);
  }

  // родитель не вызывает wait, пока потомок успевает завершиться
  sleep(2);

  printf("\nParent has not called wait, child is a zombie (STAT = Z):\n");
  show_process(child_pid);

  // waitpid забирает код завершения, и ядро удаляет запись о потомке
  waitpid(child_pid, NULL, 0);

  printf("\nParent called waitpid, zombie is gone (empty list):\n");
  show_process(child_pid);

  return 0;
}
