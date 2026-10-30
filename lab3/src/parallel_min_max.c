#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
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
            //мой код:
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            //мой код закончился строчкой выше
            break;
          case 1:
            array_size = atoi(optarg);
            // your code here
            // error handling
            //мой код:
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            //мой код закончился строчкой выше
            break;
          case 2:
            pnum = atoi(optarg);
            // your code here
            // error handling
            //мой код:
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            //мой код закончился строчкой выше
            break;
          case 3:
            with_files = true;
            break;

          //мой код:
          default:
          //мой код закончился строчкой выше
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
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  //мой код:
  int chunk = array_size / pnum;
  //мой код закончился строчкой выше

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  //мой код:
  int pipe_fd[2];
  if (!with_files && pipe(pipe_fd) == -1) {
    perror("pipe");
    return 1;
  }
  //мой код закончился строчкой выше

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process

        // parallel somehow
        //мой код:
        unsigned int begin = i * chunk;
        unsigned int end = (i == pnum - 1) ? array_size : begin + chunk;
        struct MinMax local = GetMinMax(array, begin, end);
        //мой код закончился строчкой выше

        if (with_files) {
          // use files here
          //мой код:
          char filename[64];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f == NULL) {
            perror("fopen");
            exit(1);
          }
          fprintf(f, "%d %d", local.min, local.max);
          fclose(f);
          //мой код закончился строчкой выше
        } else {
          // use pipe here
          //мой код:
          close(pipe_fd[0]);
          if (write(pipe_fd[1], &local, sizeof(local)) != sizeof(local)) {
            perror("write");
            exit(1);
          }
          close(pipe_fd[1]);
          //мой код закончился строчкой выше
        }
        //мой код:
        free(array);
        //мой код закончился строчкой выше
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  //мой код:
  // parent only reads from the pipe
  if (!with_files) close(pipe_fd[1]);
  //мой код закончился строчкой выше

  while (active_child_processes > 0) {
    // your code here
    //мой код:
    wait(NULL);
    //мой код закончился строчкой выше

    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      //мой код:
      char filename[64];
      sprintf(filename, "min_max_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f != NULL) {
        fscanf(f, "%d %d", &min, &max);
        fclose(f);
        remove(filename);
      }
      //мой код закончился строчкой выше
    } else {
      // read from pipes
      //мой код:
      struct MinMax local;
      if (read(pipe_fd[0], &local, sizeof(local)) == sizeof(local)) {
        min = local.min;
        max = local.max;
      }
      //мой код закончился строчкой выше
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  //мой код:
  if (!with_files) close(pipe_fd[0]);
  //мой код закончился строчкой выше

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}