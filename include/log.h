/*
 * @Description: 日志模块，分为一般日志和可以控制日志文件名的数据日志
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 11:37:18
 * @LastEditors: abin
 * @LastEditTime: 2020-08-11 16:09:10
 */

#ifndef LOG_H
#define LOG_H

#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "ctl.h"

/* debug level define */
// int LOG_LEVEL;
// int DEBUG;
// int LOG_WRITE;

FILE *g_log_fp;
FILE *g_data_fp;

char *log_file_name;
char *data_file_name;

#define LOG_LEN 1024*8 //单个日志BUFFER长度，超过就写文件
#define LOG_NAME_LEN 32

char log_str[LOG_LEN];
int log_size;
char data_str[LOG_LEN];
int data_size;

/* log level */
#define INFO 0x02     // call information
#define WARNING 0x04  // paramters invalid,
#define ERR 0x06      // process error, leading to one call fails
#define CRITICAL 0x08 // process error, leading to voip process can't run exactly or exit
#define TEST 0x10     // information for test
#define DATA 0x12     // write log to file

#define L(x) x, __FUNCTION__, __FILE__, __LINE__

void DBG(int level,const char *func, const char *file, const int line, const char *fmt, ...);
void DBG_WRITE();
void DBG_WRITE_LOG();
void DBG_WRITE_DATA();
void log_init(char *log_file);
void printf_log(int level, const char *func, const char *file, const int line, const char *fmt, ...);

#endif