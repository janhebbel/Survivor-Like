#include <stdbool.h>

bool is_alphabetical(char c) {
    return 'a' <= c && c <= 'z' || 'A' <= c && c <= 'Z';
}

int string_compare(char *lhs, char *rhs) {
    int result = 0;
    char *lc, *rc;
    lc = lhs;
    rc = rhs;
    while (*lc && *rc) {
        if (*lc != *rc) {
            result = *lc - *rc;
            break;
        }
        lc++, rc++;
    }
    if (*lc == '\0' && *rc != '\0' || *lc != '\0' && *rc == '\0') {
        result = *lc - *rc;
    }
    return result;
}

int string_compare_1l(char *lhs, char *rhs, int length) {
    int i, result;
    result = 0;
    for (i = 0; i < length && lhs[i]; ++i) {
        if (lhs[i] != rhs[i]) {
            result = lhs[i] - rhs[i];
            break;
        }
    }
    if (i == length && lhs[i] != '\0' || i != length && lhs[i] == '\0') {
        result = lhs[i] - rhs[i];
    }
    return result;
}
