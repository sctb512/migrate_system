#include "rpc.h"


/*
breakdown 标号说明
101 - pull
201 - evict
301 - pagefault
401 - evcited
501 - 系统代码接口
601 - 自定义功能函数测试

应该用火焰图直接测试系统性能瓶颈
*/


void context_clear(struct rdma_context *ctx)
{
        ctx->buffer = NULL;
        ctx->buffer_mr = NULL;

        ctx->fd = NULL;
        ctx->file_name = NULL;
}


/**
 * @description: 建立rdma连接前执行的函数，进行连接前的一些准备，rdma通信发起端和接收端都会执行此函数
 * @param {struct rdma_cm_id *, struct request_message *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void on_pre_conn(struct rdma_cm_id *id, struct request_message *req_msg)
{
        struct rdma_context *ctx = (struct rdma_context *)malloc(sizeof(struct rdma_context));
        
        context_clear(ctx);

        ctx->need_log = 0;

        id->context = ctx;

        if (req_msg != NULL)
        {
                //DBG(L(INFO), "msg_type pre connection:%d",req_msg->msg_type);
                ctx->request_msg = req_msg;
        }

        posix_memalign((void **)&ctx->msg, sysconf(_SC_PAGESIZE), sizeof(*ctx->msg));
        // TEST_Z(ctx->msg_mr = ibv_reg_mr(rc_get_pd(), ctx->msg, sizeof(*ctx->msg), IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE));
        ctx->msg_mr = ibv_reg_mr(rc_get_pd(), ctx->msg, sizeof(*ctx->msg), IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
        if (ctx->msg_mr == NULL) {
                DBG(L(TEST), "[%s %d] ibv_reg_mr error: %s", __FILE__, __LINE__, strerror(errno));
                rc_disconnect(id);
                return;
        }

        post_receive(id);
}

/**
 * @description: 建立连接完成后会执行的函数，根据req_msg->msg_type的值执行不同逻辑，server端和client端需要执行的代码段都在此函数中
 * @param {struct rdma_cm_id *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void on_connection(struct rdma_cm_id *id)
{
        struct rdma_context *ctx = (struct rdma_context *)id->context;
        struct request_message *req_msg = ctx->request_msg;

        //for server mode, on_connection()
        if (req_msg->msg_type == MEM_INFO)
        {
                ctx->msg->id = MEM_INFO_RET;
                ctx->msg->data.mem_info.memory_rate = global_mem_info->mem_rate;
                ctx->msg->data.mem_info.need_pulled = global_mem_info->need_pulled;

                get_ip(id);
                strcpy(ctx->msg->ip, global_mem_info->ip);

                send_message(id);

                context_clear(ctx);
        }

        //for server mode, on_connection()
        if (req_msg->msg_type == PULL_REQUEST)
        {
                DBG(L(INFO), "rpc, on_connection(),in_remote_num:%d", in_remote_num);

                #ifdef TEST_BREAKDOWN_PULL
                unsigned  long  long pull_step2_start = get_now_time_us();
                #endif

                get_next_pulled_data();

                #ifdef TEST_BREAKDOWN_PULL
                unsigned  long  long pull_step2_end = get_now_time_us();
                DBG(L(DATA), "101,pull_step2,slect_data,%lld,%lld,%lld,%s", pull_step2_start, pull_step2_end, pull_step2_end-pull_step2_start, get_now_time());
                #endif

                struct lru_data *pulled_lru_data = next_lru_data;
                // printf_lru_data(pulled_lru_data);
                if (pulled_lru_data != NULL && pulled_lru_data->status == PULLED_WAIT)
                {
                        // DBG(L(INFO), "@ used: %ld, lru size: %.3lf, total: %lld", global_mem_info->used, pulled_lru_data->size / 1024.0, global_mem_info->total);
                        float tmp_memory_rate = ((global_mem_info->used - pulled_lru_data->size / 1024.0) * 100.0) / global_mem_info->total;//? 括号里面变为减法
                        // DBG(L(INFO), "1tmp mem rate: %f%%, T1: %f%%", tmp_memory_rate, T1);

                        if (tmp_memory_rate > T1)
                        {
                                pulled_lru_data->status = PULLING;
                                ctx->buffer = pulled_lru_data->buffer;
                                ctx->file_size = pulled_lru_data->size;
                                //new add end

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_ibv_reg_mr_start = get_now_time_us();
                                #endif

                                ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, ctx->file_size, IBV_ACCESS_REMOTE_READ | IBV_ACCESS_LOCAL_WRITE);

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_ibv_reg_mr_end = get_now_time_us();
                                DBG(L(DATA), "101,pull_step3_0,server_ibv_reg_mr,%lld,%lld,%lld,%s", pull_ibv_reg_mr_start, pull_ibv_reg_mr_end, pull_ibv_reg_mr_end-pull_ibv_reg_mr_start, get_now_time());
                                #endif

                                if (ctx->buffer_mr != NULL)
                                {
                                        ctx->msg->data.mr.file_size = ctx->file_size;

                                        ctx->msg->id = PULL_REQUEST_RET;
                                        ctx->msg->data.mr.addr = (uint64_t)ctx->buffer_mr->addr;
                                        ctx->msg->data.mr.rkey = ctx->buffer_mr->rkey;
                                        
                                        #ifdef TEST_BREAKDOWN_PULL
                                        // unsigned long long pull_step3_start = get_now_time_us();
                                        #endif

                                        get_ip(id);
                                        #ifdef TEST_BREAKDOWN_PULL
                                        // unsigned long long pull_step3_end = get_now_time_us();
                                        // DBG(L(DATA), "101,pull_step3,get_local_ip,%lld,%lld,%lld,%s", pull_step3_start, pull_step3_end, pull_step3_end-pull_step3_start, get_now_time());
                                        #endif

                                        strcpy(ctx->msg->ip, global_mem_info->ip);
                                        post_receive(id);
                                        send_message(id);
                                }
                                else
                                {
                                        rc_disconnect(id);
                                }
                        }
                        else
                        {
                                ctx->msg->id = PULL_REQUEST_RET_NOT_NEED;

                                get_ip(id);
                                strcpy(ctx->msg->ip, global_mem_info->ip);

                                send_message(id);

                                context_clear(ctx);
                        }
                }
                else
                {
                        if (pulled_lru_data != NULL)
                        {
                                ctx->msg->id = PULL_REQUEST_RET_PULLED_BY_OTHER;
                        }
                        else
                        {
                                ctx->msg->id = PULL_REQUEST_RET_NO_DATA;
                        }
                        get_ip(id);
                        strcpy(ctx->msg->ip, global_mem_info->ip);

                        send_message(id);

                        context_clear(ctx);
                }
        }

        //for client mode, on_connection()
        if (req_msg->msg_type == PAGEFAULT_EVICTED_REQUEST)
        {

                uint64_t local_addr = req_msg->addr;

                struct buffer_from_remote *curr_from_remote = NULL;

                #ifdef TEST_BREAKDOWN_PAGEFAULT
                unsigned long long pagefault_step3_start = get_now_time_us();
                #endif

                rbtree_node *rb_from_node = rbtree_search(from_remote_tree, (unsigned long)local_addr);

                #ifdef TEST_BREAKDOWN_PAGEFAULT
                unsigned long long pagefault_step3_end = get_now_time_us();
                DBG(L(DATA), "301,pagefault_step3,rbtree_search_from_remote_node,%lld,%lld,%lld,%s", pagefault_step3_start, pagefault_step3_end, pagefault_step3_end-pagefault_step3_start, get_now_time());
                #endif

                if (rb_from_node != NULL)
                {
                        curr_from_remote = (struct buffer_from_remote *)rb_from_node->struct_addr;
                }

                if (curr_from_remote != NULL && curr_from_remote->status == EVICT_WAIT)
                {
                        curr_from_remote->status = EVICTING;

                        ctx->msg->id = PAGEFAULT_EVICTED_REQUEST_RET;

                        ctx->msg->data.mr.origin_addr = curr_from_remote->origin_addr;
                        get_ip(id);
                        strcpy(ctx->msg->ip, global_mem_info->ip);

                        post_receive(id);
                        send_message(id);
                }
                else
                {
                        // DBG(L(TEST), "local addr: %p status is not EVICT_WAIT or NOT FOUND", req_msg->addr);

                        context_clear(ctx);

                        rc_disconnect(id);
                }
        }

        if (req_msg->msg_type == PAGEFAULT_EVICTED_REQUEST_FOR_TEST)
        {
                char *addr = (char *)req_msg->addr;
                DBG(L(INFO), "pagefaule in addr: %p", (void *)addr);

                DBG(L(INFO), "pagefault addr content:");
                int i;
                for (i = 0; i < 104; i++)
                {
                        DBG(L(INFO), "%c", addr[i]);
                }
                DBG(L(INFO), "");

                ctx->msg->id = PAGEFAULT_EVICTED_REQUEST_FOR_TEST_RET;
                send_message(id);

                context_clear(ctx);
        }

        //for server mode on_connection()
        if (req_msg->msg_type == EVICT_REQUEST)         //client request from client
        {
                #ifdef EVICT_DEBUG
                debug_num++;
                DBG(L(TEST), "prc.c 257.start EVICT_REQUEST. evict_data2. debug_num=%d",debug_num);
                #endif

                // DBG(L(TEST), "get evict request from client!");
                struct buffer_in_remote *curr_in_remote = NULL;
                #ifdef TEST_BREAKDOWN_EVICT
                unsigned long long evict_step2_start = get_now_time_us();
                #endif
                rbtree_node *rb_in_node = rbtree_search(in_remote_tree, (unsigned long)req_msg->addr);
                #ifdef TEST_BREAKDOWN_EVICT
                unsigned long long evict_step2_end = get_now_time_us();
                DBG(L(DATA), "201,evict_step2,rbtree_search_in_remote_node,%lld,%lld,%lld,%s", evict_step2_start, evict_step2_end, evict_step2_end-evict_step2_start, get_now_time());
                #endif
                // rbtree_inorder_by_key(in_remote_tree->root, in_remote_tree->nil, (unsigned long)req_msg->addr);

                if (rb_in_node != NULL)
                {
                        curr_in_remote = (struct buffer_in_remote *)rb_in_node->struct_addr;
                }

                if (curr_in_remote != NULL)
                {
                        curr_in_remote->status = EVICTING;
                        
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step3_1_1_start = get_now_time_us();
                        #endif
                        // posix_memalign((void **)&ctx->buffer, sysconf(_SC_PAGESIZE), curr_in_remote->size);
                        ctx->buffer = (char *)malloc(curr_in_remote->size);
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step3_1_1_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step3_1_1,malloc,%lld,%lld,%lld,%s", evict_step3_1_1_start, evict_step3_1_1_end, evict_step3_1_1_end-evict_step3_1_1_start, get_now_time());
                        
                        unsigned long long evict_step3_1_2_start = get_now_time_us();
                        #endif
                        // TEST_Z(ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, curr_in_remote->size, IBV_ACCESS_LOCAL_WRITE));
                        ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, curr_in_remote->size, IBV_ACCESS_LOCAL_WRITE);
                        if (ctx->buffer_mr == NULL) {
                                DBG(L(TEST), "[%s %d] ibv_reg_mr error: %s", __FILE__, __LINE__, strerror(errno));

                                ctx->msg->id = IBVREG_ERR;
                                send_message(id);

                                context_clear(ctx);
                                return;
                        }
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step3_1_2_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step3_1_2,server_ibv_reg_mr,%lld,%lld,%lld,%s", evict_step3_1_2_start, evict_step3_1_2_end, evict_step3_1_2_end-evict_step3_1_2_start, get_now_time());
                        // unsigned long long evict_step3_1_3_start = get_now_time_us();
                        #endif

                        ctx->file_size = curr_in_remote->size;
                        ctx->peer_addr = curr_in_remote->remote_addr;
                        ctx->peer_rkey = curr_in_remote->rkey;
                        ctx->msg->data.mr.addr = curr_in_remote->remote_addr;

                        #ifdef TEST_BREAKDOWN_EVICT
                        // unsigned long long evict_step3_1_3_end = get_now_time_us();
                        // DBG(L(DATA), "201,evict_step3_1_3,ctx,%lld,%lld,%lld,%s", evict_step3_1_3_start, evict_step3_1_3_end, evict_step3_1_3_end-evict_step3_1_3_start, get_now_time());
                        unsigned long long evict_step3_2_start = get_now_time_us();
                        #endif

                        read_remote(id, ctx->file_size);
                        
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step3_2_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step3_2,evict_read_data,%lld,%lld,%lld,%s", evict_step3_2_start, evict_step3_2_end, evict_step3_2_end-evict_step3_2_start, get_now_time());

                        unsigned long long evict_step4_start = get_now_time_us();
                        #endif

                        rbtree_delete(in_remote_tree, rb_in_node);

                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step4_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step4,rbtree_delete_in_remote_node,%lld,%lld,%lld,%s", evict_step4_start, evict_step4_end, evict_step4_end-evict_step4_start, get_now_time());
                        unsigned long long evict_step5_start = get_now_time_us();
                        #endif
                        del_in_remote_node(curr_in_remote);
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step5_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step5,list_del_in_remote_node,%lld,%lld,%lld,%s", evict_step5_start, evict_step5_end, evict_step5_end-evict_step5_start, get_now_time());

                        unsigned long long evict_step6_start = get_now_time_us();
                        #endif

                        unregister_userfaultfd(curr_in_remote->local_addr, curr_in_remote->size);

                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step6_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step6,unregister_userfaultfd,%lld,%lld,%lld,%s", evict_step6_start, evict_step6_end, evict_step6_end-evict_step6_start, get_now_time());
                        #endif

                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step6_1_start = get_now_time_us();
                        #endif
                        //? 这里拷贝的对吗
                        memcpy((void *)curr_in_remote->local_addr, ctx->buffer, ctx->file_size);
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step6_1_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step6_1,memcpy,%lld,%lld,%lld,%s", evict_step6_1_start, evict_step6_1_end, evict_step6_1_end-evict_step6_1_start, get_now_time());
                        #endif

                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step6_2_start = get_now_time_us();
                        #endif
                        //? 这里是不是有问题？
                        madvise(ctx->buffer, curr_in_remote->size, MADV_DONTNEED);
                        
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step6_2_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step6_2,server_madvise,%lld,%lld,%lld,%s", evict_step6_2_start, evict_step6_2_end, evict_step6_2_end-evict_step6_2_start, get_now_time());
                        #endif

                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step6_3_start = get_now_time_us();
                        #endif
                        free(ctx->buffer);
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step6_3_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step6_3,server_free,%lld,%lld,%lld,%s", evict_step6_3_start, evict_step6_3_end, evict_step6_3_end-evict_step6_3_start, get_now_time());
                        #endif

                        ctx->buffer = NULL;

                        //DEF_LRU LRU_data_list 状态还原
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step7_start = get_now_time_us();
                        #endif

                        update_lru_data((unsigned long)curr_in_remote->local_addr);

                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step7_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step7,update_lru_data,%lld,%lld,%lld,%s", evict_step7_start, evict_step7_end, evict_step7_end-evict_step7_start, get_now_time());
                        #endif
                        curr_in_remote->status = EVICT_DONE;

                        free(curr_in_remote);
                        in_remote_num -= 1;
                        ctx->msg->id = EVICT_REQUEST_RET;
                        get_ip(id);
                        strcpy(ctx->msg->ip, global_mem_info->ip);

                        #ifdef EVICT_DEBUG
                        #endif
                        send_message(id);

                        #ifdef EVICT_DEBUG
                        DBG(L(TEST), "prc.c 369. EVICT_REQUEST. evict_data3. debug_num=%d",debug_num);
                        #endif
                }
                else
                {
                        DBG(L(ERR), "ip: %s machine can't find node in_remote_head by local_addr!", req_msg->ip);
                }
        }

        if (req_msg->msg_type == EVICTED_REQUEST_FOR_TEST)
        {
                DBG(L(INFO), "get evicted request from ip: %s", req_msg->ip);

                evicted_data();
                ctx->msg->id = EVICTED_REQUEST_FOR_TEST_RET;
                send_message(id);

                context_clear(ctx);
        }

        //for client mode, on_connection()
        if (req_msg->msg_type == EVICTED_REQUEST)
        {
                struct buffer_from_remote *curr_from_remote = NULL;

                #ifdef TEST_BREAKDOWN_EVICTED
                unsigned long long evicted_step3_start = get_now_time_us();
                #endif

                rbtree_node *rb_from_node = rbtree_search(from_remote_tree, (unsigned long)req_msg->addr);

                #ifdef TEST_BREAKDOWN_EVICTED
                unsigned long long evicted_step3_end = get_now_time_us();
                DBG(L(DATA), "401,evicted_step3,rbtree_search_from_remote,%lld,%lld,%lld,%s", evicted_step3_start, evicted_step3_end, evicted_step3_end-evicted_step3_start, get_now_time());
                #endif

                if (rb_from_node != NULL)
                {
                        curr_from_remote = (struct buffer_from_remote *)rb_from_node->struct_addr;
                }

                if (curr_from_remote != NULL)
                {
                        curr_from_remote->status = EVICTING;
                        // DBG(L(TEST), "EVICTED_REQUEST, origin addr: %p", curr_from_remote->origin_addr);
                        //rdma_write
                        #ifdef TEST_BREAKDOWN_EVICTED
                        // unsigned long long evicted_step3_1_start = get_now_time_us();
                        #endif
                        ctx->peer_addr = req_msg->new_addr;
                        ctx->peer_rkey = req_msg->new_rkey;
                        ctx->buffer_mr = curr_from_remote->buffer_mr;
                        ctx->buffer = (char *)curr_from_remote->local_addr;
                        #ifdef TEST_BREAKDOWN_EVICTED
                        // unsigned long long evicted_step3_1_end = get_now_time_us();
                        // DBG(L(DATA), "401,evicted_step3_1,ctx,%lld,%lld,%lld,%s", evicted_step3_1_start, evicted_step3_1_end, evicted_step3_1_end-evicted_step3_1_start, get_now_time());
                        #endif
                        
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step4_start = get_now_time_us();
                        #endif
                        write_remote(id, curr_from_remote->size);
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step4_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step3_2,evicted_rdma_write,%lld,%lld,%lld,%s", evicted_step4_start, evicted_step4_end, evicted_step4_end-evicted_step4_start, get_now_time());
                        #endif

                        //ibv_dereg_mr(curr_from_remote->buffer_mr);

                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long madvise_start = get_now_time_us();
                        #endif
                        madvise((void *)curr_from_remote->local_addr, curr_from_remote->size, MADV_DONTNEED);

                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long madvise_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step4,evicted_madvise,%lld,%lld,%lld,%s", madvise_start, madvise_end, madvise_end-madvise_start, get_now_time());
                        #endif

                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long free_start = get_now_time_us();
                        #endif
                        free((void *)curr_from_remote->local_addr);
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long free_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step4_1,evicted_free,%lld,%lld,%lld,%s", free_start, free_end, free_end-free_start, get_now_time());
                        #endif

                        ctx->buffer = NULL;
                        
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step5_start = get_now_time_us();
                        #endif
                        rbtree_delete(from_remote_tree, rb_from_node);
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step5_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step5,rbtree_delete_from_remote,%lld,%lld,%lld,%s", evicted_step5_start, evicted_step5_end, evicted_step5_end-evicted_step5_start, get_now_time());
                        #endif

                        //abin,del from list
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step6_start = get_now_time_us();
                        #endif
                        del_from_remote_node(curr_from_remote);
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step6_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step6,list_del_from_remote_node,%lld,%lld,%lld,%s", evicted_step6_start, evicted_step6_end, evicted_step6_end-evicted_step6_start, get_now_time());
                        #endif

                        ctx->msg->data.mr.addr = curr_from_remote->origin_addr;

                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step7_start = get_now_time_us();
                        #endif
                        struct memory_from_remote *tmp_remote_memory = remote_memory_head;
                        while (tmp_remote_memory->next != NULL)
                        {
                                tmp_remote_memory = tmp_remote_memory->next;
                                DBG(L(INFO), "tmp remote ip: %s", tmp_remote_memory->ip);
                                DBG(L(INFO), "del from remote ip: %s", curr_from_remote->ip);
                                if (!strcmp(tmp_remote_memory->ip, curr_from_remote->ip))
                                {
                                        tmp_remote_memory->pulled_num -= 1;
                                        break;
                                }
                        }
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step7_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step7,update_remote_mem_status,%lld,%lld,%lld,%s", evicted_step7_start, evicted_step7_end, evicted_step7_end-evicted_step7_start, get_now_time());
                        #endif

                        from_remote_total_size -= curr_from_remote->size;

                        curr_from_remote->status = EVICT_DONE;
                        free(curr_from_remote);

                        ctx->msg->id = EVICTED_REQUEST_RET;
                        get_ip(id);
                        strcpy(ctx->msg->ip, global_mem_info->ip);

                        send_message(id);
                        from_remote_num -= 1;
                }
                else
                {
                        DBG(L(ERR), "ERROR:ip: %s machine can't find node from_remote_head by local_addr!", req_msg->ip);
                }
        }
}

/**
 * @description: 建立连接并发起第一次通信后，之后的所有通信顾欧城都会到此函数进行处理，server端和client端的处理逻辑都在此函数内，根据ctx->msg->id进行区分
 * @param {struct ibv_wc *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void on_completion(struct ibv_wc *wc)
{
        struct rdma_cm_id *id = (struct rdma_cm_id *)(uintptr_t)wc->wr_id;
        struct rdma_context *ctx = (struct rdma_context *)id->context;

        if (wc->opcode & IBV_WC_RECV)
        {
                //for server
                if (ctx->msg->id == CLIENT_DONE)
                {
                        next_lru_data->status = PULLED_DONE;
                        struct buffer_in_remote *tmp_in_remote = (struct buffer_in_remote *)malloc(sizeof(struct buffer_in_remote));
                        strcpy(tmp_in_remote->ip, ctx->msg->ip);
                        tmp_in_remote->local_addr = (uint64_t)ctx->msg->data.mr.origin_addr;

                        tmp_in_remote->remote_addr = ctx->msg->data.mr.addr;
                        tmp_in_remote->rkey = ctx->msg->data.mr.rkey;
                        tmp_in_remote->size = ctx->file_size;
                        tmp_in_remote->evicted = 0;
                        tmp_in_remote->status = EVICT_WAIT;
                        tmp_in_remote->operate_num = 0;
                        tmp_in_remote->next = NULL;

                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step8_start = get_now_time_us();
                        #endif

                        rbtree_insert(in_remote_tree, (unsigned long)tmp_in_remote->local_addr, (void *)tmp_in_remote);

                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step8_end = get_now_time_us();
                        DBG(L(DATA), "101,pull_step8,rbtree_add_new_in_remote_node,%lld,%lld,%lld,%s", pull_step8_start, pull_step8_end, pull_step8_end-pull_step8_start, get_now_time());
                        #endif

                        // rbtree_inorder(in_remote_tree->root, in_remote_tree->nil);
                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step9_start = get_now_time_us();
                        #endif

                        add_in_remote_node_to_tail(tmp_in_remote);

                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step9_end = get_now_time_us();
                        DBG(L(DATA), "101,pull_step9,list_add_in_remote_node,%lld,%lld,%lld,%s", pull_step9_start, pull_step9_end, pull_step9_end-pull_step9_start, get_now_time());

                        unsigned long long pull_step10_start = get_now_time_us();
                        #endif

                        buffer_in_remote_sorting(tmp_in_remote);

                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step10_end = get_now_time_us();
                        DBG(L(DATA), "101,pull_step10,buffer_in_remote_sorting,%lld,%lld,%lld,%s", pull_step10_start, pull_step10_end, pull_step10_end-pull_step10_start, get_now_time());

                        unsigned long long pull_madvise_start = get_now_time_us();
                        #endif

                        madvise((void *)tmp_in_remote->local_addr, tmp_in_remote->size, MADV_DONTNEED);

                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_madvise_end = get_now_time_us();
                        DBG(L(DATA), "101,pull_step10_1,pull_madvise,%lld,%lld,%lld,%s", pull_madvise_start, pull_madvise_end, pull_madvise_end-pull_madvise_start, get_now_time());

                        unsigned long long pull_step11_start = get_now_time_us();
                        #endif

                        register_userfaultfd(tmp_in_remote->local_addr, tmp_in_remote->size);

                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step11_end = get_now_time_us();
                        DBG(L(DATA), "101,pull_step11,register_userfaultfd,%lld,%lld,%lld,%s", pull_step11_start, pull_step11_end, pull_step11_end-pull_step11_start, get_now_time());
                        #endif

                        ctx->buffer = NULL;
                        
                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step12_start = get_now_time_us();
                        #endif

                        ibv_dereg_mr(ctx->buffer_mr);

                        #ifdef TEST_BREAKDOWN_PULL
                        unsigned long long pull_step12_end = get_now_time_us();
                        DBG(L(DATA), "101,pull_step12,ibv_dereg_mr,%lld,%lld,%lld,%s", pull_step12_start, pull_step12_end, pull_step12_end-pull_step12_start, get_now_time());
                        #endif

                        context_clear(ctx);

                        in_remote_num++;
                        rc_disconnect(id);
                }

                if (ctx->msg->id == PULL_REQUEST_BUT_NOT_PULL)
                {
                        next_lru_data->status = PULLED_WAIT;

                        context_clear(ctx);

                }

                //for server mode
                if (ctx->msg->id == EVICTED_REQUEST_RET)
                {
                        struct buffer_in_remote *curr_in_remote = NULL;
                        //tree,abin
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step8_start = get_now_time_us();
                        #endif
                        rbtree_node *rb_in_node = rbtree_search(in_remote_tree, (unsigned long)ctx->msg->data.mr.addr);
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step8_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step8,rbtree_search_in_remote_node,%lld,%lld,%lld,%s", evicted_step8_start, evicted_step8_end, evicted_step8_end-evicted_step8_start, get_now_time());
                        #endif

                        if (rb_in_node != NULL)
                        {
                                curr_in_remote = (struct buffer_in_remote *)rb_in_node->struct_addr;
                        }

                        if (curr_in_remote != NULL)
                        {
                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long evicted_step9_start = get_now_time_us();
                                #endif
                                rbtree_delete(in_remote_tree, rb_in_node);

                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long evicted_step9_end = get_now_time_us();
                                DBG(L(DATA), "401,evicted_step9,rbtree_del_in_remote_node,%lld,%lld,%lld,%s", evicted_step9_start, evicted_step9_end, evicted_step9_end-evicted_step9_start, get_now_time());
                                #endif
                                //abin,del from list
                                // DBG(L(TEST), "del_in_remote_node 222!");
                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long evicted_step10_start = get_now_time_us();
                                #endif
                                del_in_remote_node(curr_in_remote);
                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long evicted_step10_end = get_now_time_us();
                                DBG(L(DATA), "401,evicted_step10,list_del_in_remote_node,%lld,%lld,%lld,%s", evicted_step10_start, evicted_step10_end, evicted_step10_end-evicted_step10_start, get_now_time());
                                
                                unsigned long long evicted_step11_start = get_now_time_us();
                                #endif
                                unregister_userfaultfd(curr_in_remote->local_addr, curr_in_remote->size);
                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long evicted_step11_end = get_now_time_us();
                                DBG(L(DATA), "401,evicted_step11,unregister_userfaultfd,%lld,%lld,%lld,%s", evicted_step11_start, evicted_step11_end, evicted_step11_end-evicted_step11_start, get_now_time());
                                
                                unsigned long long memcpy_start = get_now_time_us();
                                #endif

                                memcpy((void *)curr_in_remote->local_addr, (void *)curr_in_remote->new_addr, curr_in_remote->size);
                                
                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long memcpy_end = get_now_time_us();
                                DBG(L(DATA), "401,evicted_step12_0,evicted_memcpy,%lld,%lld,%lld,%s", memcpy_start, memcpy_end, memcpy_end-memcpy_start, get_now_time());
                                #endif
                                madvise((void *)curr_in_remote->new_addr, curr_in_remote->size, MADV_DONTNEED);
                                free((void *)curr_in_remote->new_addr);

                                //DEF_LRU LRU_data_list状态还原
                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long evicted_step12_start = get_now_time_us();
                                #endif
                                update_lru_data(curr_in_remote->local_addr);
                                #ifdef TEST_BREAKDOWN_EVICTED
                                unsigned long long evicted_step12_end = get_now_time_us();
                                DBG(L(DATA), "401,evicted_step12,update_lru_data,%lld,%lld,%lld,%s", evicted_step12_start, evicted_step12_end, evicted_step12_end-evicted_step12_start, get_now_time());
                                #endif

                                //PER_LOG var
                                ctx->file_size = curr_in_remote->size;
                                ctx->func = EVICTED;
                                ctx->need_log = 1;

                                curr_in_remote->status = EVICT_DONE;
                                free(curr_in_remote);
                        }
                        else
                        {
                                DBG(L(ERR), "on_completion() EVICTED_REQUEST_RET, the curr_in_remote is NULL!");
                        }

                        in_remote_num -= 1;

                        DBG(L(INFO), "evicted data successful, disconnecting...");
                        rc_disconnect(id);
                }

                //for client mode, on_completion()
                if (ctx->msg->id == PULL_REQUEST_RET)
                {
                        // DBG(L(INFO), "client ok!");
                        long slab_size = ctx->msg->data.mr.file_size;
                        //?
                        // get_memory_rate();
                        //DBG(L(INFO), "mem rate: %lf%%", global_mem_info->mem_rate);
                        //判断是否拉取后会大于T1，如果大于T1，则不拉取，返回不需要拉取的
                        float tmp_memory_rate = ((global_mem_info->used + slab_size / 1024.0) * 100.0) / global_mem_info->total;
                        //DBG(L(INFO), "2tmp mem rate: %f%%, T1: %f%%", tmp_memory_rate, T1);

                        if (tmp_memory_rate < T1)
                        {
                                char *curr_ip = ctx->msg->ip;
                                char *curr_ip_store = (char *)malloc(sizeof(char) * MAX_IP_ADDR);
                                strcpy(curr_ip_store, curr_ip);

                                ctx->file_size = slab_size;

                                ctx->peer_addr = ctx->msg->data.mr.addr;
                                ctx->peer_rkey = ctx->msg->data.mr.rkey;
                                //DBG(L(INFO), "remote buffer address %p", (void *)ctx->peer_addr);

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step4_1_1_start = get_now_time_us();
                                #endif 

                                posix_memalign((void **)&ctx->buffer, sysconf(_SC_PAGESIZE), ctx->file_size);
                                // ctx->buffer = (char *)malloc(ctx->file_size);
                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step4_1_1_end = get_now_time_us();
                                DBG(L(DATA), "101,pull_step4_1_1,malloc,%lld,%lld,%lld,%s", pull_step4_1_1_start, pull_step4_1_1_end, pull_step4_1_1_end-pull_step4_1_1_start, get_now_time());

                                unsigned long long pull_step4_1_2_start = get_now_time_us();
                                #endif
                                // TEST_Z(ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, ctx->file_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ));
                                ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, ctx->file_size, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ);
                                if (ctx->buffer_mr == NULL) {
                                        DBG(L(TEST), "[%s %d] ibv_reg_mr error: %s", __FILE__, __LINE__, strerror(errno));

                                        ctx->msg->id = IBVREG_ERR;
                                        send_message(id);

                                        context_clear(ctx);
                                        return;
                                }
                                
                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step4_1_2_end = get_now_time_us();
                                DBG(L(DATA), "101,pull_step4_1_2,client_ibv_reg_mr,%lld,%lld,%lld,%s", pull_step4_1_2_start, pull_step4_1_2_end, pull_step4_1_2_end-pull_step4_1_2_start, get_now_time());

                                unsigned long long pull_step4_2_start = get_now_time_us();
                                #endif

                                read_remote(id, ctx->file_size);

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step4_2_end = get_now_time_us();
                                DBG(L(DATA), "101,pull_step4_2,pull_read_data,%lld,%lld,%lld,%s", pull_step4_2_start, pull_step4_2_end, pull_step4_2_end-pull_step4_2_start, get_now_time());
                                #endif

                                struct buffer_from_remote *tmp_from_remote = (struct buffer_from_remote *)malloc(sizeof(struct buffer_from_remote));
                                strcpy(tmp_from_remote->ip, curr_ip_store);
                                tmp_from_remote->local_addr = (uintptr_t)ctx->buffer_mr->addr;
                                tmp_from_remote->origin_addr = ctx->peer_addr;
                                tmp_from_remote->size = ctx->file_size;
                                tmp_from_remote->buffer_mr = ctx->buffer_mr;
                                tmp_from_remote->status = EVICT_WAIT;
                                tmp_from_remote->hot_degree = rand();
                                //strcpy(tmp_from_remote->file_name, ctx->file_name);
                                tmp_from_remote->next = NULL;

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step5_start = get_now_time_us();
                                #endif

                                rbtree_insert(from_remote_tree, (unsigned long)tmp_from_remote->local_addr, (void *)tmp_from_remote);

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step5_end = get_now_time_us();
                                DBG(L(DATA), "101,pull_step5,rbtree_add_new_from_remote_node,%lld,%lld,%lld,%s", pull_step5_start, pull_step5_end, pull_step5_end-pull_step5_start, get_now_time());
                                
                                unsigned long long pull_step6_start = get_now_time_us();
                                #endif
                                add_from_remote_node_to_tail(tmp_from_remote);

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step6_end = get_now_time_us();
                                DBG(L(DATA), "101,pull_step6,list_add_from_remote_node,%lld,%lld,%lld,%s", pull_step6_start, pull_step6_end, pull_step6_end-pull_step6_start, get_now_time());
                                
                                unsigned long long pull_step7_start = get_now_time_us();
                                #endif

                                buffer_from_remote_sorting(tmp_from_remote);

                                #ifdef TEST_BREAKDOWN_PULL
                                unsigned long long pull_step7_end = get_now_time_us();
                                DBG(L(DATA), "101,pull_step7,buffer_from_remote_sorting,%lld,%lld,%lld,%s", pull_step7_start, pull_step7_end, pull_step7_end-pull_step7_start, get_now_time());
                                #endif

                                /*from_remote_tail->next = tmp_from_remote;
                                from_remote_tail = tmp_from_remote;*/
                                from_remote_num++;
                                from_remote_total_size += ctx->file_size;

                                // DBG(L(TEST), "from remote num: %d", from_remote_num);
                                struct memory_from_remote *tmp_remote_memory = remote_memory_head;
                                while (tmp_remote_memory->next != NULL)
                                {
                                        tmp_remote_memory = tmp_remote_memory->next;
                                        //DBG(L(INFO), "tmp remote ip: %s", tmp_remote_memory->ip);
                                        //DBG(L(INFO), "curr ip: %s", curr_ip_store);
                                        if (!strcmp(tmp_remote_memory->ip, curr_ip_store))
                                        {
                                                tmp_remote_memory->pulled_num += 1;
                                                break;
                                        }
                                }
                                free(curr_ip_store);

                                ctx->msg->id = CLIENT_DONE;
                                //PER_LOG var
                                ctx->need_log = 1;
                                ctx->func = PULL;
                                //strcpy(ctx->msg->data.mr.file_name, ctx->file_name);
                                ctx->msg->data.mr.file_size = ctx->file_size;

                                get_ip(id);

                                strcpy(ctx->msg->ip, global_mem_info->ip);
                                ctx->msg->data.mr.addr = (uint64_t)ctx->buffer;
                                ctx->msg->data.mr.origin_addr = ctx->peer_addr;
                                ctx->msg->data.mr.rkey = ctx->buffer_mr->rkey;

                                #ifdef TEST_BREAKDOWN_PULL
                                // unsigned long long pull_post_receive_start = get_now_time_us();
                                #endif
                                post_receive(id);

                                #ifdef TEST_BREAKDOWN_PULL
                                // unsigned long long pull_post_receive_end = get_now_time_us();
                                // DBG(L(DATA), "501,rdma_post_receive,pull_rdma_post_receive,%lld,%lld,%lld,%s", pull_post_receive_start, pull_post_receive_end, pull_post_receive_end-pull_post_receive_start, get_now_time());
                                
                                // unsigned long long pull_send_message_start = get_now_time_us();
                                #endif

                                send_message(id);

                                #ifdef TEST_BREAKDOWN_PULL
                                // unsigned long long pull_send_message_end = get_now_time_us();
                                // DBG(L(DATA), "501,rdma_send_message,pull_rdma_send_message,%lld,%lld,%lld,%s", pull_send_message_start, pull_send_message_end, pull_send_message_end-pull_send_message_start, get_now_time());
                                #endif

                                //printf("receive done, disconnecting!");
                                //rc_disconnect(id);
                        }
                        else
                        {
                                DBG(L(INFO), "tmp_memory_rate >= T1");
                                ctx->msg->id = PULL_REQUEST_BUT_NOT_PULL;
                                send_message(id);

                                rc_disconnect(id);
                        }
                }

                //for client mode, on_completion()
                if (ctx->msg->id == PAGEFAULT_EVICTED_REQUEST_DONE)
                {
                        uint64_t local_addr = ctx->msg->data.mr.addr;
                        struct buffer_from_remote *curr_from_remote = NULL; //, *pre_from_remote = NULL;

                        #ifdef TEST_BREAKDOWN_PAGEFAULT
                        unsigned long long pagefault_step8_start = get_now_time_us();
                        #endif

                        rbtree_node *rb_from_node = rbtree_search(from_remote_tree, (unsigned long)local_addr);

                        #ifdef TEST_BREAKDOWN_PAGEFAULT
                        unsigned long long pagefault_step8_end = get_now_time_us();
                        DBG(L(DATA), "301,pagefault_step8,rbtree_search_from_remote_node,%lld,%lld,%lld,%s", pagefault_step8_start, pagefault_step8_end, pagefault_step8_end-pagefault_step8_start, get_now_time());
                        #endif

                        if (rb_from_node != NULL)
                        {
                                curr_from_remote = (struct buffer_from_remote *)rb_from_node->struct_addr;
                        }
                        if (curr_from_remote != NULL && curr_from_remote->status == EVICTING)
                        {
                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step9_start = get_now_time_us();
                                #endif

                                rbtree_delete(from_remote_tree, rb_from_node);

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step9_end = get_now_time_us();
                                DBG(L(DATA), "301,pagefault_step9,rbtree_del_from_remote_node,%lld,%lld,%lld,%s", pagefault_step9_start, pagefault_step9_end, pagefault_step9_end-pagefault_step9_start, get_now_time());

                                unsigned long long pagefault_step10_start = get_now_time_us();
                                #endif

                                del_from_remote_node(curr_from_remote);

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step10_end = get_now_time_us();
                                DBG(L(DATA), "301,pagefault_step10,list_del_from_remote_node,%lld,%lld,%lld,%s", pagefault_step10_start, pagefault_step10_end, pagefault_step10_end-pagefault_step10_start, get_now_time());
                                #endif

                                from_remote_num--;

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step11_start = get_now_time_us();
                                #endif

                                struct memory_from_remote *tmp_remote_memory = remote_memory_head;
                                while (tmp_remote_memory->next != NULL)
                                {
                                        tmp_remote_memory = tmp_remote_memory->next;
                                        if (!strcmp(tmp_remote_memory->ip, curr_from_remote->ip))
                                        {
                                                tmp_remote_memory->pulled_num -= 1;
                                                break;
                                        }
                                }

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step11_end = get_now_time_us();
                                DBG(L(DATA), "301,pagefault_step11,update_remote_machine_memory_status,%lld,%lld,%lld,%s", pagefault_step11_start, pagefault_step11_end, pagefault_step11_end-pagefault_step11_start, get_now_time());
                                #endif

                                madvise((void *)curr_from_remote->local_addr, curr_from_remote->size, MADV_DONTNEED);
                                
                                free((void *)curr_from_remote->local_addr);

                                from_remote_total_size -= curr_from_remote->size;

                                curr_from_remote->status = EVICT_DONE;
                                free(curr_from_remote);

                                context_clear(ctx);

                                rc_disconnect(id);
                        }
                        else
                        {
                                rc_disconnect(id);
                                DBG(L(WARNING), "addr not in from_remote list!");
                        }
                }

                //for server mode, on_completion()
                if (ctx->msg->id == PAGEFAULT_EVICTED_REQUEST_RET)
                {
                        uint64_t origin_addr = ctx->msg->data.mr.origin_addr;

                        struct buffer_in_remote *curr_in_remote = NULL;

                        #ifdef TEST_BREAKDOWN_PAGEFAULT
                        unsigned long long pagefault_step4_start = get_now_time_us();
                        #endif

                        rbtree_node *rb_in_node = rbtree_search(in_remote_tree, (unsigned long)origin_addr);

                        #ifdef TEST_BREAKDOWN_PAGEFAULT
                        unsigned long long pagefault_step4_end = get_now_time_us();
                        DBG(L(DATA), "301,pagefault_step4,rbtree_search_in_remote_node,%lld,%lld,%lld,%s", pagefault_step4_start, pagefault_step4_end, pagefault_step4_end-pagefault_step4_start, get_now_time());
                        #endif

                        if (rb_in_node != NULL)
                        {
                                curr_in_remote = (struct buffer_in_remote *)rb_in_node->struct_addr;
                        }

                        if (curr_in_remote != NULL)
                        {
                                int size = curr_in_remote->size;

                                //printf("ctx->buffer addr: %p", ctx->buffer);
                                //printf("msg file size: %d", size);
                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step5_1_1_start = get_now_time_us();
                                #endif

                                // posix_memalign((void **)&ctx->buffer, sysconf(_SC_PAGESIZE), size);
                                // ctx->buffer = (char *)malloc(size);
                                ctx->buffer = (char *)(curr_in_remote->local_addr);
                                unregister_userfaultfd((unsigned long)curr_in_remote->local_addr, curr_in_remote->size);

                                // #ifdef TEST_BREAKDOWN_PAGEFAULT
                                // unsigned long long pagefault_step5_1_1_end = get_now_time_us();
                                // DBG(L(DATA), "301,pagefault_step5_1_1,malloc,%lld,%lld,%lld,%s", pagefault_step5_1_1_start, pagefault_step5_1_1_end, pagefault_step5_1_1_end-pagefault_step5_1_1_start, get_now_time());

                                // unsigned long long pagefault_step5_1_2_start = get_now_time_us();
                                // #endif

                                // abin
                                // DBG(L(TEST), "ctx->buffer_mr:%p", ctx->buffer_mr);
                                ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, size, IBV_ACCESS_LOCAL_WRITE);
                                if (ctx->buffer_mr == NULL) {
                                        DBG(L(TEST), "[%s %d] ibv_reg_mr error: %s", __FILE__, __LINE__, strerror(errno));

                                        ctx->msg->id = IBVREG_ERR;
                                        send_message(id);

                                        context_clear(ctx);
                                        return;
                                }

                                // #ifdef TEST_BREAKDOWN_PAGEFAULT
                                // unsigned long long pagefault_step5_1_2_end = get_now_time_us();
                                // DBG(L(DATA), "301,pagefault_step5_1_2,ibv_reg_mr,%lld,%lld,%lld,%s", pagefault_step5_1_2_start, pagefault_step5_1_2_end, pagefault_step5_1_2_end-pagefault_step5_1_2_start, get_now_time());

                                //该步骤性能测试可忽略
                                // unsigned long long pagefault_step5_1_start = get_now_time_us();
                                // #endif

                                ctx->file_size = curr_in_remote->size;
                                ctx->peer_addr = curr_in_remote->remote_addr;
                                ctx->peer_rkey = curr_in_remote->rkey;

                                // #ifdef TEST_BREAKDOWN_PAGEFAULT
                                // unsigned long long pagefault_step5_1_end = get_now_time_us();
                                // DBG(L(DATA), "301,pagefault_step5_1,ctx,%lld,%lld,%lld,%s", pagefault_step5_1_start, pagefault_step5_1_end, pagefault_step5_1_end-pagefault_step5_1_start, get_now_time());

                                // unsigned long long pagefault_step5_2_start = get_now_time_us();
                                // #endif

                                read_remote(id, size);

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step5_2_end = get_now_time_us();
                                DBG(L(DATA), "301,pagefault_step5_2,pagefault_read_data,%lld,%lld,%lld,%s", pagefault_step5_2_start, pagefault_step5_2_end, pagefault_step5_2_end-pagefault_step5_2_start, get_now_time());
                                #endif
                                
                                ctx->msg->data.mr.addr = curr_in_remote->remote_addr;

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step6_start = get_now_time_us();
                                #endif

                                rbtree_delete(in_remote_tree, rb_in_node);

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step6_end = get_now_time_us();
                                DBG(L(DATA), "301,pagefault_step6,rbtree_delete_in_remote_node,%lld,%lld,%lld,%s", pagefault_step6_start, pagefault_step6_end, pagefault_step6_end-pagefault_step6_start, get_now_time());

                                unsigned long long pagefault_step7_start = get_now_time_us();
                                #endif

                                del_in_remote_node(curr_in_remote);

                                #ifdef TEST_BREAKDOWN_PAGEFAULT
                                unsigned long long pagefault_step7_end = get_now_time_us();
                                DBG(L(DATA), "301,pagefault_step7,list_del_in_remote_node,%lld,%lld,%lld,%s", pagefault_step7_start, pagefault_step7_end, pagefault_step7_end-pagefault_step7_start, get_now_time());
                                #endif

                                // curr_in_remote->new_addr = (uint64_t)ctx->buffer;
                                curr_in_remote->evicted = 1;

                                //此处不释放，因为需要在缺页中断请求函数中进行释放，还需要根据in_remote中的evicted变量状态判断是否已经执行完缺页被动回迁数据操作
                                //free(curr_in_remote);

                                //PER_LOG var
                                ctx->func = PAGEFAULT_EVICTED;
                                ctx->need_log = 1;

                                //缺页被动回迁成功后
                                in_remote_num -= 1;

                                // update_lru_data((unsigned long)curr_in_remote->local_addr);
                              
                                ctx->msg->id = PAGEFAULT_EVICTED_REQUEST_DONE;//  = 17
                                // DBG(L(TEST), "2 server ctx->msg->id: %d", ctx->msg->id);
                                send_message(id);

                                context_clear(ctx);
                        }
                        else
                        {
                                context_clear(ctx);

                                rc_disconnect(id);
                        }
                }

                if (ctx->msg->id == MEM_INFO_RET)
                {
                        float mem_rate = ctx->msg->data.mem_info.memory_rate;
                        char *ip = ctx->msg->ip;

                        struct memory_from_remote *tmp_remote_memory = remote_memory_head;
                        struct memory_from_remote *curr_remote_memory = NULL;
                        while (tmp_remote_memory->next != NULL)
                        {
                                tmp_remote_memory = tmp_remote_memory->next;
                                if (!strcmp(tmp_remote_memory->ip, ip))
                                {
                                        curr_remote_memory = tmp_remote_memory;
                                        break;
                                }
                        }
                        if (curr_remote_memory != NULL)
                        {
                                curr_remote_memory->mem_rate = mem_rate;
                                curr_remote_memory->need_pulled = ctx->msg->data.mem_info.need_pulled;
                        }
                        else
                        {
                                struct memory_from_remote *new_remote_memory = (struct memory_from_remote *)malloc(sizeof(struct memory_from_remote));
                                strcpy(new_remote_memory->ip, ip);
                                new_remote_memory->mem_rate = mem_rate;
                                new_remote_memory->pulled_num = 0;
                                new_remote_memory->need_pulled = ctx->msg->data.mem_info.need_pulled;
                                new_remote_memory->next = NULL;

                                remote_memory_tail->next = new_remote_memory;
                                remote_memory_tail = new_remote_memory;
                        }
                        //printf("received from: %s, memory rate: %lf%%",ip,mem_rate);
                        rc_disconnect(id);
                }

                //for client   on_completion()
                if (ctx->msg->id == EVICT_REQUEST_RET)
                {
                        #ifdef EVICT_DEBUG
                        DBG(L(TEST), "prc.c 1116. start EVICT_REQUEST_RET. evict_data4. debug_num=%d",debug_num);
                        #endif

                        struct buffer_from_remote *curr_from_remote = NULL;
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step8_start = get_now_time_us();
                        #endif
                        rbtree_node *rb_from_node = rbtree_search(from_remote_tree, (unsigned long)ctx->msg->data.mr.addr);
                        #ifdef TEST_BREAKDOWN_EVICT
                        unsigned long long evict_step8_end = get_now_time_us();
                        DBG(L(DATA), "201,evict_step8,rbtree_search_from_remote_node,%lld,%lld,%lld,%s", evict_step8_start, evict_step8_end, evict_step8_end-evict_step8_start, get_now_time());
                        #endif
                        if (rb_from_node != NULL)
                        {
                                curr_from_remote = (struct buffer_from_remote *)rb_from_node->struct_addr;
                        }

                        if (curr_from_remote != NULL)
                        {
                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_ibv_dereg_mr_start = get_now_time_us();
                                #endif

                                ibv_dereg_mr(curr_from_remote->buffer_mr);

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_ibv_dereg_mr_end = get_now_time_us();
                                DBG(L(DATA), "201,evict_step9_0,client_ibv_dereg_mr,%lld,%lld,%lld,%s", evict_ibv_dereg_mr_start, evict_ibv_dereg_mr_end, evict_ibv_dereg_mr_end-evict_ibv_dereg_mr_start, get_now_time());
                                // unsigned long long evict_rbtree_search_start = get_now_time_us();
                                #endif

                                rbtree_node *rb_from_node = rbtree_search(from_remote_tree, (unsigned long)ctx->msg->data.mr.addr);

                                // #ifdef TEST_BREAKDOWN_EVICT
                                // unsigned long long evict_rbtree_search_end = get_now_time_us();
                                // DBG(L(DATA), "201,evict_pre_step9,rbtree_search,%lld,%lld,%lld,%s", evict_rbtree_search_start, evict_rbtree_search_end, evict_rbtree_search_end-evict_rbtree_search_start, get_now_time());
                                // #endif

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step9_1_start = get_now_time_us();
                                #endif

                                madvise((void *)curr_from_remote->local_addr, curr_from_remote->size, MADV_DONTNEED);

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step9_1_end = get_now_time_us();
                                DBG(L(DATA), "201,evict_step9_1,client_madvise,%lld,%lld,%lld,%s", evict_step9_1_start, evict_step9_1_end, evict_step9_1_end-evict_step9_1_start, get_now_time());
                                #endif

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step9_2_start = get_now_time_us();
                                #endif

                                free((void *)curr_from_remote->local_addr);

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step9_2_end = get_now_time_us();
                                DBG(L(DATA), "201,evict_step9_2,client_free,%lld,%lld,%lld,%s", evict_step9_2_start, evict_step9_2_end, evict_step9_2_end-evict_step9_2_start, get_now_time());
                                #endif

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step9_start = get_now_time_us();
                                #endif

                                rbtree_delete(from_remote_tree, rb_from_node);

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step9_end = get_now_time_us();
                                DBG(L(DATA), "201,evict_step9,rbtree_delete_from_remote_node,%lld,%lld,%lld,%s", evict_step9_start, evict_step9_end, evict_step9_end-evict_step9_start, get_now_time());
                                
                                unsigned long long evict_step10_start = get_now_time_us();
                                #endif

                                del_from_remote_node(curr_from_remote);

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step10_end = get_now_time_us();
                                DBG(L(DATA), "201,evict_step10,list_del_from_remote_node,%lld,%lld,%lld,%s", evict_step10_start, evict_step10_end, evict_step10_end-evict_step10_start, get_now_time());
                                #endif

                                from_remote_num -= 1;

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step11_start = get_now_time_us();
                                #endif

                                struct memory_from_remote *tmp_remote_memory = remote_memory_head;
                                while (tmp_remote_memory->next != NULL)
                                {
                                        tmp_remote_memory = tmp_remote_memory->next;
                                        //DBG(L(INFO), "tmp remote ip: %s", tmp_remote_memory->ip);
                                        //DBG(L(INFO), "del from remote ip: %s", curr_from_remote->ip);
                                        if (!strcmp(tmp_remote_memory->ip, curr_from_remote->ip))
                                        {
                                                tmp_remote_memory->pulled_num -= 1;
                                                break;
                                        }
                                }

                                #ifdef TEST_BREAKDOWN_EVICT
                                unsigned long long evict_step11_end = get_now_time_us();
                                DBG(L(DATA), "201,evict_step11,update_remote_status,%lld,%lld,%lld,%s", evict_step11_start, evict_step11_end, evict_step11_end-evict_step11_start, get_now_time());
                                #endif

                                //PER_LOG var
                                ctx->file_size = curr_from_remote->size;
                                ctx->func = EVICT;
                                ctx->need_log = 1;

                                from_remote_total_size -= ctx->file_size;

                                curr_from_remote->status = EVICT_DONE;
                                free(curr_from_remote);
                                #ifdef EVICT_DEBUG
                                DBG(L(TEST), "prc.c 1211. end EVICT_REQUEST_RET. evict_data5. debuge_num=%d",debug_num);
                                #endif
                        }
                        else
                        {
                                DBG(L(ERR), "the curr_from_remote is NULL!");
                        }
                        rc_disconnect(id);
                }

                if (ctx->msg->id == EVICTED_REQUEST_FOR_TEST_RET)
                {
                        rc_disconnect(id);
                }

                if (ctx->msg->id == PAGEFAULT_EVICTED_REQUEST_FOR_TEST_RET)
                {
                        rc_disconnect(id);
                }

                if (ctx->msg->id == PAGEFAULT_EVICTED_REQUEST_RET_NO_ADDR)
                {
                        DBG(L(TEST), "GET PAGEFAULT_EVICTED_REQUEST_RET_NO_ADDR");
                        rc_disconnect(id);
                }

                if (ctx->msg->id == PULL_REQUEST_RET_PULLED_BY_OTHER)
                {
                        DBG(L(INFO), "data in this host is pulled by other!");
                        rc_disconnect(id);
                }

                if (ctx->msg->id == PULL_REQUEST_RET_NO_DATA || ctx->msg->id == PULL_REQUEST_RET_NOT_NEED)
                {
                        DBG(L(INFO), "no data to be pulled in: %s", ctx->msg->ip);
                        struct memory_from_remote *tmp_remote_memory = remote_memory_head;
                        while (tmp_remote_memory->next != NULL)
                        {
                                tmp_remote_memory = tmp_remote_memory->next;
                                if (!strcmp(ctx->msg->ip, tmp_remote_memory->ip))
                                {
                                        tmp_remote_memory->need_pulled = 0;
                                        break;
                                }
                        }
                        rc_disconnect(id);
                }
                if (ctx->msg->id == IBVREG_ERR)
                {
                        context_clear(ctx);

                        rc_disconnect(id);
                }
        }else{
                //TODO
        }
}

/**
 * @description: 当一方发起rc_disconnect请求之后，发起端和接收端都会执行此函数，主要用于释放buffer等操作，flag参数标记该机器处于server端还是client端，从common.c中传过来
 * @param {struct rdma_cm_id *, int}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void on_disconnect(struct rdma_cm_id *id, int flag)
{
        // DBG(L(TEST), "~on_disconnect, start");
        struct rdma_context *ctx = (struct rdma_context *)id->context;
        // DBG(L(TEST), "~on_disconnect, 1");
        // DBG(L(INFO), "@ flag: %d", flag);
        if (flag)
        {
                // DBG(L(TEST), "~on_disconnect, 2");
                if (ctx->fd != NULL)
                {
                        // DBG(L(TEST), "~on_disconnect, 2-1-1");
                        fclose(ctx->fd);
                        // DBG(L(TEST), "~on_disconnect, 2-1-2");
                        ctx->fd = NULL;
                }

                if (ctx->buffer_mr != NULL)
                {
                        // DBG(L(TEST), "~on_disconnect, 2-2-1");
                        // DBG(L(TEST), "ctx->buffer_mr: %p", ctx->buffer_mr);
                        ibv_dereg_mr(ctx->buffer_mr);
                        // DBG(L(TEST), "~on_disconnect, 2-2-2");
                }

                if (ctx->buffer != NULL)
                {
                        // DBG(L(TEST), "~on_disconnect, 2-3-1");
                        free(ctx->buffer);
                        // DBG(L(TEST), "~on_disconnect, 2-3-2");
                        ctx->buffer = NULL;
                }
        }

        // DBG(L(TEST), "~on_disconnect, 3");
        ibv_dereg_mr(ctx->msg_mr);
        // DBG(L(TEST), "~on_disconnect, 4");
        free(ctx->msg);

        //PER_LOG end
        // DBG(L(TEST), "~on_disconnect, 8");
        free(ctx);
        // DBG(L(TEST), "~on_disconnect, end");
}

/**
 * @description: rpc初始化，将四个函数传递给common.h中，同时进行迁移程序的初始化
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rpc_init()
{
        rc_init(
            on_pre_conn,
            on_connection,
            on_completion,
            on_disconnect);
}

