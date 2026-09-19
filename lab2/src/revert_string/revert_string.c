#include "revert_string.h"
#include <string.h>

void RevertString(char *str) {
    if (str == NULL) {
        return; 
    }

    size_t len = strlen(str);
    char *start = str;
    char *end = str + len - 1;

    while (start < end) {
        char tmp = *start;
        *start = *end;
        *end = tmp;

        ++start;
        --end;
    }
}
