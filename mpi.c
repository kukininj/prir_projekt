#include "globals.h"
#include "md5.c"
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

struct search_result check_batch(size_t permutation_number, size_t length,
                                 size_t hashes_to_check) {
    char *password = NULL;
    size_t indices[PASSWORD_LENGTH] = {};
    get_nth_permutation(indices, permutation_number, length);

    char buffer[PASSWORD_LENGTH + 1] = {0};
    size_t checked_hashes = 0;
    while (checked_hashes < hashes_to_check) {
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

        if (pos < 0) {
            break;
        }
    }

    struct search_result search_result = {.password = password,
                                          .checked_passwords = checked_hashes};
    return search_result;
}

void run_tests(int rank, int size, struct test_result *result) {
    size_t hashes_per_process = PERMUTATIONS_TO_CHECK / size;

    struct progress_context progress_context = {0};
    gettimeofday(&progress_context.start, NULL);

    size_t start = rank * hashes_per_process;
    size_t end = (rank == size - 1) ? PERMUTATIONS_TO_CHECK
                                    : (start + hashes_per_process);
    size_t total_checked_hashes = 0;

    struct search_result search_result = {.password = NULL,
                                          .checked_passwords = 0};

    int i = 0;

    for (size_t permutation_number = start; permutation_number < end;
         permutation_number += BATCH_SIZE) {
        size_t to_check = min(BATCH_SIZE, end - permutation_number);

        search_result =
            check_batch(permutation_number, PASSWORD_LENGTH, to_check);
        total_checked_hashes += search_result.checked_passwords;

        // if (i % 10 * BATCH_SIZE == 0) {
        //     printf("%zu:%zu:%zu\n", start, permutation_number, end);
        // }
        // i++;

        if (search_result.password != NULL) {
            break;
        }
    }

    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    double time_taken =
        (end_time.tv_sec - progress_context.start.tv_sec) +
        (end_time.tv_usec - progress_context.start.tv_usec) / 1e6;

    result->time_taken = time_taken;
    result->total_checked_hashes = total_checked_hashes;

    if (search_result.password != NULL) {
        printf("Process %d found the password: %s\n", rank,
               search_result.password);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    struct test_result local_result = {0}, global_result = {0};
    run_tests(rank, size, &local_result);
    double total_time = local_result.time_taken;

    MPI_Reduce(&local_result.total_checked_hashes,
               &global_result.total_checked_hashes, 1, MPI_UNSIGNED_LONG,
               MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&total_time, &global_result.time_taken, 1, MPI_DOUBLE, MPI_MAX,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("%dx1,%lf,%zu\n", size, global_result.time_taken, global_result.total_checked_hashes);
        // printf("Average time: %lf seconds, Total hashes checked: %zu\n",
        //        global_result.time_taken, global_result.total_checked_hashes);
        // printf("%lf hs/s\n", global_result.time_taken /
        //                          (double)global_result.total_checked_hashes);
    }

    MPI_Finalize();
    return 0;
}
