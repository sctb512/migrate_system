
#ifndef PERFFUNC_H
#define PERFFUNC_H

/*
 * @Description: 测试模块，程序测试和性能测试用到的一些函数
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 20:50:21
 * @LastEditors: xy
 * @LastEditTime: 2020-08-30 10:04:41
 */

#include "perf_comm.h"


void access_in_remote(int num);

void test_func_evcited_server();
void test_func_evcited_client();

void test_func_evcit_server();
void test_func_evcit_client();

void test_func_pagefault_server();
void test_func_pagefault_client();

void test_func_tossd_server();
void test_func_tossd_client();

#endif