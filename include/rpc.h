/*
 * @Description: rpc模块，封装了rdma通信过程中的四个处理函数，这些函数会作为rc_init的参数，函数被调用的位置在common.c
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 20:37:01
 * @LastEditors: abin
 * @LastEditTime: 2020-08-15 11:29:35
 */

#ifndef RPC_H
#define RPC_H

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "ctl.h"

#include "rdma.h"
#include "migrate.h"

void context_clear(struct rdma_context *ctx);
// four main func
void on_pre_conn(struct rdma_cm_id *id, struct request_message *req_msg);
void on_connection(struct rdma_cm_id *id);
void on_completion(struct ibv_wc *wc);
void on_disconnect(struct rdma_cm_id *id, int flag);

void rpc_init();

#endif