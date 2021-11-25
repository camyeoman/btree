#include "btreestore.h"
#include "btree.h"

void print_links(struct bnode* node, int size, char* msg);
void print_keys(struct bnode* node, int size, char* msg);

uint32_t displace(uint32_t value, uint32_t sum, uint32_t key[2]) {
    return ((value << 4) + key[0]) % POWER_32
         ^ (    (value + sum)    ) % POWER_32
         ^ ((value >> 5) + key[1]) % POWER_32;
}

void* init_store(uint16_t branching, uint8_t n_processors) {
    struct btree* tree = malloc(sizeof(struct btree));
    tree->root = new_node(branching, 1);
    tree->processors = n_processors;
    tree->branching = branching;
    tree->num_nodes = 0;
    return tree;
}

void close_store(void * helper) {
    struct btree* tree = helper;

    free_node(tree, tree->root);
    free(tree);
    return;
}

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void* helper) {
    struct search_result search = { NULL, -1 };
    struct btree* tree = helper;

    if (!find_key(tree, tree->root, key, &search)) {
        // fill in key_value struct
        struct key_value item = {
            .key = key,
            .info = {
                .key = { 0, 0, 0, 0 },
                .nonce = nonce,
                .size = count,
                .data = NULL
            }
        };

        memcpy(item.info.key, encryption_key, sizeof(uint32_t) * 4);

        // encrypt data
        if (count > 0) {
            int padded = count + (count % 8 > 0 ? (8 - (count % 8)) : 0);
            item.info.data = malloc(padded);
            int num_blocks = padded / 8;

            // copy plaintext to padded text, padding it will null
            // terminating characters
            uint64_t* text = malloc(padded);
            for (int i=0; i < padded; i++) {
                ((char*) text)[i] = (i < count ? ((char*) plaintext)[i] : '\0');
            }

            encrypt_tea_ctr(text, encryption_key, nonce, item.info.data, num_blocks);
            free(text);
        }

        // insert data into given node
        insert_key(search.node, &item);
        divide(tree, search.node);
        tree->num_nodes += 1;

        return 0;
    } else {
        fprintf(stderr, "FAILED TO FIND INSERT\n");
        return 1;
    }
}

int btree_retrieve(uint32_t key, struct info* found, void* helper) {
    struct search_result search = { NULL, -1 };
    struct btree* tree = helper;

    if (find_key(tree, tree->root, key, &search)) {
        struct key_value* stored = &search.node->keys[search.index];
        *found = stored->info;
        return 0;
    } else {
        return 1;
    }
}

int btree_decrypt(uint32_t key, void* output, void* helper) {
    struct info result = { 0, { 0, 0, 0, 0 }, 0, NULL };
    struct btree* tree = helper;

    if (btree_retrieve(key, &result, tree) == 0) {
        int padded = result.size;
        padded += (padded % 8 > 0 ? (8 - (padded % 8)) : 0);

        void* buffer = malloc(padded);
        void* copy = malloc(padded);
        memcpy(copy, result.data, padded);

        decrypt_tea_ctr(
            copy,
            result.key,
            result.nonce,
            buffer,
            padded / 8
        );

        memcpy(output, buffer, result.size);

        free(copy);
        free(buffer);
        return 0;
    } else {
        return 1;
    }
}

int btree_delete(uint32_t key, void* helper) {
    struct search_result search = { NULL, -1 };
    struct btree* tree = helper;

    if (find_key(tree, tree->root, key, &search)) {
        struct bnode* target = search.node;

        // free the data associated with the previous key

        if (!target->leaf) {
            struct bnode* subnode = target;
            while (subnode && !subnode->leaf) {
                subnode = subnode->links[subnode->num_keys];
            }

            // move largest subkey in the left subtree into target
            struct key_value* destination = &target->keys[search.index];
            free(destination->info.data);
            take_key(subnode, subnode->num_keys-1, destination);
            target = subnode;
        } else {
            take_key(target, search.index, NULL);
        }

        merge(tree, target);
        tree->num_nodes -= 1;
        return 1;
    } else {
        return 0;
    }
}

uint64_t btree_export(void* helper, struct node** list) {
    struct btree* tree = helper;
    *list = malloc(tree->num_nodes * sizeof(struct node));
    return listing_nodes(tree->root, *list);
}

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]) {
    uint32_t v0 = plain[0], v1 = plain[1];
    uint32_t sum = 0;

    for (int i=0; i < 1024; i++) {
        sum = (sum + DELTA) % POWER_32;
        v0 = (v0 + displace(v1, sum, key + 0)) % POWER_32;
        v1 = (v1 + displace(v0, sum, key + 2)) % POWER_32;
    }

    // store results
    cipher[0] = v0;
    cipher[1] = v1;
}

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]) {
    uint32_t v0 = cipher[0], v1 = cipher[1];
    uint32_t sum = DECRYPT_SUM;

    for (int i=0; i < 1024; i++) {
        v1 = (v1 - displace(v0, sum, key + 2)) % POWER_32;
        v0 = (v0 - displace(v1, sum, key + 0)) % POWER_32;
        sum = (sum - DELTA) % POWER_32;
    }

    // store results
    plain[0] = v0;
    plain[1] = v1;
}

void encrypt_tea_ctr(uint64_t* plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks) {
    uint64_t a, b;
    for (int i=0; i < num_blocks; i++) {
        a = i ^ nonce;
        encrypt_tea((uint32_t*) &a, (uint32_t*) &b, key);
        cipher[i] = plain[i] ^ b;
    }
}

void decrypt_tea_ctr(uint64_t* cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks) {
    uint64_t a, b;
    for (int i=num_blocks-1; i >= 0; i--) {
        a = i ^ nonce;
        encrypt_tea((uint32_t*) &a, (uint32_t*) &b, key);
        plain[i] = cipher[i] ^ b;
    }
}
