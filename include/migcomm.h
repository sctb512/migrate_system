/*
 * @Description: 迁移程序用到的一些通用函数以及迁移过程需要用到的一些全局变量
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-22 15:21:10
 * @LastEditors: xy
 * @LastEditTime: 2020-08-27 10:55:07
 */

#ifndef MIGCOMM_H
#define MIGCOMM_H

#include <stddef.h>
#include <arpa/inet.h>

#include "rbtree.h"
#include "common.h"
#include "tools.h"


struct meminfo
{
        char mem_str[16];
        char mem_size[16];
        int mem_total;
        int mem_free;
        int mem_cached;
        int mem_buffer;
        int mem_available;
};

struct memory_info
{
        float mem_rate;
        char *ip;
        unsigned long long total;
        unsigned long used;
        int need_pulled;
};

struct memory_from_remote
{
        char ip[MAX_IP_ADDR];
        float mem_rate;
        int pulled_num;
        int need_pulled;

        struct memory_from_remote *next;
};

struct ip
{
        char ip[MAX_IP_ADDR];
        struct ip *next;
};


enum data_status
{
        HEAD_FLAG = -1,
        PULLED_WAIT,
        PULLING,
        PULLED_DONE,
        FSYNCING,
        FSYNC_DONE,
        EVICT_WAIT,
        EVICTING,
        EVICT_DONE
};

struct buffer_in_remote
{
        char ip[MAX_IP_ADDR];
        uint64_t local_addr;
        uint64_t remote_addr;
        uintptr_t new_addr; // used in userfaultfd evicted program
        uint64_t rkey;
        int size;
        int operate_num;
        int evicted;

        enum data_status status;

        char file_name[MID_STR_LEN];

        struct buffer_in_remote *pre;
        struct buffer_in_remote *next;
};

struct buffer_from_remote
{
        char ip[MAX_IP_ADDR];
        uint64_t local_addr;
        uint64_t origin_addr;
        int size;
        //int operate_num;
        int hot_degree;

        enum data_status status;

        char file_name[MID_STR_LEN];

        struct ibv_mr *buffer_mr;

        struct buffer_from_remote *pre;
        struct buffer_from_remote *next;
};

struct lru_data
{
        char file_name[MID_STR_LEN];
        unsigned char type;
        long size;
        char *buffer;
        int hot_degree;

        enum data_status status;

        struct lru_data *pre;
        struct lru_data *next;
};

char *IPS_FILE;
char local_ip[MAX_IP_ADDR];

float T1;
float T2;

int cycle; // time of every turn

char *path;

struct memory_info *global_mem_info;

// some global variable

struct ip *ips;

struct lru_data *lru_data_head;
struct lru_data *lru_data_tail;
struct lru_data *next_lru_data;

struct buffer_in_remote *in_remote_head; // cong 
struct buffer_in_remote *in_remote_tail;
struct buffer_from_remote *from_remote_head;
struct buffer_from_remote *from_remote_tail;
struct memory_from_remote *remote_memory_head;
struct memory_from_remote *remote_memory_tail;

int in_remote_num;
int from_remote_num;
int to_ssd_num;
long from_remote_total_size;
// long long base_sys_page;
#ifdef EVICT_DEBUG
int debug_num;
#endif

void migcomm_init();
void get_remote_memory();
void get_next_pulled_data();
void get_ip(struct rdma_cm_id *id);
int read_cfgs();
void get_memory_rate();

//function for test
void print_curr_info();
void printf_from_remote_head_node(struct buffer_from_remote *tmp_from_remote);
void printf_lru_data(struct lru_data *tmp_lru_data);
void print_remote_memory();

#endif