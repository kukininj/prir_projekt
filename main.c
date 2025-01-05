#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "md5.c"

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Specify threads number\n");
        return 1;
    }

    int threads = atoi(argv[1]);
    omp_set_num_threads(threads);

    char *actual_password = "zzzzza";
    if (strlen(actual_password) > PASSWORD_LENGTH) {
        printf("Invalid password lenght\n");
        exit(0);
    }

    uint8_t actual_password_hash[16];
    md5String(actual_password, strlen(actual_password), actual_password_hash);
    print_hash(actual_password_hash);
}
