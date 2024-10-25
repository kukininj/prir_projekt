#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "openmp.c"


int main(int argc, char *argv[]) {
    char *actual_password = "zzzzza";
    if (strlen(actual_password) > PASSWORD_LENGTH) {
        printf("Invalid password lenght\n");
        exit(0);
    }
    uint8_t actual_password_hash[16];
    md5String(actual_password, strlen(actual_password), actual_password_hash);
    print_hash(actual_password_hash);

    run_tests();
}
