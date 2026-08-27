
#include <stdio.h>
#include <string.h>

void strip_brackets(char *str, char leading, char trailing) {
    if (str == NULL || *str == '\0') return;

    char *start = str;
    char *end;

    // 1. Move 'start' past leading characters
    while (*start == leading && *start != '\0') {
        start++;
    }

    // 2. Find the end and work backward for trailing characters
    size_t len = strlen(start);
    if (len == 0) {
        *str = '\0'; // Entire string was leading chars
        return;
    }

    end = start + len - 1;
    while (end >= start && *end == trailing) {
        *end = '\0';
        end--;
    }

    // 3. Shift the result back to the original address
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

int main() {
    char data1[] = "[\"Important News\"]";
    char data2[] = "((Mistake))";

    strip_brackets(data1, '[', ']');
    strip_brackets(data2, '(', ')');

    printf("Result 1: %s\n", data1); // "Important News"
    printf("Result 2: %s\n", data2); // "Mistake"

    return 0;
}

