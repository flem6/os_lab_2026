#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

//мой код:
#include <getopt.h>
#include <sys/time.h>

#include "sum.h"
#include "utils.h"
//мой код выше

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  //мой код:
  static struct option options[] = {{"threads_num", required_argument, 0, 0},
                                    {"seed", required_argument, 0, 0},
                                    {"array_size", required_argument, 0, 0},
                                    {0, 0, 0, 0}};

  while (1) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;
    if (c != 0) return 1;

    int value = atoi(optarg);
    if (value <= 0) {
      printf("%s must be a positive number\n", options[option_index].name);
      return 1;
    }

    switch (option_index) {
      case 0:
        threads_num = value;
        break;
      case 1:
        seed = value;
        break;
      case 2:
        array_size = value;
        break;
    }
  }

  if (optind < argc || threads_num == 0 || seed == 0 || array_size == 0) {
    printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n",
           argv[0]);
    return 1;
  }
  //мой код выше

  pthread_t threads[threads_num];

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed); //мой код

  struct SumArgs args[threads_num];

  //мой код:
  uint32_t chunk = array_size / threads_num;

  // замер начинается после генерации массива
  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  //мой код выше

  for (uint32_t i = 0; i < threads_num; i++) {
    //мой код:
    args[i].array = array;
    args[i].begin = i * chunk;
    args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk;
    //мой код выше
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) { //мой код
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  long long total_sum = 0; //мой код
  for (uint32_t i = 0; i < threads_num; i++) {
    //мой код:
    // pthread_join записывает в result значение размером с указатель
    void *result = NULL;
    pthread_join(threads[i], &result);
    total_sum += (size_t)result;
    //мой код выше
  }

  //мой код:
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;
  //мой код выше

  free(array);
  printf("Total: %lld\n", total_sum); //мой код
  printf("Elapsed time: %fms\n", elapsed_time); //мой код
  return 0;
}