#ifndef TEST_H
#define TEST_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COL_RED     "\x1b[31m"
#define COL_GREEN   "\x1b[32m"
#define COL_YELLOW  "\x1b[33m"
#define COL_BLUE    "\x1b[34m"
#define COL_MAGENTA "\x1b[35m"
#define COL_CYAN    "\x1b[36m"
#define COL_RESET   "\x1b[0m"
#define COL_ORANGE  "\e[38;5;208m"

#define SUCCESS (COL_GREEN "SUCCESS" COL_RESET)
#define BROKEN (COL_YELLOW "BROKEN" COL_RESET)
#define FAILURE (COL_RED "FAILURE" COL_RESET)

int assert_temp_file(char* expected);

#endif
