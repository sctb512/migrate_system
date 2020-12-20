#include "migrate.h"

/*
        client端 请求函数
        client端mem_rate< T1时，发起拉取数据骑牛
        @client 向server端发起 PULL_REQUEST 请求
*/
void pull_data()
{
        struct rdma_context ctx;
        struct request_message req_msg;

        struct memory_from_remote *tmp_remote_memory = remote_memory_head;
        struct memory_from_remote *curr_remote_memory = NULL;

        #ifdef TEST_BREAKDOWN_PULL
        // unsigned  long  long pull_step1_start = get_now_time_us();
        #endif
        //从mem_list链表中找到ip列表中，需要被拉数据的机器中，被拉取数量最少的机器ip进行建立链接
        while (tmp_remote_memory->next != NULL)
        {
                tmp_remote_memory = tmp_remote_memory->next;
                if (tmp_remote_memory->need_pulled)
                {
                        if (curr_remote_memory == NULL)
                        {
                                curr_remote_memory = tmp_remote_memory;
                        }

                        //选择被拉数据块最小的ip机器进行数据拉取请求
                        if (curr_remote_memory->pulled_num > tmp_remote_memory->pulled_num)
                        {
                                curr_remote_memory = tmp_remote_memory;
                        }
                }
        }
        #ifdef TEST_BREAKDOWN_PULL
        // unsigned  long  long pull_step1_end = get_now_time_us();
        // DBG(L(DATA), "101,pull_step1,slect_machine,%lld,%lld,%lld,%s", pull_step1_start, pull_step1_end, pull_step1_end-pull_step1_start, get_now_time());
        #endif

        req_msg.msg_type = PULL_REQUEST;
        ctx.request_msg = &req_msg;
        if (curr_remote_memory != NULL)
        {
                //start pull data, start rmda function
                rc_client_loop(curr_remote_memory->ip, DEFAULT_PORT, &ctx);
        }
        else
        {
                DBG(L(INFO), "migrate.c pull_data() no remote host need to be pulled");
        }
}


/*
server端 请求函数
server端mem_rate < T1时，主动发起被动回迁操作
@server 向client发起 EVICTED_REQUEST 请求
*/
void evicted_data()
{
        struct rdma_context *ctx = (struct rdma_context *)malloc(sizeof(struct rdma_context));
        struct request_message req_msg;

        DBG(L(INFO), "begin to evicted data !");

        if (in_remote_num > 0)
        {
                //从in_remote链表末尾节点选择，因为尾结点是热度值最高，直接选中in_remote链表的尾结点即可
                struct buffer_in_remote *evicted_remote_data = in_remote_tail;

                #ifdef TEST_BREAKDOWN_EVICTED
                unsigned long long evicted_step1_start = get_now_time_us();
                #endif

                while (evicted_remote_data->status != EVICT_WAIT && evicted_remote_data != in_remote_head)
                {
                        evicted_remote_data  =evicted_remote_data->pre;
                }

                #ifdef TEST_BREAKDOWN_EVICTED
                unsigned long long evicted_step1_end = get_now_time_us();
                DBG(L(DATA), "401,evicted_step1,select_evicted_data,%lld,%lld,%lld,%s", evicted_step1_start, evicted_step1_end, evicted_step1_end-evicted_step1_start, get_now_time());
                #endif

                if (evicted_remote_data != in_remote_head)
                {
                        //新建一个buffer，跟回迁数据块大小相同
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step2_1_start = get_now_time_us();
                        #endif

                        // posix_memalign((void **)&ctx->buffer, base_sys_page, evicted_remote_data->size);
                        ctx->buffer = (char*)malloc(evicted_remote_data->size);

                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step2_1_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step2_1,malloc,%lld,%lld,%lld,%s", evicted_step2_1_start, evicted_step2_1_end, evicted_step2_1_end-evicted_step2_1_start, get_now_time());
                        unsigned long long evicted_step2_2_start = get_now_time_us();
                        #endif

                        // TEST_Z(ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, evicted_remote_data->size, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE));
                        ctx->buffer_mr = ibv_reg_mr(rc_get_pd(), ctx->buffer, evicted_remote_data->size, IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
                        if (ctx->buffer_mr == NULL) {
                                DBG(L(TEST), "[%s %d] ibv_reg_mr error: %s", __FILE__, __LINE__, strerror(errno));
                                return;
                        }
                        
                        #ifdef TEST_BREAKDOWN_EVICTED
                        unsigned long long evicted_step2_2_end = get_now_time_us();
                        DBG(L(DATA), "401,evicted_step2_2,server_ibv_reg_mr,%lld,%lld,%lld,%s", evicted_step2_2_start, evicted_step2_2_end, evicted_step2_2_end-evicted_step2_2_start, get_now_time());
                        // unsigned long long evicted_step2_3_start = get_now_time_us();
                        #endif

                        evicted_remote_data->new_addr = (uint64_t)ctx->buffer_mr->addr; //? 重复1
                        req_msg.msg_type = EVICTED_REQUEST;
                        //传递client端那边的local_addr
                        req_msg.addr = evicted_remote_data->remote_addr;
                        //将新建buffer的地址,远程秘钥发给客户端进行处理
                        req_msg.new_addr = (uint64_t)ctx->buffer_mr->addr;
                        req_msg.new_rkey = ctx->buffer_mr->rkey;
                        //evicted_remote_data->new_addr 变量是用来传递新buffer地址的
                        // evicted_remote_data->new_addr = (uint64_t)ctx->buffer;  //? 重复2
                        ctx->request_msg = &req_msg;

                        #ifdef TEST_BREAKDOWN_EVICTED
                        // unsigned long long evicted_step2_3_end = get_now_time_us();
                        // DBG(L(DATA), "401,evicted_step2_3,ctx&req_msg,%lld,%lld,%lld,%s", evicted_step2_3_start, evicted_step2_3_end, evicted_step2_3_end-evicted_step2_3_start, get_now_time());
                        #endif

                        evicted_remote_data->status = EVICTING;
                        rc_client_loop(evicted_remote_data->ip, DEFAULT_PORT, ctx);
                }
                else
                {
                        DBG(L(INFO), "no in remote data status is EVICT_WAIT");
                }

        }
        else
        {
                DBG(L(INFO), "this host didn't be pulled data by remote machine!");
        }
}


/**
 * server端 请求函数
 * server端某应用触发 读取远程数据时，server端主动发起缺页被动回迁请求
 * @server端向client端发起 PAGEFAULT_EVICTED_REQUEST 请求
 *
*/
void pagefault_evicted_data(uint64_t local_addr)
{
        // DBG(L(TEST), "in pagefault_evicted_data local addr: %p", local_addr);
        struct buffer_in_remote *curr_in_remote = NULL;

        //先根据地址从红黑树里面找到对应的树节点
        
        #ifdef TEST_BREAKDOWN_PAGEFAULT
        unsigned long long pagefault_step2_start = get_now_time_us();
        #endif

        rbtree_node *r_node = rbtree_search(in_remote_tree, (unsigned long)local_addr);

        #ifdef TEST_BREAKDOWN_PAGEFAULT
        unsigned long long pagefault_step2_end = get_now_time_us();
        DBG(L(DATA), "301,pagefault_step2,rbtree_search_in_remote_node,%lld,%lld,%lld,%s", pagefault_step2_start, pagefault_step2_end, pagefault_step2_end-pagefault_step2_start, get_now_time());
        #endif

        if (r_node != NULL)
        {
                //然后从树节点中找到in_remote节点
                curr_in_remote = (struct buffer_in_remote *)r_node->struct_addr;
        }
        else{
                DBG(L(TEST), "01 pagefault_evicted_data: in_remote_tree not find!");
        }

        if (curr_in_remote != NULL && curr_in_remote->status == EVICT_WAIT)
        {
                curr_in_remote->status = EVICTING;

                struct rdma_context ctx;
                struct request_message req_msg;

                req_msg.msg_type = PAGEFAULT_EVICTED_REQUEST;
                //将client端的local_addr发送过去
                req_msg.addr = curr_in_remote->remote_addr;

                ctx.request_msg = &req_msg;

                // DBG(L(TEST), "01 pagefault_evicted_data: rc_client_looping...");
                // DBG(L(TEST), "ip: %s", curr_in_remote->ip);
                rc_client_loop(curr_in_remote->ip, DEFAULT_PORT, &ctx);
        }
        else
        {
                DBG(L(TEST), "01 pagefault_evicted_data: data not in remote!");
        }
}


//是否需要继续存在的函数?
void request_evicted_data()
{

        struct buffer_from_remote *tmp_from_remote = from_remote_head;
        struct buffer_from_remote *curr_from_remote = NULL;

        if (tmp_from_remote->next != NULL)
        {
                curr_from_remote = tmp_from_remote->next;
                struct rdma_context ctx;
                struct request_message req_msg;

                req_msg.msg_type = EVICTED_REQUEST_FOR_TEST;

                //以下while循环查找过程应该被替换
                while (tmp_from_remote->next != NULL)
                {
                        tmp_from_remote = tmp_from_remote->next;
                        if (tmp_from_remote->hot_degree > curr_from_remote->hot_degree)
                        {
                                curr_from_remote = tmp_from_remote;
                        }
                }

                strcpy(req_msg.ip, local_ip);
                ctx.request_msg = &req_msg;

                rc_client_loop(curr_from_remote->ip, DEFAULT_PORT, &ctx);
        }
        else
        {
                DBG(L(INFO), "no data need to be evicted!");
        }
}

//是否需要继续存在的函数?
void request_pagefault_evicted_data()
{

        struct buffer_from_remote *tmp_from_remote = from_remote_head;
        struct buffer_from_remote *curr_from_remote = NULL;

        if (tmp_from_remote->next != NULL)
        {

                curr_from_remote = tmp_from_remote->next;
                struct rdma_context ctx;
                struct request_message req_msg;

                req_msg.msg_type = PAGEFAULT_EVICTED_REQUEST_FOR_TEST;

                //以下while循环查找过程应该被替换
                while (tmp_from_remote->next != NULL)
                {
                        tmp_from_remote = tmp_from_remote->next;
                        if (tmp_from_remote->hot_degree > curr_from_remote->hot_degree)
                        {
                                curr_from_remote = tmp_from_remote;
                        }
                }

                req_msg.addr = curr_from_remote->origin_addr;
                DBG(L(INFO), "pagefault evicted data addr: %p", (void *)curr_from_remote->origin_addr);
                strcpy(req_msg.ip, local_ip);
                ctx.request_msg = &req_msg;

                rc_client_loop(curr_from_remote->ip, DEFAULT_PORT, &ctx);
        }
        else
        {
                DBG(L(INFO), "no data need to be evicted!");
        }
}

/**
 * client端 请求函数
 * client端mem_rate > T1时，主动回迁服务端的数据
 * @client 向server端发起 EVICT_REQUEST 请求
*/
void evict_data()
{
        struct rdma_context ctx;
        struct request_message req_msg;

        #ifdef EVICT_DEBUG
        debug_num++;
        #endif

        if (from_remote_num > 0)
        {
                //回迁数据块直接从from_remote链表尾部节点处获取,因为from_remote链表尾部节点是最热的节点
                // DBG(L(TEST), "from_origin addr: %p", from_remote_tail->origin_addr);
                struct buffer_from_remote *evict_remote_data = from_remote_tail;

                #ifdef TEST_BREAKDOWN_EVICT
                unsigned long long evict_step1_start = get_now_time_us();
                #endif

                while (evict_remote_data->status != EVICT_WAIT && evict_remote_data != from_remote_head)
                {
                        evict_remote_data = evict_remote_data->pre;
                }

                #ifdef TEST_BREAKDOWN_EVICT
                unsigned long long evict_step1_end = get_now_time_us();
                DBG(L(DATA), "201,evcit_step1,select_data,%lld,%lld,%lld,%s", evict_step1_start, evict_step1_end, evict_step1_end-evict_step1_start, get_now_time());
                #endif

                if (evict_remote_data != from_remote_head)
                {
                        //封装需要传递的参数
                        req_msg.msg_type = EVICT_REQUEST;
                        req_msg.addr = evict_remote_data->origin_addr;
                        strcpy(req_msg.ip, local_ip);

                        ctx.request_msg = &req_msg;

                        #ifdef EVICT_DEBUG
                        DBG(L(TEST), "migrate.c 327 befor rc_client_loop. evict_data1. debug_num=%d",debug_num);
                        #endif
                        
                        evict_remote_data->status = EVICTING;
                        rc_client_loop(evict_remote_data->ip, DEFAULT_PORT, &ctx);

                        #ifdef EVICT_DEBUG
                        DBG(L(TEST), "migrate.c 339 after rc_client_loop. evict_data1-1. debug_num=%d",debug_num);
                        #endif
                }
                else
                {
                        DBG(L(INFO), "no from remote data status is EVICT_WAIT");
                }

        }
        else
        {
                DBG(L(INFO), "this host didn't pull remote data!");
        }
}


/**
 * 主程序流程中的 功能函数
 * @ 当mem_rate > T2 且无 远程机器数据时被调用，持续执行落盘操作直到(T1+T2)/2以下才停止
 * @ 选择 热度最冷(lru_data_head开始找)&&状态是 PULL_WAIT的数据块 进行落盘
 * @ 数据块落盘后以时间戳(微妙)命名，并保存在lru_data节点里面，以便下次缺页中断读取
 *
*/
int to_ssd()
{
        //计算落盘到 (T1+T2)/2的位置
        float avg_memory = (T1 + T2) / 2;
        get_memory_rate();

        DBG(L(INFO), "@to_ssd():start to def_fsync operates!");
        // DBG(L(TEST), "global_mem_info->mem_rate: %f%%, avg_memory: %.2lf%%", global_mem_info->mem_rate, avg_memory);
        while (global_mem_info->mem_rate > avg_memory)
        {

                struct lru_data *tmp_lru_data = lru_data_head->next;
                struct lru_data *fsync_lru_data = NULL;
                char *fsync_path = (char *)malloc(sizeof(char) * MID_STR_LEN);
                // char file_name[MID_STR_LEN];

                long time;
                struct timeval tv;

                while (tmp_lru_data != NULL)
                {
                        if (tmp_lru_data->status == PULLED_WAIT)
                        {
                                fsync_lru_data = tmp_lru_data;
                                break;
                        }
                        else
                        {
                                tmp_lru_data = tmp_lru_data->next;
                        }
                }

                if (fsync_lru_data != NULL)
                {
                        fsync_lru_data->status = FSYNCING;

                        gettimeofday(&tv, NULL);
                        time = tv.tv_sec * 1000000 + tv.tv_usec;
                        sprintf(fsync_path, "%s%ld.bak", path,time);

                        int fd = open(fsync_path, O_RDWR | O_CREAT, 0644);

                        size_t write_size = write(fd, fsync_lru_data->buffer, fsync_lru_data->size);
                        // DBG(L(TEST), "fsync_lru_data->size: %d, write_size: %d", fsync_lru_data->size, write_size);
                        if (write_size)
                        {
                                fsync_lru_data->status = FSYNC_DONE;
                                strcpy(fsync_lru_data->file_name, fsync_path);
                                madvise((void *)fsync_lru_data->buffer, fsync_lru_data->size, MADV_DONTNEED);
                                register_userfaultfd((unsigned long)fsync_lru_data->buffer, fsync_lru_data->size);
                                to_ssd_num ++;

                                DBG(L(INFO), "lru_data_node fsync fall disk successful!");
                        }
                        else
                        {
                                fsync_lru_data->status = PULLED_WAIT;
                                if (remove(fsync_path) == 0)
                                        DBG(L(INFO), "remove fsync file successful!");
                                else
                                        DBG(L(ERR), "remove fsync file failed!");
                                DBG(L(WARNING), "lru_data_node fsync fall disk failed!");
                        }
                        if(fd != -1)
                        {
                                close(fd);
                        }
                        if (fsync_path != NULL)
                        {
                                free(fsync_path);
                        }
                }
                else
                {
                        DBG(L(INFO), "In to_ssd() failed by the lru_data hasn't the node who's status is PULLED_WAIT!, the mem_rata = %f ", global_mem_info->mem_rate);
                        break;
                }
                get_memory_rate();
                // printf("@ memory rate: %lf%%", global_mem_info->mem_rate);
                // printf("@ avg memory rate: %lf%%", avg_memory);
        }
        // DBG(L(TEST), "to_ssd() function end!");
        return 0;
}


/**
 * @description: 初始化lru_data链表和红黑树，这些数据将模拟冷热数据，进行程序的性能测试
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: xy
 */
void lru_data_init()
{
        lru_data_head = (struct lru_data *)malloc(sizeof(struct lru_data));
        lru_data_head->next = NULL;
        lru_data_head->pre = NULL;
        lru_data_head->status = HEAD_FLAG;
        lru_data_head->hot_degree = -1;
        lru_data_tail = lru_data_head;

        lru_data_tree = (rbtree *)malloc(sizeof(rbtree));
        rbtree_init(lru_data_tree);
}


//是否需要继续存在的函数?
void migrate_init()
{
        rpc_init();
        migcomm_init();

        init_list();
        init_tree();

        if (read_cfgs())
        {
                DBG(L(INFO), "read ips from %s successful!", IPS_FILE);
                struct ip *tmp_ip = ips;
                int i = 1;
                while (tmp_ip->next != NULL)
                {
                        tmp_ip = tmp_ip->next;
                        DBG(L(INFO), "* ip%d: %s", i++, tmp_ip->ip);
                }
        }

        //initial_lru_data(path);
        
        lru_data_init();

        struct lru_data *tmp_lru_data = lru_data_head;

        while (tmp_lru_data->next != NULL)
        {
                tmp_lru_data = tmp_lru_data->next;
                printf_lru_data(tmp_lru_data);
                DBG(L(INFO), "");
        }

        init_userfaultfd();

        if (access("file", R_OK) != 0)
        {
                if (mkdir("file", 0777))
                {
                        DBG(L(CRITICAL), "creat floder file failed!!!n");
                }
        }
}


/**
 * @description: 迁移程序的主要逻辑，能保证程序自动运行
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void run()
{
        log_init("log.txt");
        migrate_init();

        pthread_t server_thread;

        int server_thread_code = pthread_create(&server_thread, NULL, (void *)&rc_server_loop, (void *)DEFAULT_PORT);
        if (server_thread_code)
        {
                DBG(L(INFO), "rpc.c. create server pthread failed!");
        }
        else
        {
                DBG(L(INFO), "rpc.c. create server pthread success!");
        }

        while (1)
        {
                #ifdef TEST_BREAKDOWN_DEF_FUNC
                unsigned long long get_mem_status_start = get_now_time_us();
                #endif

                get_memory_rate();

                #ifdef TEST_BREAKDOWN_DEF_FUNC
                unsigned long long get_mem_status_end = get_now_time_us();
                DBG(L(DATA), "601,get_local_memory_rate,get_memory_rate(),%lld,%lld,%lld,%s", get_mem_status_start, get_mem_status_end, get_mem_status_end-get_mem_status_start, get_now_time());

                //本机更新集群节点的内存状态：内存使用率，是否需要拉取，
                unsigned long long get_remote_mem_status_start = get_now_time_us();
                #endif

                get_remote_memory();

                #ifdef TEST_BREAKDOWN_DEF_FUNC
                unsigned long long get_remote_mem_status_end = get_now_time_us();
                DBG(L(DATA), "601,get_remote_machine_memory_status,get_remote_memory(),%lld,%lld,%lld,%s", get_remote_mem_status_start, get_remote_mem_status_end, get_remote_mem_status_end-get_remote_mem_status_start, get_now_time());
                #endif

                // print_remote_memory();
                // DBG(L(TEST), "T1: %.2lf, T2: %.2lf, mem_rate: %.2lf", T1, T2, global_mem_info->mem_rate);
                if (global_mem_info->mem_rate < T1)     // memory rate < T1
                {
                        global_mem_info->need_pulled = 0;
                        if (in_remote_num > 0)          //have data in remote
                        {
                                struct buffer_in_remote *tmp_in_remote = in_remote_tail;
                                // DBG(L(INFO), "prc.c 1137 run() T<T1 && in_remote_num > 0");
                                while (tmp_in_remote != in_remote_head)
                                {
                                        float tmp_memory_rate = ((global_mem_info->used + tmp_in_remote->size / 1024.0) * 100.0) / global_mem_info->total;
                                        //DBG(L(INFO), "3tmp mem rate: %f%%, T1: %f%%", tmp_memory_rate, T1);

                                        if (tmp_memory_rate < T1)       //evicted when memory rate < T1 after evicting
                                        {
                                                tmp_in_remote = tmp_in_remote->pre;
                                                evicted_data();         // evict data initiative
                                                get_memory_rate();
                                        }
                                        else
                                        {
                                                break;
                                        }
                                }
                                if(in_remote_num == 0)
                                {
                                        pull_data();
                                }
                        }
                        else
                        {
                                pull_data();
                        }
                }
                else if (global_mem_info->mem_rate < T2)
                {
                        DBG(L(INFO),"rpc.c run():2 from_remote_num:%d", from_remote_num);
                        if (from_remote_num > 0)
                        {
                                //DBG(L(INFO), "~from_remote_total_size: %ld", from_remote_total_size);
                                float tmp_memory_rate = ((global_mem_info->used - from_remote_total_size / 1024.0) * 100.0) / global_mem_info->total;

                                if (tmp_memory_rate > T1)
                                {
                                        while (from_remote_num >0)
                                        {
                                                // tmp_from_remote = tmp_from_remote->next;
                                                #ifdef EVICT_DEBUG
                                                DBG(L(TEST), "rpc.c 1463 befor evict_data0");
                                                #endif
                                                evict_data();
                                                #ifdef EVICT_DEBUG
                                                DBG(L(TEST), "rpc.c 1465 after evict_data6");
                                                #endif
                                                sleep_ms(cycle);
                                        }
                                        global_mem_info->need_pulled = 1;
                                        get_memory_rate();
                                }
                        } else
                        {
                                global_mem_info->need_pulled = 1;
                        }
                }
                else //>T2
                {
                        if (from_remote_num > 0)
                        {
                                global_mem_info->need_pulled = 0;
                                struct buffer_from_remote *tmp_from_remote = from_remote_tail;
                                while (tmp_from_remote != from_remote_head)//确保头节点后面全部回迁完成
                                {
                                        float tmp_memory_rate = ((global_mem_info->used - tmp_from_remote->size / 1024.0) * 100.0) / global_mem_info->total;
                                        DBG(L(INFO), "5tmp mem rate: %f%%, T1: %f%%", tmp_memory_rate, T1);

                                        if (tmp_memory_rate >= T1)
                                        {
                                                tmp_from_remote = tmp_from_remote->pre;
                                                // DBG(L(TEST), "prc.c 1085.start evict. from_num: %d",from_remote_num);
                                                evict_data();
                                                sleep_ms(cycle);
                                                // DBG(L(TEST), "prc.c 1085.end evict. from_num: %d", from_remote_num);
                                                get_memory_rate();
                                        }
                                }
                                if (global_mem_info->mem_rate > T2)
                                {
                                        global_mem_info->need_pulled = 0;
                                        to_ssd();
                                }
                        }
                        else
                        {
                                global_mem_info->need_pulled = 0;
                                to_ssd();
                        }
                }
                sleep_ms(cycle);
        }
}