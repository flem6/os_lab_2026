#!/bin/bash

# Передаем все аргументы в awk
echo "$@" | awk '
{
    sum = 0
    count = NF
    for (i = 1; i <= NF; i++) {
        sum += $i
    }
    avg = sum / count
    printf "Количество чисел: %d\n", count
    printf "Среднее арифметическое: %.2f\n", avg
}'
