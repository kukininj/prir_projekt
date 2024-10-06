#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "md5.c"

// #define MAX_PASSWORD_LENGTH 4
// #define ALPHABET_SIZE 4
// const char *alphabet = "abcd";

#define MAX_PASSWORD_LENGTH 6
#define ALPHABET_SIZE 26
const char *alphabet = "abcdefghijklmnopqrstuvwxyz";
const size_t CHECK_COUNT = 1 << 22;

void print_permutation(size_t *indeces, size_t len) {
    for (int i = 0; i < len; i++) {
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

char *clone(char buffer[MAX_PASSWORD_LENGTH]) {
    char *ptr = (char *)calloc(MAX_PASSWORD_LENGTH, sizeof(char));
    memcpy(ptr, buffer, MAX_PASSWORD_LENGTH);
    return ptr;
}

void print_progres(struct timeval *start, struct timeval *last_check,
                   size_t checked_hashes) {
    struct timeval now;

    gettimeofday(&now, NULL);
    double time_taken =
        (now.tv_sec - start->tv_sec) + (now.tv_usec - start->tv_usec) / 1e6;

    double time_taken_since_last_check =
        (now.tv_sec - last_check->tv_sec) +
        (now.tv_usec - last_check->tv_usec) / 1e6;

    double speed = (1 / time_taken_since_last_check) * CHECK_COUNT;

    gettimeofday(last_check, NULL);

    printf("progress: %zu, total time: %f sec, batch time: %f sec, speed: %f "
           "pass/sec.\n",
           checked_hashes, time_taken, time_taken_since_last_check, speed);
}

size_t *get_nth_permutation(size_t n, size_t len) {
    size_t *permutation = (size_t *)calloc(MAX_PASSWORD_LENGTH, sizeof(size_t));
    for (int i = 0; i < MAX_PASSWORD_LENGTH; i++) {
        permutation[i] = 0;
    }
    for (int k = len - 1; k >= 0; k--) {
        permutation[k] = n % ALPHABET_SIZE;
        n /= ALPHABET_SIZE;
    }

    return permutation;
}

char *check_hashes(uint8_t actual_password_hash[16], size_t *indices,
                   size_t length, size_t hashes_to_check,
                   size_t *total_checked_hashes) {
    char buffer[MAX_PASSWORD_LENGTH + 1] = {0};
    size_t checked_hashes = 0;
    while (checked_hashes < hashes_to_check) {
        int i = 0;
        for (i = 0; i < length; i++) {
            buffer[i] = alphabet[indices[i]];
        }
        buffer[i] = '\0';

        // printf("%s, thread: %d\n", buffer,omp_get_thread_num());

        // Increment the rightmost index
        int pos = length - 1;
        while (pos >= 0) {
            indices[pos]++;
            if (indices[pos] < ALPHABET_SIZE) {
                break;
            }
            // If the current index exceeds the alphabet size, reset it
            // and move to the next index to the left
            indices[pos] = 0;
            pos--;
        }

        uint8_t result[16];
        md5String(buffer, length, result);
        checked_hashes++;

        if (memcmp(actual_password_hash, result, 16) == 0) {
#pragma omp atomic
            total_checked_hashes += checked_hashes & 0xFF;

            return clone(buffer);
        }
        memset(buffer, 0, MAX_PASSWORD_LENGTH);

        if ((checked_hashes & 0xFF) == 0) {
#pragma omp atomic
            *total_checked_hashes += 0xFF;
        }

        // If we have incremented past the leftmost index, we are done
        if (pos < 0) {
            break;
        }
    }
#pragma omp atomic
    total_checked_hashes += checked_hashes & 0xFF;

    return NULL;
}

size_t min(size_t a, size_t b) { return a < b ? a : b; }

int main(int argc, char *argv[]) {
    char *actual_password = "zzzzza";
    if (strlen(actual_password) > MAX_PASSWORD_LENGTH) {
        printf("Invalid password lenght\n");
        exit(0);
    }
    uint8_t actual_password_hash[16];
    md5String(actual_password, strlen(actual_password), actual_password_hash);
    print_hash(actual_password_hash);

    size_t *indices = NULL;
    size_t alphabet_size = strlen(alphabet);
    size_t total_checked_hashes = 0;
    size_t last_checked_hashes = 0;
    struct timeval start, end, checkpoint;

    gettimeofday(&start, NULL);
    gettimeofday(&checkpoint, NULL);

    for (size_t length = 1; length <= MAX_PASSWORD_LENGTH; length++) {
        size_t total_hashes = powl(ALPHABET_SIZE, length);
        size_t hashes_to_check = 0x100000;
        // size_t hashes_to_check = 0x8;
#pragma omp parallel for num_threads(4) shared(total_checked_hashes)
        for (size_t n = 0; n <= total_hashes / hashes_to_check; n++) {
            size_t permutation_number = hashes_to_check * n;
            size_t to_check =
                min(hashes_to_check, total_hashes - permutation_number);

            printf("starting thread %d, total_hashes: %zu, hashes_to_check: "
                   "%zu, perm_nr: %zu\n",
                   omp_get_thread_num(), total_hashes, to_check,
                   permutation_number);
            indices = get_nth_permutation(permutation_number, length);

            char *password =
                check_hashes(actual_password_hash, indices, length,
                             to_check, &total_checked_hashes);
            if (password != NULL) {
                printf("matching password is: %s\n", password);
                gettimeofday(&end, NULL);
                double time_taken = (end.tv_sec - start.tv_sec) +
                                    (end.tv_usec - start.tv_usec) / 1e6;
                printf("checked %zu hashes, in %f seconds.\n",
                       total_checked_hashes, time_taken);
                exit(0);
            }

            if (omp_get_thread_num() == 0 &&
                total_checked_hashes - last_checked_hashes > CHECK_COUNT) {
                last_checked_hashes = total_checked_hashes;
                print_progres(&start, &checkpoint, total_checked_hashes);
            }
        }
    }
}
