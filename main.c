#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.c"

#define MAX_PASSWORD_LENGTH 6

const char alphabet[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                         'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                         's', 't', 'u', 'v', 'v', 'w', 'x', 'y', 'z'};

void print_hash(uint8_t *p) {
    for (unsigned int i = 0; i < 16; ++i) {
        printf("%02x", p[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    char *actual_password = "asdfg";
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

            // If we have incremented past the leftmost index, we are done
            if (pos < 0) {
                break;
            }
            uint8_t result[16];
            md5String(buffer, length, result);
            generated_hashes++;
            if (memcmp(actual_password_hash, result, 16) == 0) {
                print_hash(result);
                printf("checked %zu hashes.\n", generated_hashes);
                return 0;
            }
            memset(buffer, 0, MAX_PASSWORD_LENGTH);
        }
    }
}
