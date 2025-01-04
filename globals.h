
#ifndef globals
#define globals

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// #define MAX_PASSWORD_LENGTH 4
// #define ALPHABET_SIZE 4
// const char *alphabet = "abcd";

#define PASSWORD_LENGTH 12
#define ALPHABET_SIZE 26
#define BATCH_SIZE 65535
const char *alphabet = "abcdefghijklmnopqrstuvwxyz";
const uint8_t TEST_HASH[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
                               0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
const size_t PERMUTATIONS_TO_CHECK = 1ul << 28;
const size_t CHECKPOINT_COUNT = 1ul << 16;

struct search_result {
    char *password;
    size_t checked_passwords;
};

struct progress_context {
    struct timeval start;
    struct timeval last_check;

    size_t last_checked_passwords;
    size_t total_checked_passwords;
};

void update_progress(struct progress_context *context,
                            size_t check_count) {
    context->last_checked_passwords = context->total_checked_passwords;
    context->total_checked_passwords = check_count;
}

void print_permutation(size_t *indeces, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%c", alphabet[indeces[i]]);
    }
    printf("; thread: %d\n", omp_get_thread_num());
}

void print_hash(uint8_t *p) {
    for (unsigned int i = 0; i < 16; ++i) {
        printf("%02x", p[i]);
    }
    printf("\n");
}

void print_progres(struct progress_context *context) {
    struct timeval now;
    size_t check_count = context->total_checked_passwords - context->last_checked_passwords;

    gettimeofday(&now, NULL);
    double time_taken = (now.tv_sec - context->start.tv_sec) +
                        (now.tv_usec - context->start.tv_usec) / 1e6;

    double time_taken_since_last_check =
        (now.tv_sec - context->last_check.tv_sec) +
        (now.tv_usec - context->last_check.tv_usec) / 1e6;

    double speed = (1 / time_taken_since_last_check) * check_count;

    gettimeofday(&context->last_check, NULL);

    printf("progress: %zu, total time: %f sec, batch time: %f sec, speed: %f "
           "pass/sec.\n",
           context->total_checked_passwords, time_taken,
           time_taken_since_last_check, speed);
}

void get_nth_permutation(size_t *permutation, size_t n, size_t len) {
    for (int i = 0; i < PASSWORD_LENGTH; i++) {
        permutation[i] = 0;
    }
    for (int k = len - 1; k >= 0; k--) {
        permutation[k] = n % ALPHABET_SIZE;
        n /= ALPHABET_SIZE;
    }
}

size_t min(size_t a, size_t b) { return a < b ? a : b; }

char *clone(char buffer[PASSWORD_LENGTH]) {
    char *ptr = (char *)calloc(PASSWORD_LENGTH, sizeof(char));
    memcpy(ptr, buffer, PASSWORD_LENGTH);
    return ptr;
}

#endif
