/*
 * @Description: RDMA通信过程中会用到的一些基本函数，包括初始化与通信过程逻辑，调用的函数在rdma编程手册中有详细介绍
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 11:32:58
 * @LastEditors: abin
 * @LastEditTime: 2020-08-11 15:11:36
 */

#ifndef RDMA_COMMON_H
#define RDMA_COMMON_H

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>

#include "message.h"
#include "log.h"

#define TEST_NZ(x) do { if ( (x)) rc_die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) rc_die("error: " #x " failed (returned zero/null)."); } while (0)


enum func_type {
  PULL,
  EVICT,
  EVICTED,
  PAGEFAULT_EVICTED,
  TO_SSD//该值暂时没用到
};

//在rdma网络中，ctx变量对应的结构体
struct rdma_context {
  char *buffer;
  struct ibv_mr *buffer_mr;

  struct message *msg;
  struct ibv_mr *msg_mr;

  uint64_t peer_addr;
  uint32_t peer_rkey;

  FILE *fd;
  char *file_name;
  long file_size;

  struct request_message *request_msg;

  enum func_type func; //用于打日志：执行的函数，如PULL、EVICT...
  int need_log; //用来表示是否打印日志
};

typedef void (*pre_conn_cb_fn)(struct rdma_cm_id *id, struct request_message *req_msg);
typedef void (*connect_cb_fn)(struct rdma_cm_id *id);
typedef void (*completion_cb_fn)(struct ibv_wc *wc);
typedef void (*disconnect_cb_fn)(struct rdma_cm_id *id, int flag);

void rc_init(pre_conn_cb_fn, connect_cb_fn, completion_cb_fn, disconnect_cb_fn);
void rc_client_loop(const char *host, const char *port, void *context);
void rc_disconnect(struct rdma_cm_id *id);
void rc_die(const char *message);
struct ibv_pd * rc_get_pd();
void rc_server_loop(const char *port);

#endif
