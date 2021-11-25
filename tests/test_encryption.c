#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../btreestore.h"
#include "test.h"

void test_encryption_simple(int* passed, int* failed) {
    uint32_t plaintext[2], cipher[2], result[2];
    uint32_t encryption_key[4] = {12,34,56,78};

    char* strings[] = {
        "Chris",
        "Jacob",
        "Wayne",
        "$@#$_+1",
        "*((I)()",
    };

    for (int i=0; i < sizeof(strings)/sizeof(strings[0]); i++) {
        strcpy((char*) plaintext, strings[i]);
        encrypt_tea(plaintext, cipher, encryption_key);
        decrypt_tea(cipher, result, encryption_key);

        int test_result = !strcmp((char*) plaintext, (char*) result);

        if (!test_result) {
            fprintf(stderr, "encryption_simple %d -> Expect '%s' got '%s'\n", i,
                (char*) plaintext,
                (char*) result
            );
        }

        *(test_result ? passed : failed) += 1;
    }
}

int test_encryption_ctr(int* passed, int* failed) {
    uint32_t encryption_key[4] = {12,34,56,78};
    uint64_t nonce = 111;

    uint64_t plaintext[10];
    uint64_t cipher[10];
    uint64_t result[10];
    char* strings[] = {
        "hello world",
        "#@$%!#$.asdfasadf2+_!  !41 4   ",
        "#@$%!#$.assafojjl$%!#$asjdlk$%!#$fjklsjlfjass__)()(+)+lkdkflaskldjfkljd"
    };
    

    for (int i=0; i < sizeof(strings)/sizeof(strings[0]); i++) {
        strcpy((char*) plaintext, strings[i]);
        encrypt_tea_ctr(plaintext, encryption_key, nonce, cipher, 10);

        decrypt_tea_ctr(cipher, encryption_key, nonce, result, 10);

        int test_result = !strcmp((char*) plaintext, (char*) result);
        if (!test_result) {
            fprintf(stderr, "encryption_simple %d -> Expect '%s' got '%s'\n", i,
                (char*) plaintext,
                (char*) result
            );
        }

        *(test_result ? passed : failed) += 1;
    }

    return 1;
}
