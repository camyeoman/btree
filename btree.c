#include "btree.h"

// HELPER FUNCTIONS

static void swap_keys(struct key_value* a, struct key_value* b) {
    struct key_value temp = *a;
    *a = *b;
    *b = temp;
}

static void swap_links(struct bnode** a, struct bnode** b) {
    struct bnode** temp = a;
    *a = *b;
    *b = *temp;
}

/**
 * The function has two modes, switched to by the variable bounds. If bounds
 * is true then the function behaves as a normal binary search returning the
 * index of the key being searched for. Else it returns the index where an
 * element would be inserted to maintain sorted order in the keylist.
 */
int key_index(struct bnode* node, uint32_t key, int l, int r) {
    int m = (l + r) / 2;
    struct key_value* items = node->keys;
    int greater = key > items[m].key && node->num_keys > 0;

    if (key == items[m].key || l > r) {
        return (greater && l > r) ? m + 1 : m;
    } else {
        return key_index(node, key,
            (greater) ? m + 1 : l,
            (greater) ? r : m - 1
        );
    }
}

/**
 * Inserts a key_value from the keys list of a node such that they remain in
 * sorted order, and returns the index at which the key_value item was inserted.
 */
int insert_key(struct bnode* node, struct key_value* item) {
    if (node->num_keys == 0) {
        node->keys[0] = *item;
        node->num_keys = 1;
        return 0;
    }

    int index = key_index(node, item->key, 0, node->num_keys-1);
    int contained = node->keys[index].key == item->key;

    if (!contained) {
        node->num_keys += 1;
        struct key_value previous = *item;
        for (int i = index; i < node->num_keys; i++) {
            swap_keys(&node->keys[i], &previous);
        }

        return index;
    } else {
        return -1;
    }
}

/**
 * Inserts a link at the given index into the node links list.
 */
void insert_link(struct bnode* node, struct bnode* link, int index) {
    for (int i = index; i+1 < node->num_keys + 1; i++) {
        node->links[i+1] = node->links[i];
    }
}

void take_key(struct bnode* node, int index, struct key_value* buffer) {
    if (buffer) {
        *buffer = node->keys[index];
    } else if (node->keys[index].info.data != NULL) {
        free(node->keys[index].info.data);
        node->keys[index].info.data = NULL;
    }

    for (int i = index; i < node->num_keys; i++) {
        if (i + 1 < node->num_keys) {
            node->keys[i] = node->keys[i+1];
        } else {
            memset(&node->keys[i], 0, sizeof(struct key_value));
        }
    }

    node->num_keys -= 1;
}

void take_link(struct btree* tree, struct bnode* node, int index, struct bnode** buffer) {
    struct bnode* target = node->links[index];
    // debug(tree->root, "", 1);
    for (int i=0; i + index + 1 < tree->branching + 1; i++) {
        node->links[i + index] = node->links[i + index + 1];
        if (node->links[i]) {
            node->links[i]->link_index = i;
            node->links[i]->parent = node;
        }
    }

    if (buffer) {
        *buffer = node->links[index];
    } else if (node->links[index]) {
        free_node(tree, target);
    }
}

int edge_index(struct bnode* node, enum Direction side, int is_key) {
    if (node->num_keys == 0) {
        return 0;
    } else {
        return (side == LEFT) ? 0 : (node->num_keys + (is_key ? -1 : 0));
    }
}

// CORE FUNCTIONALITY
     
struct btree* new_tree(uint32_t branching_factor) {
    struct btree* tree = malloc(sizeof(struct btree));
    tree->branching = branching_factor;
    tree->root = NULL;
    return tree;
}

/**
 * Finds the parent node of the key and the index for that key, and stores
 * the results in a search_result struct. Returns whether key was found.
 */
int find_key(struct btree* tree, struct bnode* node, uint32_t key, struct search_result* result) {
    if (node->num_keys == 0) {
        result->node = node;
        result->index = 0;
        return 0;
    }

    int index = key_index(node, key, 0, node->num_keys-1);
    if (node->keys[index].key == key || node->leaf) {
        result->index = index;
        result->node = node;
        return node->keys[index].key == key || (!node->num_keys);
    } else {
        return find_key(
            tree,
            node->links[index],
            key,
            result
        );
    }
}

// TREE INSERTION

struct bnode* split_node(struct btree* tree, struct bnode* node) {
    int median = (node->num_keys - (node->num_keys+1) % 2) / 2;
    struct bnode* split = new_node(tree->branching, node->leaf);

    // update properties
    split->num_keys = node->num_keys - (median + 1);
    split->parent = node->parent;
    split->leaf = node->leaf;
    node->num_keys = median;

    // migrating keys
    for (int i=0; i < split->num_keys + 1; i++) {
        if (i < split->num_keys) {
            split->keys[i] = node->keys[i + (median + 1)];
        }

        split->links[i] = node->links[i + (median + 1)];
        if (!split->links[i] && !split->leaf) {
            split->links[i] = new_node(tree->branching, 1);
        }

        if (split->links[i]) {
            split->links[i]->link_index = i;
            split->links[i]->parent = split;
        }
    }

    return split;
}

void divide(struct btree* tree, struct bnode* target) {
    if (target->num_keys == tree->branching) {
        if (target == tree->root) {
            tree->root = new_node(tree->branching, 0);
            target->parent = tree->root;
        }

        // promote median key to parent
        struct bnode* parent = target->parent;
        int median = (target->num_keys - (target->num_keys+1) % 2) / 2;
        int inserted_index = insert_key(parent, &target->keys[median]);

        // split and update parent links
        struct bnode* split = split_node(tree, target);
        parent->leaf = 0;

        // move parent links
        memmove(
            parent->links + inserted_index + 1,
            parent->links + inserted_index,
            (parent->num_keys - inserted_index) * (sizeof(struct bnode*))
        );

        parent->links[inserted_index] = target;
        parent->links[inserted_index + 1] = split;

        target->link_index = inserted_index;
        split->link_index = inserted_index + 1;

        if (target != tree->root) {
            divide(tree, target->parent);
        }
    }
}

// TREE DELETION

/**
 * Takes key, and a link if node is not a leaf, from sibling if there is excess
 * of keys and if successful returns 1. Note that if direction is LEFT then the
 * max key and, possibly, link of the left sibling is taken or if it is RIGHT
 * then it takes the min key and, possibly, link of the right sibling.
 */
int robin_hood(struct btree* tree, struct bnode* node, enum Direction side) {
    int sibling_index = node->link_index + (side == LEFT ? -1 : 1);
    if (sibling_index < 0 || sibling_index > node->parent->num_keys) {
        return 0;
    }

    struct bnode* sibling = node->parent->links[sibling_index];
    struct key_value key_buffer = { -1 };

    // if sibling exists, there must exists a key separating them,
    // then check if there is an excess of keys to redistribute
    if (sibling->num_keys > 1) {
        // move parent key to current node
        take_key(node->parent, edge_index(node->parent, side, 1), &key_buffer);
        insert_key(node, &key_buffer);

        // if node is not a leaf move link
        if (!node->leaf) {
            struct bnode* buffer = NULL;
            int take_index = edge_index(sibling, !side, 0);
            take_link(tree, sibling, take_index, &buffer);

            // insert link
            for (int i = edge_index(node, side, 0) + 1; i < node->num_keys + 1; i++) {
                swap_links(&node->links[i], &buffer);
            }
        }

        // move sibling key to parent
        take_key(sibling, edge_index(sibling, !side, 1), &key_buffer);
        insert_key(node->parent, &key_buffer);

        return 1;
    } else {
        return 0;
    }
}

void combine(struct btree* tree, struct bnode* node) {
    enum Direction side = (node->link_index > 0 ? LEFT : RIGHT);
    int sibling_index = node->link_index + (side == LEFT ? -1 : 1);
    int parent_index = node->link_index - (side == LEFT ? 1 : 0);
    struct bnode* sibling = node->parent->links[sibling_index];

    // offset keys and links for insertion
    if (side == LEFT) {
        if (!node->leaf) {
            memmove(
                node->links + sibling->num_keys + 1,
                node->links,
                (node->num_keys + 1) * sizeof(struct bnode*)
            );
        }

        memmove(
            node->keys + sibling->num_keys,
            node->keys,
            node->num_keys * sizeof(struct key_value)
        );
    }

    // drain keys and links from sibling
    int links_offset = (side == LEFT ? 0 : node->num_keys + 1);
    int keys_offset = (side == LEFT ? node->num_keys : 0);
    struct bnode* buffer = NULL;
    for (int i = 0; i < sibling->num_keys + 1; i++) {
        if (i < sibling->num_keys) {
            node->keys[i + keys_offset] = sibling->keys[i];
            memset(&sibling->keys[i], 0, sizeof(struct key_value));
        }

        if (sibling->links[i]) {
            node->links[i + links_offset] = sibling->links[i];
            sibling->links[i] = NULL;

            if (node->links[i + links_offset]) {
                node->links[i + links_offset]->link_index = i + links_offset;
                node->links[i + links_offset]->parent = node;
            }
        }
    }

    node->num_keys += sibling->num_keys;
    sibling->num_keys = 0;

    // take key from parent
    insert_key(node, &node->parent->keys[parent_index]);
    take_key(node->parent, parent_index, NULL);

    // kill sibling and recurse
    free_node(tree, sibling);

    if (node->parent != tree->root && node->parent->num_keys < 1) {
        merge(tree, node->parent);
    }
}

void merge(struct btree* tree, struct bnode* target) {
    // fprintf(stderr, "\nMerging tree\n");
    // debug(tree->root, "", 1);
    // fprintf(stderr, "\n\n");
    if (tree->root == target && tree->root->num_keys == 0) {
        return;
    }

    int min_keys = (target->leaf) ? 1 : 2;
    if (target->num_keys >= min_keys) {
        return;
    }

    // fix properties

    for (int direction = LEFT; direction <= RIGHT; direction++) {
        if (robin_hood(tree, target, direction)) {
            if (target->num_keys < 1) {
                merge(tree, target);
            }

            return;
        }
    }

    combine(tree, target);

    if (tree->root->num_keys < 1) {
        struct bnode* old_root = tree->root;
        tree->root = tree->root->links[0];
        tree->root->parent = NULL;

        memset(old_root->links, 0, sizeof(void*) * (tree->branching + 1));
        free_node(tree, old_root);
    }
}

struct bnode* new_node(uint32_t branching, int leaf) {
    struct bnode* node = malloc(sizeof(struct bnode));
    node->parent = NULL;
    node->num_keys = 0;
    node->leaf = leaf;

    node->links = malloc(sizeof(void*) * (branching+1));
    node->keys = malloc(sizeof(struct key_value) * branching);
    memset(node->links, 0, sizeof(void*) * (branching+1));
    memset(node->keys, 0, sizeof(struct key_value) * branching);
    return node;
}

void disconnect_node(struct btree* tree, struct bnode* node) {
    // fprintf(stderr, "disonnectin %p\n", node);
    if (node) {
        if (node->parent) {
            // disconnect from parent
            node->parent->links[node->link_index] = NULL;
            node->parent = NULL;
        }

        // recurse down tree disconnecting all nodes
        for (int i = 0; i < tree->branching; i++) {
            disconnect_node(tree, node->links[i]);
        }

        // free stored data
        for (int i = 0; i < tree->branching; i++) {
            if (node->keys[i].info.data) {
                free(node->keys[i].info.data);
            }
        }

        // fprintf(stderr, "freeing links & keys\n");
        free(node->links);
        free(node->keys);
        free(node);
    }
}

void recursive_free_node(struct bnode* target) {
    if (target != NULL) {
        // recursively free subtree
        for (int i = 0; i < target->num_keys + 1; i++) {
            if (target->links[i]) {
                recursive_free_node(target->links[i]);
                target->links[i] = NULL;
            }

            // free associated data with key
            if (i < target->num_keys && target->keys[i].info.data) {
                free(target->keys[i].info.data);
                target->keys[i].info.data = NULL;
            }
        }

        free(target->links);
        free(target->keys);
        free(target);
    }
}

void fix_node(struct btree* tree, struct bnode* node) {
    for (int i=0; i + 1 < tree->branching + 1; i++) {
        if (!node->links[i]) {
            for (int j=i; j < tree->branching + 1; j++) {
                if (j + 1 < tree->branching + 1) {
                    node->links[j] = node->links[j + 1];
                } else {
                    node->links[j] = NULL;
                }

                if (node->links[j]) {
                    node->links[j]->link_index = j;
                    node->links[j]->parent = node;
                }
            }
        }
    }
}

void free_node(struct btree* tree, struct bnode* node) {
    struct bnode* parent = node->parent;
    if (parent) {
        // speparate subtree from parent
        parent->links[node->link_index] = NULL;

        // readjust links
        fix_node(tree, parent);
    }

    // recursively free subtree
    recursive_free_node(node);
}

uint64_t listing_nodes(struct bnode* node, struct node* insert) {
    // recursively call providing bnode as argument
    if (node) {
        if (insert) {
            insert->num_keys = node->num_keys;
            insert->keys = malloc(node->num_keys * sizeof(uint32_t));
            for (int i = 0; i < node->num_keys; i++) {
                insert->keys[i] = node->keys[i].key;
            }
        }

        uint64_t index = 1;
        for (int i = 0; i < node->num_keys + 1; i++) {
            index += listing_nodes(node->links[i], insert + index);
        }

        return index;
    } else {
        return 0;
    }
}

void display(struct bnode* node, char* prefix, int last) {
    printf("%s%s(", prefix, (last ? " └─ " : " ├─ "));
    if (node == NULL) {
        printf("NULL)\n");
        return;
    }

    for (int i=0; i < node->num_keys; i++) {
        char* end = (i < node->num_keys-1) ? ", " : "";
        printf("%d%s", node->keys[i].key, end);
    }

    printf(")\n");

    int count = 0;
    for (int i=0; i < node->num_keys + 1; i++) {
        if (node->links[i])
            count += 1;
    }

    if (count > 0) {
        for (int i=0; i < node->num_keys + 1; i++) {
            char* new_prefix = malloc(100 * sizeof(char));
            strcpy(new_prefix, prefix);
            strcat(new_prefix, (last ? "    " : " |  "));
            display(node->links[i], new_prefix, i == node->num_keys);
            free(new_prefix);
        }
    }
}

void debug(struct bnode* node, char* prefix, int last) {
    fprintf(stderr, "%s%s(", prefix, (last ? " └─ " : " ├─ "));
    if (node == NULL) {
        fprintf(stderr, "NULL)\n");
        return;
    }

    for (int i=0; i < node->num_keys; i++) {
        char* end = (i < node->num_keys-1) ? ", " : "";
        fprintf(stderr, "%d%s", node->keys[i].key, end);
    }

    fprintf(stderr, ") %p\n", node);

    int count = 0;
    for (int i=0; i < node->num_keys + 1; i++) {
        if (node->links[i])
            count += 1;
    }

    if (count > 0) {
        for (int i=0; i < node->num_keys + 1; i++) {
            char* new_prefix = malloc(100 * sizeof(char));
            strcpy(new_prefix, prefix);
            strcat(new_prefix, (last ? "    " : " |  "));
            debug(node->links[i], new_prefix, i == node->num_keys);
            free(new_prefix);
        }
    }
}
