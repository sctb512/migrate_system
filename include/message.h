/*
 * @Description: rdma通信过程中会用到的一些结构体，主要用来传递信息
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 11:32:58
 * @LastEditors: abin
 * @LastEditTime: 2020-08-07 08:32:18
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define DEFAULT_PORT "12345"
#define BUFFER_SIZE 10 * 1024 * 1024 //may used

#define MAX_IP_ADDR 20

#define SML_STR_LEN 32
#define MID_STR_LEN 48
#define MAX_STR_LEN 64

#define SSML_STR_LEN 16
#define MMAX_STR_LEN 128

enum message_id
{

        INVALID = -1,
        MSG_MR,
        MEM_INFO_RET,
        PULL_REQUEST_RET,
        PULL_REQUEST_RET_NO_DATA,
        PULL_REQUEST_RET_PULLED_BY_OTHER,
        PULL_REQUEST_RET_NOT_NEED,
        PULL_REQUEST_BUT_NOT_PULL,
        PULLED_INFO,
        PULL_STATUS,
        EVICT_REQUEST_RET,
        EVICTED_REQUEST_RET,
        CLIENT_DONE,
        MSG_SERVER_DONE,
        EVICTED_REQUEST_FOR_TEST_RET,
        PAGEFAULT_EVICTED_REQUEST_RET,
        PAGEFAULT_EVICTED_REQUEST_RET_NO_ADDR,
        PAGEFAULT_EVICTED_REQUEST_FOR_TEST_RET,
        PAGEFAULT_EVICTED_REQUEST_DONE,
        IBVREG_ERR
};

struct message
{

        enum message_id id;
        char ip[MAX_IP_ADDR];

        union {

                struct
                {
                        uint64_t addr;
                        uint64_t origin_addr;
                        uint64_t new_addr;
                        uint32_t rkey;

                        char file_name[MID_STR_LEN];
                        long file_size;
                } mr;

                struct
                {
                        float memory_rate;
                        int need_pulled;
                } mem_info;

        } data;
};

enum request_message_type
{

        PULL_REQUEST,
        MEM_INFO,
        REJECT_REQUEST,
        READ_DATA,
        EDIT_DATA,
        EVICT_REQUEST,
        EVICTED_REQUEST,
        EVICTED_REQUEST_FOR_TEST,
        PAGEFAULT_EVICTED_REQUEST,
        PAGEFAULT_EVICTED_REQUEST_FOR_TEST
};

struct request_message
{

        enum request_message_type msg_type;
        char ip[MAX_IP_ADDR];
        uint64_t addr;

        //when evicted remote data will be used
        uint64_t new_addr;
        uint64_t new_rkey;
};

#endif
