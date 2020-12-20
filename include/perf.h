/*
 * @Description: 测试模块，程序测试和性能测试用到的一些函数
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 20:50:21
 * @LastEditors: abin
 * @LastEditTime: 2020-08-30 10:04:41
 */

#ifndef PERF_H
#define PERF_H

#include "perf_comm.h"

void access_in_remote(int num);

void performance_test_memory(void *num);
void performance_test_func_time_server();
void performance_test_func_time_client();

void server_mode();
void client_mode();
void test(int argc, char **argv);


#endif