/*
 * @Description: rdma模块，rdma远程读写的函数封装，主要包括rdma协议中的send、receive、write、read
 * @Version: 2.0
 * @Autor: abin
 * @Date: 2020-07-21 11:32:58
 * @LastEditors: abin
 * @LastEditTime: 2020-08-06 22:21:59
 */

#ifndef RDMA_H
#define RDMA_H

#include "common.h"

void read_remote(struct rdma_cm_id *id, uint32_t len);
void write_remote(struct rdma_cm_id *id, uint32_t len);
void post_receive(struct rdma_cm_id *id);
void send_message(struct rdma_cm_id *id);

#endif
