
#ifndef openmp_impl
#define openmp_impl

#include "globals.h"
#include "md5.c"
#include <bits/pthreadtypes.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <sys/time.h>

struct search_result check_batch(size_t permutation_number, size_t length,
                                 size_t hashes_to_check) {
    char *password = NULL;
    size_t indices[PASSWORD_LENGTH] = {};
    get_nth_permutation(indices, permutation_number, length);

    char buffer[PASSWORD_LENGTH + 1] = {0};
    size_t checked_hashes = 0;
    while (checked_hashes < hashes_to_check) {
        // if (omp_get_thread_num() == 0) {
        //     print_permutation(indices, length);
        // }

        size_t i = 0;
        for (i = 0; i < length; i++) {
            buffer[i] = alphabet[indices[i]];
        }
        buffer[i] = '\0';

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

        if (memcmp(TEST_HASH, result, 16) == 0) {
            password = clone(buffer);
            break;
        }
        memset(buffer, 0, PASSWORD_LENGTH);

        // If we have incremented past the leftmost index, we are done
        if (pos < 0) {
            break;
        }
    }

    struct search_result search_result = {.password = password,
                                          .checked_passwords = checked_hashes};

    return search_result;
}

struct search_result check_hashes(size_t length, size_t hashes_to_check,
                                  struct progress_context *progress_context) {
    size_t total_hashes = powl(ALPHABET_SIZE, PASSWORD_LENGTH);
    hashes_to_check = min(total_hashes, hashes_to_check);
    size_t total_checked_hashes = 0;
    // size_t last_checked_hashes = 0;

    struct search_result search_result = {.password = NULL,
                                          .checked_passwords = 0};

    volatile int finished = 0;

#pragma omp parallel for shared(total_checked_hashes, finished)
    for (size_t n = 0; n <= hashes_to_check / BATCH_SIZE; n++) {
        if (finished == 1) {
            continue;
        }

        size_t permutation_number = BATCH_SIZE * n;
        size_t to_check = min(BATCH_SIZE, hashes_to_check - permutation_number);

        // printf("starting thread %d, total_hashes: %zu, hashes_to_check: "
        //        "%zu, perm_nr: %zu\n",
        //        omp_get_thread_num(), total_hashes, to_check,
        //        permutation_number);

        search_result = check_batch(permutation_number, length, to_check);
#pragma omp atomic
        total_checked_hashes += search_result.checked_passwords;
        if (search_result.password != NULL) {
            finished = 1;
            printf("finished: %p\n", search_result.password);
        }

        // if (omp_get_thread_num() == 0) {
        //     update_progress(progress_context, total_checked_hashes);
        // }
        // if (omp_get_thread_num() == 0 &&
        //     total_checked_hashes - last_checked_hashes > CHECKPOINT_COUNT) {
        //     last_checked_hashes = total_checked_hashes;
        //     print_progres(progress_context);
        // }
    }
    update_progress(progress_context, total_checked_hashes);

    return search_result;
}

struct test_result {
    double time_taken;
    size_t total_checked_hashes;
};

void run_tests(struct test_result *result) {
    struct progress_context progress_context = {0};

    gettimeofday(&progress_context.start, NULL);
    gettimeofday(&progress_context.last_check, NULL);

    // struct search_result search_result =
        check_hashes(PASSWORD_LENGTH, PERMUTATIONS_TO_CHECK, &progress_context);

    // printf("matching password is: %s\n", search_result.password);

    size_t total_checked_hashes = progress_context.total_checked_passwords;

    struct timeval start, end;
    start = progress_context.start;
    gettimeofday(&end, NULL);

    double time_taken =
        (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
    //printf("checked %zu hashes, in %f seconds.\n", total_checked_hashes,
    //      time_taken);
    result->time_taken = time_taken;
    result->total_checked_hashes = total_checked_hashes;
}

#endif
