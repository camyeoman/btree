#include "../btreestore.h"
#include "../btree.h"
#include "./test.h"

void test_store_init(int* passed, int* failed) {
    // Your own testing code here
    struct btree* tree = init_store(4, 4);
    int result = tree->processors == 4
        && tree->branching == 4
        && tree->root != NULL;
    close_store(tree);
    *(result ? passed : failed) += 1;
}

void test_store_freeing(int* passed, int* failed) {
    // Your own testing code here
    struct btree* tree = init_store(3, 3);


    uint32_t encrypt_key[4] = { 1, 2, 3, 4};
    char placeholder[100] = "hello";
    btree_insert(5, placeholder,  4, encrypt_key, 123, tree);
    char buffer[50];
    btree_decrypt(5, buffer, tree);
    printf("buffer '%s'\n", buffer);

    btree_insert(6, "asdfdas",  7, encrypt_key, 123, tree);
    btree_insert(10, "asdfdas", 8, encrypt_key, 123, tree);
    btree_insert(25, "asdfdas", 8, encrypt_key, 123, tree);
    btree_insert(24, "asdfdas", 8, encrypt_key, 123, tree);
    btree_insert(25, "asdfdas", 6, encrypt_key, 123, tree);
    btree_insert(26, "asdfdas", 8, encrypt_key, 123, tree);
    btree_insert(27, "asdfdas", 8, encrypt_key, 123, tree);
    btree_insert(28, "asdfdas", 8, encrypt_key, 123, tree);
    btree_insert(29, "asdfdas", 8, encrypt_key, 123, tree);

    free_node(tree, tree->root->links[1]);

    close_store(tree);

    int result = 1;
    *(result ? passed : failed) += 1;
}

void test_store_insert_retrieve(int* passed, int* failed) {
    struct {
        uint32_t key;
        char message[100];
        uint32_t size;
        uint32_t encrypt_key[4];
        uint64_t nonce;
    } tests[] = {
        { 512, "Don", 3, {0,1,4,5}, 5399 },
        { 320, "Hello", 5, {0,1,4,5}, 5399 },
        { 59,  "Smaller ", 8, {0,1,4,5}, 5399 },
        { 934, "A shorter one", 16, {0,1,4,5}, 5399 },
        { 251, "Prince Gerard ate a horse",     32, {0,1,4,5}, 5399 },
        // { 35,  "Geoffry was a quick brown fox", 32, {0,1,4,5}, 5399 },
    };

    struct btree* tree = init_store(3, 3);

    for (int i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
        btree_insert(
            tests[i].key,
            tests[i].message,
            tests[i].size,
            tests[i].encrypt_key,
            tests[i].nonce,
            tree
        );

        struct info found = { 0, { 0, 0, 0, 0 }, 0, NULL };
        btree_retrieve(tests[i].key, &found, tree);

        void* buffer = malloc(1000);
        btree_decrypt(tests[i].key, buffer, tree);

        int result = 1;
        for (int j=0; j < found.size; j++) {
            if (((char*) buffer)[j] != tests[i].message[j]) {
                result = 0;
                break;
            }
        }

        free(buffer);
        *(result ? passed : failed) += 1;
    }

    close_store(tree);
}
