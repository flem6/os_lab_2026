#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h> //мой код

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

//мой код:
// обработчик сигнала не получает аргументов от main, поэтому данные глобальные
static volatile pid_t *child_pids = NULL;
static int child_count = 0;
static volatile sig_atomic_t timed_out = 0;

static void alarm_handler(int sig) {
  (void)sig;
  timed_out = 1;
  for (int i = 0; i < child_count; i++) {
    // 0 значит, что потомок уже завершён; kill(0, ...) убил бы всю группу
    if (child_pids[i] > 0) kill(child_pids[i], SIGKILL);
  }
}
//мой код выше

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout = -1; //мой код

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0}, //мой код
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            // your code here
            // error handling
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            // your code here
            // error handling
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            // your code here
            // error handling
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          //мой код:
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be a positive number\n");
              return 1;
            }
            break;
          //мой код выше

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
           "[--timeout \"sec\"]\n", //мой код
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int chunk = array_size / pnum;
  child_pids = malloc(sizeof(pid_t) * pnum); //мой код

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipe_fd[2];
  if (!with_files && pipe(pipe_fd) == -1) {
    perror("pipe");
    return 1;
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process

        // parallel somehow
        unsigned int begin = i * chunk;
        unsigned int end = (i == pnum - 1) ? array_size : begin + chunk;
        struct MinMax local = GetMinMax(array, begin, end);

        if (with_files) {
          // use files here
          char filename[64];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f == NULL) {
            perror("fopen");
            exit(1);
          }
          fprintf(f, "%d %d", local.min, local.max);
          fclose(f);
        } else {
          // use pipe here
          close(pipe_fd[0]);
          if (write(pipe_fd[1], &local, sizeof(local)) != sizeof(local)) {
            perror("write");
            exit(1);
          }
          close(pipe_fd[1]);
        }
        free(array);
        return 0;
      }

      //мой код:
      child_pids[i] = child_pid;
      child_count = i + 1;
      //мой код выше

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // parent only reads from the pipe
  if (!with_files) close(pipe_fd[1]);

  //мой код:
  // таймер ставим после всех fork, чтобы обработчик видел все pid
  if (timeout > 0) {
    signal(SIGALRM, alarm_handler);
    alarm(timeout);
  }
  //мой код выше

  while (active_child_processes > 0) {
    // your code here
    //мой код:
    if (timeout > 0) {
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] == 0) continue;
        if (waitpid(child_pids[i], NULL, WNOHANG) != 0) {
          child_pids[i] = 0;
          active_child_processes -= 1;
        }
      }
      if (active_child_processes > 0) usleep(1000);
      continue;
    }
    //мой код выше
    wait(NULL);

    active_child_processes -= 1;
  }

  //мой код:
  // все успели завершиться: отменяем таймер, иначе он сработает зря
  alarm(0);
  free((void *)child_pids);
  //мой код выше

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      char filename[64];
      sprintf(filename, "min_max_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f != NULL) {
        fscanf(f, "%d %d", &min, &max);
        fclose(f);
        remove(filename);
      }
    } else {
      // read from pipes
      struct MinMax local;
      if (read(pipe_fd[0], &local, sizeof(local)) == sizeof(local)) {
        min = local.min;
        max = local.max;
      }
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  if (!with_files) close(pipe_fd[0]);

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  //мой код:
  if (timed_out) {
    printf("Timeout: child processes were killed, result is incomplete\n");
  } else {
    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
  }
  //мой код выше
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
