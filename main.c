#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "md5.c"

#define MAX_PASSWORD_LENGTH 6

const size_t CHECK_COUNT = 1<<22;
const char *alphabet = "abcdefghijklmnopqrstuvwxyz";

void print_hash(uint8_t *p) {
    for (unsigned int i = 0; i < 16; ++i) {
        printf("%02x", p[i]);
    }
    printf("\n");
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

int main(int argc, char *argv[]) {
    char *actual_password = "zzzzzz";
    if (strlen(actual_password) > MAX_PASSWORD_LENGTH) {
        printf("Invalid password lenght\n");
        exit(0);
    }
    uint8_t actual_password_hash[16];
    md5String(actual_password, strlen(actual_password), actual_password_hash);
    print_hash(actual_password_hash);

    size_t indices[MAX_PASSWORD_LENGTH] = {0};
    size_t alphabet_size = strlen(alphabet);
    size_t generated_hashes = 0;
    struct timeval start, end, checkpoint;

    gettimeofday(&start, NULL);
    gettimeofday(&checkpoint, NULL);

    for (size_t length = 0; length <= MAX_PASSWORD_LENGTH; length++) {
        char buffer[MAX_PASSWORD_LENGTH] = {0};
        while (1) {
            for (int i = 0; i < length; i++) {
                buffer[i] = alphabet[indices[i]];
            }

            // Increment the rightmost index
            int pos = length - 1;
            while (pos >= 0) {
                indices[pos]++;
                if (indices[pos] < alphabet_size) {
                    break;
                }
                // If the current index exceeds the alphabet size, reset it and
                // move to the next index to the left
                indices[pos] = 0;
                pos--;
            }

            uint8_t result[16];
            md5String(buffer, length, result);
            generated_hashes++;
            if (memcmp(actual_password_hash, result, 16) == 0) {
                gettimeofday(&end, NULL);
                double time_taken = (end.tv_sec - start.tv_sec) +
                                    (end.tv_usec - start.tv_usec) / 1e6;
                print_hash(result);
                printf("checked %zu hashes, in %f seconds.\n", generated_hashes,
                       time_taken);
                return 0;
            }
            memset(buffer, 0, MAX_PASSWORD_LENGTH);

            if ((generated_hashes & CHECK_COUNT - 1) == 0) {
                print_progres(&start, &checkpoint, generated_hashes);
            }

            // If we have incremented past the leftmost index, we are done
            if (pos < 0) {
                break;
            }
        }
    }
}
