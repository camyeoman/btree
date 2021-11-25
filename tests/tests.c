#include "test.h"

void test_encryption_simple(int* passed, int* failed);
void test_encryption_ctr(int* passed, int* failed);
void test_btree_key_index(int* passed, int* failed);
void test_btree_insert_key(int* passed, int* failed);
void test_store_init(int* passed, int* failed);
void test_store_freeing(int* passed, int* failed);
void test_btree_basic_insert(int* passed, int* failed);
void test_btree_traversal(int* passed, int* failed);
void test_btree_insert_basic(int* passed, int* failed);
void test_btree_insert_promotion(int* passed, int* failed);
void test_btree_insert_dividing(int* passed, int* failed);
void test_btree_insert_large(int* passed, int* failed);
void test_btree_insert_massive(int* passed, int* failed);
void test_btree_delete_simple(int* passed, int* failed);
void test_btree_delete_collapse(int* passed, int* failed);
void test_btree_delete_complete(int* passed, int* failed);
void test_store_insert_retrieve(int* passed, int* failed);

static struct {
    char message[50];
    void (*function)(int* passed, int* failed);
} TESTS[] = {
    { "ENCRYPTION: simple encryption",    &test_encryption_simple      },
    { "ENCRYPTION: counter encryption",   &test_encryption_ctr         },
    { "INTERNAL BTREE: key index",        &test_btree_key_index        },
    { "INTERNAL BTREE: insert key",       &test_btree_insert_key       },
    { "INTERNAL BTREE: traversal",        &test_btree_traversal        },
    { "STORE BTREE: initialise",          &test_store_init             },
    { "STORE BTREE: freeing",             &test_store_freeing          },
    { "STORE BTREE: basic insert",        &test_btree_insert_basic     },
    { "STORE BTREE: dividing insert",     &test_btree_insert_dividing  },
    { "STORE BTREE: insert promotion",    &test_btree_insert_promotion },
    { "STORE BTREE: large insert",        &test_btree_insert_large     },
    { "STORE BTREE: massive insert",      &test_btree_insert_large     },
    { "STORE BTREE: delete simple",       &test_btree_delete_simple    },
    { "STORE BTREE: delete collapse",     &test_btree_delete_collapse  },
    { "STORE BTREE: delete complete",     &test_btree_delete_complete  },
    { "STORE BTREE: insert and retrive",  &test_store_insert_retrieve  },
};

int main() {
    size_t len = sizeof(TESTS)/sizeof(TESTS[0]);
    int prev_passed, prev_failed;
    int passed = 0, failed = 0;

    for (int i=0; i < len; i++) {
        prev_failed = failed;
        prev_passed = passed;
        TESTS[i].function(&passed, &failed);

        if (failed == prev_failed && passed > prev_passed) {
            char* fmt = "%*s  "COL_GREEN"%*s %d"COL_RESET"\n";
            printf(fmt, -35, TESTS[i].message, 0, "PASSED", passed - prev_passed);
        } else {
            char* fmt = "%*s  "COL_RED"%*s %d"COL_RESET"\n";
            printf(fmt, -35, TESTS[i].message, 0, "FAILED", failed - prev_failed);
        }
    }

    printf("\nResults: ("COL_GREEN"%d"COL_RESET", ", passed);
    printf(COL_RED"%d"COL_RESET")\n", failed);
    return 0;
}

// HELPER FUNCTIONS

int assert_temp_file(char* expected) {
    FILE* file = fopen("bin/output_testing", "r");

    fseek(file, 0, SEEK_END);
    size_t num_chars = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(num_chars + 1);
    buffer[num_chars] = '\0';

    fread(buffer, sizeof(char), num_chars, file);
    fclose(file);

    if (strcmp(buffer, expected) != 0) {
        printf("\nEXPECTED\n");
        printf("----------\n\n");
        printf("%s\n", expected);
        printf("----------------\n");
        printf("\nOUTPUT\n");
        printf("--------\n\n");
        printf("%s\n", buffer);
        printf("----------------\n");
        return 0;
    }

    free(buffer);
    return 1;
}
