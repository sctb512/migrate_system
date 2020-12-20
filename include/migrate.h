/*
 * @Description: 迁移模块，迁移程序的主要函数，包括拉取数据、主动迁移、被动迁移、数据落盘和缺页中断回迁
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 20:44:57
 * @LastEditors: abin
 * @LastEditTime: 2020-08-06 22:20:28
 */

#ifndef MIGRATE_H
#define MIGRATE_H

#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "ctl.h"

#include "migcomm.h"
#include "userfaultfd.h"
#include "rpc.h"

void pull_data();
void evict_data();
void evicted_data();
void request_evicted_data();
void pagefault_evicted_data(uint64_t local_addr);
void request_pagefault_evicted_data();
int to_ssd();

void lru_data_init();
void migrate_init();

void run();

#endif