#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "btreestore.h"

struct bnode {
    uint32_t num_keys;
    int link_index;
    int leaf;

    struct key_value* keys;
    struct bnode** links;
    struct bnode* parent;
};

struct btree {
    uint32_t branching;
    uint8_t processors;
    struct bnode* root;
    uint64_t num_nodes;
};

struct key_value {
    uint32_t key;
    struct info info;
};

struct search_result {
    struct bnode* node;
    int index;
};

enum Direction {
    LEFT = 0,
    RIGHT = 1
};

// CORE FUNCTIONALITY

void divide(struct btree* tree, struct bnode* target);

void merge(struct btree* tree, struct bnode* target);

int find_key(struct btree* tree, struct bnode* node, uint32_t key, struct search_result* result);

// MANAGING KEYS AND LINKS

int insert_key(struct bnode* node, struct key_value* item);

int key_index(struct bnode* node, uint32_t key, int l, int r);

void take_key(struct bnode* node, int index, struct key_value* buffer);

void take_link(struct btree* tree, struct bnode* node, int index, struct bnode** buffer);

int edge_index(struct bnode* node, enum Direction side, int is_key);

// UTILITY

struct btree* new_tree(uint32_t branching_factor);

uint64_t listing_nodes(struct bnode* node, struct node* insert);

struct bnode* new_node(uint32_t branching, int leaf);

void display(struct bnode* node, char* prefix, int last);

void debug(struct bnode* node, char* prefix, int last);

void free_node(struct btree* tree, struct bnode* node);

#endif
