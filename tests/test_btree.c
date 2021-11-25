#include "../btree.h"
#include "../btreestore.h"
#include "test.h"

// HELPER FUNCTIONS

int assert_tree(struct bnode* node, char* expected) {
    freopen("bin/output_testing", "w", stdout);
    display(node, "", 1);
    freopen("/dev/tty", "w", stdout);
    return assert_temp_file(expected);
}

// TESTING

void test_btree_key_index(int* passed, int* failed) {
    struct bnode* node = new_node(9, 1);
    int keys[] = { 1, 3, 6, 7, 9, 12, 13, 18, 21 };
    for (int i=0; i < 9; i++) {
        node->keys[i].key = keys[i];
        node->num_keys += 1;
    }

    struct {
        int new_key;
        int expected_index;
    } tests[] = {
        { .new_key = 0,  .expected_index = 0 },
        { .new_key = 1,  .expected_index = 0 },
        { .new_key = 2,  .expected_index = 1 },
        { .new_key = 3,  .expected_index = 1 },
        { .new_key = 4,  .expected_index = 2 },
        { .new_key = 5,  .expected_index = 2 },
        { .new_key = 6,  .expected_index = 2 },
        { .new_key = 7,  .expected_index = 3 },
        { .new_key = 8,  .expected_index = 4 },
        { .new_key = 9,  .expected_index = 4 },
        { .new_key = 10, .expected_index = 5 },
        { .new_key = 11, .expected_index = 5 },
        { .new_key = 14, .expected_index = 7 },
        { .new_key = 15, .expected_index = 7 },
        { .new_key = 16, .expected_index = 7 },
        { .new_key = 17, .expected_index = 7 },
        { .new_key = 18, .expected_index = 7 },
        { .new_key = 19, .expected_index = 8 },
        { .new_key = 20, .expected_index = 8 },
        { .new_key = 21, .expected_index = 8 },
        { .new_key = 22, .expected_index = 9 },
        { .new_key = 99, .expected_index = 9 },
    };

    for (int i=0; i < sizeof(tests)/sizeof(tests[0]); i++) {
        uint32_t key = tests[i].new_key;
        int expected = tests[i].expected_index;

        int result_index = key_index(node, key, 0, 8);
        int test_result = result_index == expected;

        if (!test_result) {
            fprintf(stderr, "key_index %d -> Expect %d got %d\n", i,
                expected,
                result_index
            );
        }

        *(test_result ? passed : failed) += 1;
    }

    free(node->keys);
    free(node);
}

void test_btree_insert_key(int* passed, int* failed) {
    struct btree tree = { 10, 1, new_node(10, 1) };
    struct bnode* node = tree.root;

    struct {
        int ret;
        int new_key;
        int keys[50];
    } tests[] = {
        { .ret = 0,  .new_key = 7,  .keys = { 7 } },
        { .ret = 0,  .new_key = 3,  .keys = { 3, 7 } },
        { .ret = 1,  .new_key = 6,  .keys = { 3, 6, 7 } },
        { .ret = 0,  .new_key = 1,  .keys = { 1, 3, 6, 7 } },
        { .ret = 2,  .new_key = 5,  .keys = { 1, 3, 5, 6, 7 } },
        { .ret = 2,  .new_key = 4,  .keys = { 1, 3, 4, 5, 6, 7 } },
        { .ret = 0,  .new_key = 0,  .keys = { 0, 1, 3, 4, 5, 6, 7 } },
        { .ret = 7,  .new_key = 12, .keys = { 0, 1, 3, 4, 5, 6, 7, 12 } },
        { .ret = 8,  .new_key = 13, .keys = { 0, 1, 3, 4, 5, 6, 7, 12, 13 } },
        { .ret = 9,  .new_key = 22, .keys = { 0, 1, 3, 4, 5, 6, 7, 12, 13, 22 } },
    };

    for (int j=0; j < sizeof(tests)/sizeof(tests[0]); j++) {
        struct key_value new = { tests[j].new_key };
        int ret = tests[j].ret;

        int index = insert_key(node, &new);
        int test_result = index == ret;

        if (test_result) {
            for (int i=0; i < node->num_keys; i++) {
                if (tests[j].keys[i] != node->keys[i].key) {
                    test_result = 0;
                }
            }
        }

        *(test_result ? passed : failed) += 1;
    }

    free_node(&tree, node);
}

struct insert_test {
    int length;
    int branching;
    int sequence[50];
    char* expected;
};

struct delete_test {
    int length;
    int sequence[50];
    char* expected;
};

int wrap_tree_insert(struct btree* tree, uint32_t key) {
    uint32_t encryption_key[4] = { 0, 1, 2, 3 };
    int result = btree_insert(key, "asdf", 0, encryption_key, 123, tree);
    return result;
}

int test_insert_sequence(struct insert_test* test, int print) {
    struct btree* tree = init_store(test->branching, 1);

    for (int i = 0; i < test->length; i++) {
        wrap_tree_insert(tree, test->sequence[i]);
        if (print) {
            display(tree->root, "", 1);
        }
    }

    int result = assert_tree(tree->root, test->expected);
    close_store(tree);
    return result;
}

int test_delete_sequence(struct insert_test* insert, struct delete_test* test) {
    struct btree* tree = init_store(insert->branching, 1);
    for (int i = 0; i < insert->length; i++) {
        wrap_tree_insert(tree, insert->sequence[i]);
    }

    for (int i = 0; i < test->length; i++) {
        // fprintf(stderr, "\n\ndeleting %d\n", test->sequence[i]);
        // debug(tree->root, "", 1);
        btree_delete(test->sequence[i], tree);
        // debug(tree->root, "", 1);
        // fprintf(stderr, "\n\n");
    }

    int result = assert_tree(tree->root, test->expected);
    close_store(tree);
    return result;
}

// INSERT SEQUENCES

void test_btree_insert_basic(int* passed, int* failed) {
    struct insert_test test = {
        .length = 12,
        .branching = 3,
        .sequence = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
        .expected = " └─ (3, 7)"            "\n"
                    "     ├─ (1)"           "\n"
                    "     |   ├─ (0)"       "\n"
                    "     |   └─ (2)"       "\n"
                    "     ├─ (5)"           "\n"
                    "     |   ├─ (4)"       "\n"
                    "     |   └─ (6)"       "\n"
                    "     └─ (9)"           "\n"
                    "         ├─ (8)"       "\n"
                    "         └─ (10, 11)"  "\n"
    };

    *(test_insert_sequence(&test, 0) ? passed : failed) += 1;
}

void test_btree_insert_dividing(int* passed, int* failed) {
    struct insert_test test = {
        .length = 10,
        .branching = 4,
        .sequence = { 3, 7, 13, 2, 5, 11, 17, 19, 20, 21 },
        .expected = " └─ (7)"               "\n"
                    "     ├─ (3)"           "\n"
                    "     |   ├─ (2)"       "\n"
                    "     |   └─ (5)"       "\n"
                    "     └─ (13, 19)"      "\n"
                    "         ├─ (11)"      "\n"
                    "         ├─ (17)"      "\n"
                    "         └─ (20, 21)"  "\n"
    };

    *(test_insert_sequence(&test, 0) ? passed : failed) += 1;
}

void test_btree_insert_promotion(int* passed, int* failed) {
    struct insert_test test = {
        .length = 9,
        .branching = 3,
        .sequence = { 5, 55, 91, 115, 46, 13, 8, 53, 72  },
        .expected = " └─ (55)"              "\n"
                    "     ├─ (13)"          "\n"
                    "     |   ├─ (5, 8)"    "\n"
                    "     |   └─ (46, 53)"  "\n"
                    "     └─ (91)"          "\n"
                    "         ├─ (72)"      "\n"
                    "         └─ (115)"     "\n"

    };

    *(test_insert_sequence(&test, 0) ? passed : failed) += 1;
}

void test_btree_insert_large(int* passed, int* failed) {
    struct insert_test test = {
        .length = 14,
        .branching = 4,
        .sequence = { 3, 7, 13, 2, 5, 11, 17, 19, 20, 21, 22, 25, 26, 27 },
        .expected = " └─ (7, 19)"           "\n"
                    "     ├─ (3)"           "\n"
                    "     |   ├─ (2)"       "\n"
                    "     |   └─ (5)"       "\n"
                    "     ├─ (13)"          "\n"
                    "     |   ├─ (11)"      "\n"
                    "     |   └─ (17)"      "\n"
                    "     └─ (21, 25)"      "\n"
                    "         ├─ (20)"      "\n"
                    "         ├─ (22)"      "\n"
                    "         └─ (26, 27)"  "\n"
    };

    *(test_insert_sequence(&test, 0) ? passed : failed) += 1;
}

void test_btree_insert_massive(int* passed, int* failed) {
    struct insert_test test = {
        .length = 22,
        .branching = 4,
        .sequence = {
            3, 7, 13, 2, 5, 11, 17, 19, 20, 21, 22, 25, 26,
            27, 28, 29, 45, 55, 99, 60, 57, 56
        },
        .expected = " └─ (19)"                      "\n"
                    "     ├─ (7)"                   "\n"
                    "     |   ├─ (3)"               "\n"
                    "     |   |   ├─ (2)"           "\n"
                    "     |   |   └─ (5)"           "\n"
                    "     |   └─ (13)"              "\n"
                    "     |       ├─ (11)"          "\n"
                    "     |       └─ (17)"          "\n"
                    "     └─ (25, 29)"              "\n"
                    "         ├─ (21)"              "\n"
                    "         |   ├─ (20)"          "\n"
                    "         |   └─ (22)"          "\n"
                    "         ├─ (27)"              "\n"
                    "         |   ├─ (26)"          "\n"
                    "         |   └─ (28)"          "\n"
                    "         └─ (55, 57)"          "\n"
                    "             ├─ (45)"          "\n"
                    "             ├─ (56)"          "\n"
                    "             └─ (60, 99)"      "\n"
    };

    *(test_insert_sequence(&test, 0) ? passed : failed) += 1;
}

// DELETE SEQUENCES

void test_btree_delete_simple(int* passed, int* failed) {
    struct insert_test insert = {
        .length = 10,
        .branching = 4,
        .sequence = { 3, 7, 13, 2, 5, 11, 17, 19, 20, 21 },
    };

    struct delete_test test = {
        .sequence = { 3 },
        .length = 1,
        .expected = " └─ (13)"              "\n"
                    "     ├─ (7)"           "\n"
                    "     |   ├─ (2, 5)"    "\n"
                    "     |   └─ (11)"      "\n"
                    "     └─ (19)"          "\n"
                    "         ├─ (17)"      "\n"
                    "         └─ (20, 21)"  "\n"
    };

    *(test_delete_sequence(&insert, &test) ? passed : failed) += 1;
}

void test_btree_delete_collapse(int* passed, int* failed) {
    struct insert_test insert = {
        .length = 10,
        .branching = 4,
        .sequence = { 3, 7, 13, 2, 5, 11, 17, 19, 20, 21 },
    };

    struct delete_test test = {
        .sequence = { 3, 5, 2 },
        .length = 3,
        .expected = " └─ (13, 19)"      "\n"
                    "     ├─ (7, 11)"   "\n"
                    "     ├─ (17)"      "\n"
                    "     └─ (20, 21)"  "\n"
    };

    *(test_delete_sequence(&insert, &test) ? passed : failed) += 1;
}

void test_btree_delete_complete(int* passed, int* failed) {
    struct insert_test insert = {
        .length = 10,
        .branching = 4,
        .sequence = { 3, 7, 13, 2, 5, 11, 17, 19, 20, 21 },
    };

    struct delete_test test = {
        .sequence = { 3, 5, 2, 20, 21, 17, 11, 7, 13, 19 },
        .length = 10,
        .expected = " └─ ()"      "\n"
    };

    *(test_delete_sequence(&insert, &test) ? passed : failed) += 1;
}

void test_btree_traversal(int* passed, int* failed) {
    struct insert_test insert = {
        .length = 10,
        .branching = 4,
        .sequence = { 3, 7, 13, 2, 5, 11, 17, 19, 20, 21 },
        .expected = " └─ (7)"               "\n"
                    "     ├─ (3)"           "\n"
                    "     |   ├─ (2)"       "\n"
                    "     |   └─ (5)"       "\n"
                    "     └─ (13, 19)"      "\n"
                    "         ├─ (11)"      "\n"
                    "         ├─ (17)"      "\n"
                    "         └─ (20, 21)"  "\n"
    };

    struct btree* tree = init_store(4, 4);
    for (int i = 0; i < insert.length; i++) {
        wrap_tree_insert(tree, insert.sequence[i]);
    }

    struct node* list = malloc(tree->num_nodes * sizeof(struct node));

    uint32_t expected[][4] = {
        { 7,   0, 0, 0 },
        { 3,   0, 0, 0 },
        { 2,   0, 0, 0 },
        { 5,   0, 0, 0 },
        { 13, 19, 0, 0 },
        { 11,  0, 0, 0 },
        { 17,  0, 0, 0 },
        { 20, 21, 0, 0 }
    };

    uint64_t size = listing_nodes(tree->root, list);
    int result = size == 8;

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < list[i].num_keys; j++) {
            result = result && (expected[i][j] == list[i].keys[j]);
        }

        free(list[i].keys);
    }

    free(list);

    *(result ? passed : failed) += 1;
}
