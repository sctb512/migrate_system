/*
 * @Description: 缺页中断处理模块，包括缺页中断初始化、缺页中断注册和缺页中断处理
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-22 09:15:00
 * @LastEditors: abin
 * @LastEditTime: 2020-08-06 22:26:51
 */

#ifndef USERFAULTFD_H
#define USERFAFULTFD_H

#include <sys/types.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <fcntl.h>

#include "ctl.h"

#include "migrate.h"
#include "list.h"


#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

//for userfaultfd
long uffd; /* userfaultfd file descriptor */
struct uffdio_register uffdio_register;

void *fault_handler_thread();
int init_userfaultfd();
int register_userfaultfd(unsigned long addr, unsigned long len);
int unregister_userfaultfd(unsigned long addr, unsigned long len);

#endif