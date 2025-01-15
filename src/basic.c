#include <stdbool.h>

#define array_len(x) (sizeof(x) / sizeof(*x))

#define minimum(x, y) (((x) < (y)) ? (x) : (y))
#define maximum(x, y) (((x) > (y)) ? (x) : (y))
#define sign(x)       ((x > 0) - (x < 0))
#define abs(x)        ((x < 0) ? -(x) : (x))
#define clamp(x, min, max) (maximum(minimum(x, max), min))

#define kilobytes(x) ((x) << 10)
#define megabytes(x) ((x) << 20)
#define gigabytes(x) ((x) << 30)

#define is_enabled(x) ((x) != 0)

int is_end_of_line(char c) {
    return (c == '\n') || (c == '\r');
}

int is_spacing(char c) {
    return (c == ' ') || (c == '\t') || (c == '\v') || (c == '\f');
}

int is_whitespace(char c) {
    return is_spacing(c) || is_end_of_line(c);
}

int is_letter(char c) {
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z';
}

int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int is_alphanumeric(char c) {
    return is_letter(c) || is_digit(c);
}

int is_power_of_two(uintptr_t x) {
    return (x & (x-1)) == 0;
}

int string_compare(char *a, char *b) {
    while (*a && *b && *a == *b) {
        a += 1;
        b += 1;
    }
    return *a - *b;
}

int string_compare_1l(char *a, char *b, int b_length) {
    int result, i;
    i = 0;
    while (*a && i < b_length && *a == b[i]) {
        a += 1;
        i += 1;
    }
    if (*a == '\0' && i == b_length) {
        result = 0;
    } else if (i == b_length) {
        result = *a;
    } else {
        result = *a - b[i];
    }
    return result;
}

int string_to_int(char *str, int len) {
    // the maximum characters a 32-bit integer can have, is 11
    char null_terminated_str[12] = {0};
    memcpy(null_terminated_str, str, len);
    int i = atoi(null_terminated_str);
    return i;
}

float string_to_float(char *str, int len) {
    // 1 sign character + largest integral part: 39 + 1 decimal point + fractional part: 7 = 48
    char null_terminated_str[49] = {0};
    memcpy(null_terminated_str, str, len);
    float f = (float)atof(null_terminated_str);
    return f;
}

int round_float(float val) {
    // if val is positive 1 - 0 = +1,
    // if val is negative 0 - 1 = -1.
    int   sign = (val > 0) - (val < 0);
    float half = 0.5;
    return (int)(val + half * sign);
}
