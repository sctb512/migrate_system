/*
 * @Description: 工具函数，写文件，读取时间，读取配置...
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-22 09:15:08
 * @LastEditors: abin
 * @LastEditTime: 2020-08-08 22:12:45
 */

#ifndef TOOLS_H
#define TOOLS_H

#include "log.h"

void write_file(void *addr, int size, FILE *fd); //just used in test
char *read_attr(char *line);
char *get_now_time();

long get_now_time_s();
long get_now_time_ms();
long get_now_time_us();

void sleep_ms(int ms);

#endif