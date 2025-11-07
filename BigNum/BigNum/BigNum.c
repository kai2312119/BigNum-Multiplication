#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    size_t n;
    size_t cap;
    uint32_t* d;
} Big;

static void big_init(Big* x) {
    x->n = 0;
    x->cap = 0;
    x->d = NULL;
}

static void big_free(Big* x) {
    free(x->d);
    x->d = NULL;
    x->n = x->cap = 0;
}

static void big_reserve(Big* x, size_t need) {
    if (x->cap >= need) return;
    size_t nc = x->cap ? x->cap : 4;
    while (nc < need) {
        if (nc > (SIZE_MAX >> 1)) {
            fprintf(stderr, "capacity overflow\n");
            exit(1);
        }
        nc <<= 1;
    }
    void* p = realloc(x->d, nc * sizeof(uint32_t));
    if (!p) { perror("realloc"); exit(1); }
    x->d = (uint32_t*)p;
    x->cap = nc;
}

static void big_normalize(Big* x) {
    while (x->n > 0 && x->d[x->n - 1] == 0) x->n--;
}

static void big_zero(Big* x) {
    big_reserve(x, 1);
    x->d[0] = 0;
    x->n = 1;
}

static void big_mul10_add(Big* x, uint32_t digit) {
    uint64_t carry = digit;
    if (x->n == 0) big_zero(x);
    for (size_t i = 0; i < x->n; ++i) {
        uint64_t cur = (uint64_t)x->d[i] * 10ull + carry;
        x->d[i] = (uint32_t)cur;
        carry = cur >> 32;
    }
    if (carry) {
        big_reserve(x, x->n + 1);
        x->d[x->n++] = (uint32_t)carry;
    }
}

static int big_from_dec(Big* x, const char* s) {
    big_zero(x);
    while (isspace((unsigned char)*s)) ++s;
    if (*s == '+') ++s;
    if (!isdigit((unsigned char)*s)) return 0;
    for (; *s; ++s) {
        if (isspace((unsigned char)*s)) {
            while (isspace((unsigned char)*s)) ++s;
            break;
        }
        if (!isdigit((unsigned char)*s)) return 0;
        big_mul10_add(x, (uint32_t)(*s - '0'));
    }
    if (*s != '\0') return 0;
    big_normalize(x);
    if (x->n == 0) big_zero(x);
    return 1;
}

static void big_mul(Big* z, const Big* a, const Big* b) {
    if ((a->n == 0) || (b->n == 0) ||
        (a->n == 1 && a->d[0] == 0) ||
        (b->n == 1 && b->d[0] == 0)) {
        big_zero(z);
        return;
    }

    size_t an = a->n, bn = b->n;
    size_t rn = an + bn;

    big_reserve(z, rn);
    memset(z->d, 0, rn * sizeof(uint32_t));
    z->n = rn;

    for (size_t i = 0; i < an; ++i) {
        uint64_t carry = 0;
        uint64_t ai = a->d[i];
        for (size_t j = 0; j < bn; ++j) {
            uint64_t sum = (uint64_t)z->d[i + j] + ai * (uint64_t)b->d[j] + carry;
            z->d[i + j] = (uint32_t)sum;
            carry = sum >> 32;
        }
        size_t k = i + bn;
        while (carry) {
            uint64_t t = (uint64_t)z->d[k] + carry;
            z->d[k] = (uint32_t)t;
            carry = t >> 32;
            ++k;
            if (k == rn && carry) {
                big_reserve(z, rn + 1);
                z->d[rn++] = 0;
                z->n = rn;
            }
        }
    }

    big_normalize(z);
    if (z->n == 0) big_zero(z);
}

static void big_print_hex(const Big* x) {
    if (x->n == 0 || (x->n == 1 && x->d[0] == 0)) {
        puts("0x0");
        return;
    }
    printf("0x");
    printf("%x", x->d[x->n - 1]);
    for (size_t k = x->n - 1; k-- > 0; ) {
        printf("%08x", x->d[k]);
    }
    puts("");
}

int main(void) {
    char a_str[4096], b_str[4096];

    printf("Enter first (decimal) number: ");
    fflush(stdout);
    if (!fgets(a_str, sizeof(a_str), stdin)) {
        fprintf(stderr, "Input error.\n");
        return 1;
    }

    printf("Enter second (decimal) number: ");
    fflush(stdout);
    if (!fgets(b_str, sizeof(b_str), stdin)) {
        fprintf(stderr, "Input error.\n");
        return 1;
    }

    Big A, B, C;
    big_init(&A); big_init(&B); big_init(&C);

    if (!big_from_dec(&A, a_str) || !big_from_dec(&B, b_str)) {
        fprintf(stderr, "Invalid input. Please enter decimal digits only.\n");
        big_free(&A); big_free(&B); big_free(&C);
        return 1;
    }

    big_mul(&C, &A, &B);

    printf("Result (hex): ");
    big_print_hex(&C);

    big_free(&A); big_free(&B); big_free(&C);
    return 0;
}
