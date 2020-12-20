
#ifndef PERFCOMM_H
#define PERFCOMM_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rpc.h"

long lru_size_total;
// char buffer_content;
int test_flag;

int PERF_FUNC_TIME;

char *data_file;

int max_lru_size;
int min_lru_size;

int server_lru_size;
int client_lru_size;

void perf_data_init();
// void perf_init();
void global_memory_init();

int new_one_lru_data();
int new_one_lru_data_solid_size(int size);

#endif