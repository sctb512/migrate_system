#include "perf_base.h"

/*
*测试程序，访问远程数据
    出发缺页中断功能测试
**/
void access_in_remote(int num)
{
        int i = 0;
        struct buffer_in_remote *tmp_in_remote = in_remote_head->next;
        while (tmp_in_remote != NULL)
        {
                if(i == num)
                {
                        DBG(L(TEST), "perf.c access_in_remote():pagefault done, read data num: %d", num);
                        break;
                }

                struct  buffer_in_remote * next_in_remote = tmp_in_remote->next;
                i++;

                char *addr = (char *)tmp_in_remote->local_addr;
                int j;
                for (j = 0; j < 5; j++)
                {
                        printf("access_in_remote, addr[%d] = %c \n", j, addr[j]);
                }

                //?
                sleep_ms(40);

                DBG(L(TEST), "access_in_remote() in_remote_num:%d. has done num:%d", in_remote_num,i);
                tmp_in_remote = next_in_remote;
        }
        if (i != num)
        {
                DBG(L(TEST), "not so many buffer in remote, only %d!\n", i);
        }
}

/**
 * PER_LOG FUNC1 专用测试函数1
 * 测试内存使用率 随着 ‘lru_data SIZE的增大’&&‘换入换出系统执行’ 的变化情况
 * 测试内存使用率 随着 ‘lru_data SIZE的增大’&&‘换入换出系统执行’ && ‘运行时间’ 的变化情况
 *@ 使用真实的performance_test_memory函数创造线程，以异步线程的方式运行自定义测试函数
*/
void performance_test_memory(void *num)
{
        DBG(L(TEST), "performance_test_memory will start after 6s...");
        sleep(6);

        // 单机运行测试performance_test_memory函数时，需要用到test_flag，test_flag为1时，会进行一些必要的初始化，使当前函数能够正确运行
        if (test_flag)
        {
                lru_data_init();
                global_memory_init();
                init_userfaultfd();
        }

        while(!global_mem_info);

        perf_data_init();
        if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
        {
                DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "mem_total", "mem_used", "mem_rate", "lru_total", "elapsed_time", "time");
        }

        int i = 0;
        while (1)
        {
                if (i == *(int *)num)
                {
                        break;
                }

                int t_size = new_one_lru_data();
                // if (lru_data_head->next != NULL)
                // {
                //         global_mem_info->need_pulled = 1;
                // }
                lru_size_total += t_size;
                // sleep(1);
                //睡眠50ms才继续下一次循环
                sleep_ms(50);

                get_memory_rate();

                DBG(L(DATA), "%lld,%ld,%.2lf,%f,%ld,%s", global_mem_info->total, global_mem_info->used, global_mem_info->mem_rate, (float)lru_size_total/(1024*1024), get_now_time_ms(), get_now_time());
                // DBG(L(INFO), "%lld,%ld,%.2lf,%f,%d,%s", global_mem_info->total, global_mem_info->used, global_mem_info->mem_rate, (float)lru_size_total/(1024*1024), get_now_time_us(), get_now_time());
                if (global_mem_info->mem_rate > 98)
                {
                        DBG(L(TEST), "mem rate: %lf%%", global_mem_info->mem_rate);
                        DBG(L(TEST), "times: %d", i+1);
                        // exit(0);
                        break;
                }
                i++;

        }
        DBG_WRITE();
        DBG(L(TEST), "performance_test_memory done!");
        exit(0);
}


//缺页中断访问数量(如果变动值，需要改client,server两端测试程序的值)
int access_num;
/**
 * PER_LOG FUNC2 专用测试函数2
 * server端 测试程序
 * 先增加数据超过T1，然后超过T2
*/
void performance_test_func_time_server()
{
        float t1,t2,t3,t4,t5;
        

        #ifdef DEV
        t1 = 72;
        t2 = 10;
        t3 = 65;
        t4 = 96;
        t5 = 0.3;
        access_num = 100;
        #else
        t1 = 9.216;
        t2 = 0.92;      //是根据client端拉取了数据总尺寸进行计算的,根据拉取个数进行计算
        t3 = 8.01;
        t4 = 12.288;
        t5 = 0.0384;
        access_num = 800;
        #endif

        // DBG(L(TEST), "t1 : %f", t1);

        perf_data_init();

        DBG(L(TEST), "performance_test_func_time_server will start after 10s...");
        sleep(10);
        while(!global_mem_info);
        if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
        {
                DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
        }

        get_memory_rate();
        DBG(L(TEST), "server 1) before malloc: %lf%%", global_mem_info->mem_rate);
        long buffer_size = global_mem_info->total * t5 * 1024;
        char *release_buffer = (char *)malloc(buffer_size);
        // DBG(L(TEST), "buffer_size: %ld", buffer_size);
        // DBG(L(TEST), "release_buffer: %p", release_buffer);
        memset(release_buffer, 'A', buffer_size);
        get_memory_rate();
        DBG(L(TEST), "server 1) after malloc: %lf%%", global_mem_info->mem_rate);
        // DBG(L(TEST), "global_mem_info: %p", global_mem_info);
        float base_rate = global_mem_info->mem_rate;


        //先增加数据，让其达到t1设定值,这样使其有充分的数据块可被拉
        int i = 0;
        float total = 0;
        while (1)
        {
                get_memory_rate();

                //t1就是说防止空闲机器没有拉，而这边一直在增加过多
                if (total*100.0 / (global_mem_info->total * 1024.0) + base_rate > t1)   //9.126
                {
                        DBG(L(TEST), "server while(1) add new lru_data up to > %lf%%", t1);
                        break;
                }
                //内存使用率达到95%，为避免机器崩溃，退出测试程序
                // if (global_mem_info->mem_rate > 95)
                // {
                //         DBG(L(TEST), "mem rate: %lf%%", global_mem_info->mem_rate);
                //         DBG(L(TEST), "times: %d", i+1);
                //         break;
                // }

                sleep_ms(50);//每隔50ms产生新的一块数据
                int t_size = new_one_lru_data_solid_size(server_lru_size);
                total += t_size;
                i++;
        }

        //这时候测试空闲机器拉取
        while (1)
        {
                get_memory_rate();
                if(in_remote_num == 0)
                {
                        DBG(L(TEST), "server while(2) after client evict, mem rate up to: %lf%%", global_mem_info->mem_rate);
                        DBG(L(TEST), "server while(2) waitting for client pull...");
                        while (1)
                        {
                                //拉取个数有限制，拉取数量超过t2就停止增加新数据
                                if ((in_remote_num * server_lru_size *100.0 / 1024.0) / global_mem_info->total > t2)    //t2值要根据拉取多少个进行计算
                                {
                                        // DBG(L(TEST), "test_server(): in_remote_num: %d", in_remote_num);
                                        break;
                                }
                                sleep_ms(10);
                        }
                        //
                        DBG(L(TEST), "server while(2) server pulled up to %f%%, free buffer...",t2);
                        get_memory_rate();
                        DBG(L(TEST), "server 1) before free, mem rate: %lf%%", global_mem_info->mem_rate);
                        madvise(release_buffer, buffer_size, MADV_DONTNEED);
                        free(release_buffer);
                        get_memory_rate();
                        DBG(L(TEST), "server 1) after free, mem rate: %lf%%", global_mem_info->mem_rate);
                        break;
                }

                sleep_ms(100);
                // new_one_lru_data_solid_size();
        }

        //此时启动了被动回迁 功能，判断是否完成
        while (1)
        {
                if(in_remote_num == 0)
                {
                        DBG(L(TEST), "server while(3) test evicted done! break.");
                        break;
                }
                // else{
                //         DBG(L(TEST), "server while(3) test evicted not done! in_remote_num >0");
                // }
                sleep(1);
        }

        sleep(15);
        //等待对方拉数据完成后，再开始测试缺页访问中断
        while (1)
        {
                // DBG(L(TEST), "server while(4) start to test page_fault_fd fun");
                get_memory_rate();
                if(global_mem_info->mem_rate > t3)
                {
                        DBG(L(TEST), "server while(4) mem_rate up to %f%%, access in remote buffer 100 times...",t3);
                        access_in_remote(access_num);
                        DBG(L(TEST), "server while(4) access in remote buffer done, mem rate: %lf%%", global_mem_info->mem_rate);
                        break;
                }
                else{
                        DBG(L(TEST), "server while(4) mem_rate.wait page_fault!mem_rate = %f%%. t3=%f%%",t3,global_mem_info->mem_rate);
                }
                sleep_ms(100);
                new_one_lru_data_solid_size(server_lru_size);
        }

        int flag = 1;
        while (1)
        {
                get_memory_rate();

                //内存使用率达到96%，为避免机器崩溃，退出测试程序
                if (global_mem_info->mem_rate > t4)
                {
                        DBG(L(TEST), "server while(5) mem_rate up to t4: %f%%.break", t4);
                        break;
                }

                if (global_mem_info->mem_rate > T2)
                {
                        if (flag)
                        {
                                DBG(L(TEST), "server while(5) mem_rate up to T2, start to_ssd. Still add new data..");
                                flag = 0;
                        }
                }

                sleep_ms(50);
                new_one_lru_data_solid_size(server_lru_size);
        }

        sleep(60);

        DBG_WRITE();
        DBG(L(TEST), "server test_func done!");
        exit(0);
}

/**
 * PER_LOG FUNC2 专用测试函数2
 * client端 测试程序
 * 先增加数据超过T1，然后超过T2
*/
void performance_test_func_time_client()
{
        float t1,t2,t3,t4,t5,t6;
        
        #ifdef DEV
        t1 = 0.36;
        t2 = 72;
        t3 = 96;
        t4 = 10;
        t5 = 0.15;
        t6 = 0.01;
        access_num = 100;
        #else
        t1 = 0.026;     //malloc的比例
        t2 = 9.216;
        t3 =12.288;
        t4 = 1.10;      //t4值跟rpc.c run()中t1值息息相关！！！t4 < run()-t1
        t5 = 0.0192;
        t6 = 0.00828;   //缺页中断完成后只拉取一点点数据就不拉取了
        access_num = 800;
        #endif

        perf_data_init();

        DBG(L(TEST), "performance_test_func_time_client will start after 10s...");
        sleep(10);
        while(!global_mem_info);
        if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
        {
                DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
        }

        //先malloc(t1)使得mem_rate增大
        get_memory_rate();
        DBG(L(TEST), "client 1) before malloc: %lf%%", global_mem_info->mem_rate);
        long buffer_size = global_mem_info->total * t1 * 1024;
        char *release_buffer = (char *)malloc(buffer_size);
        memset(release_buffer, 'A', buffer_size);
        get_memory_rate();
        DBG(L(TEST), "client 1) after malloc: %lf%%, add %f%%", global_mem_info->mem_rate, t1*100);

        /*开始拉取数据
        client第一个循环可以完测试系统的(1)拉取，(2)主动回迁和(3)被动回迁功能
        */
        int i = 0, flag = 0;
        while (1)
        {
                get_memory_rate();
                
                //mem_rate达到t2就退出此次循环  正常该循环应该从这个推出
                if (global_mem_info->mem_rate > t2)   //达到最大数据数目时退出
                {
                        DBG(L(TEST), "client while(1):mem_rate up to %f%%. break while！", t2);
                        break;
                }

                //内存使用率达到96%，为避免机器崩溃，退出测试程序
                if (global_mem_info->mem_rate > t3)
                {
                        DBG(L(TEST), "client while(1): mem_rate up to: %lf%%",t3);
                        // DBG(L(TEST), "times: %d", i+1);
                        break;
                }

                //重点：判断拉取的数据量是否满足t4
                if(from_remote_total_size*100.0 / (global_mem_info->total * 1024.0) > t4)
                {
                        // DBG(L(TEST), "client while(1): has pull data up to: %f%%. wait to EVICT", t4);
                        flag = 1;
                }
                // else{
                //         DBG(L(TEST), "client not pull enough! t4 not right? flag = %d", flag);
                // }
                
                //当拉取数据量满足t4的时候，client端就开始自己新增数据直到t2
                if (flag)
                {
                        int t_size = new_one_lru_data_solid_size(client_lru_size);
                        lru_size_total += t_size;
                        sleep_ms(20);
                        i++;
                }
        }

        //当所有拉取的数据主动回迁结束后，就开始释放之前malloc的buffer
        while (1)
        {
                if (from_remote_num == 0)
                {
                        DBG(L(TEST), "client while(2):evict done, free buffer...");
                        get_memory_rate();
                        DBG(L(TEST), "client 1) before free: %lf%%", global_mem_info->mem_rate);
                        madvise(release_buffer, buffer_size, MADV_DONTNEED);
                        free(release_buffer);
                        get_memory_rate();
                        DBG(L(TEST), "client 1) after free: %lf%%", global_mem_info->mem_rate);
                        break;
                }
                // DBG(L(TEST), "from remote num: %d", from_remote_num);
                // sleep(1);
        }

        sleep(3);
        /*随着释放mallo(t1)后，mem_rate<T1, client又开始拉取数据,
                此处测试系统的(4)缺页中断访问 功能*/
        while (1)
        {
                //只拉取t4数据量的远程机器数据
                // if ((from_remote_num * server_lru_size * 100.0 / 1024.0) / global_mem_info->total  > t4)
                if (from_remote_num > access_num)
                {
                        // DBG(L(TEST), "Two.client has pulled data > %f%%. break", t4);
                        DBG(L(TEST), "client while(3) has pulled engouth to access_num. break");
                        // test_client_func_pull_flag = 0;
                        break;
                }
                // else{
                //         // DBG(L(TEST), "Two.client pulled data < %f%%", t4);
                //         DBG(L(TEST), "client while(3) hasn't pulled enough!");
                // }
                // get_memory_rate();
                sleep_ms(50);
        }

        sleep(10);
        //此时server端要测试缺页中断访问
        while (1)
        {
                if(from_remote_total_size*100.0 / (global_mem_info->total * 1024.0) > t6)
                {
                        DBG(L(TEST), "client while(4): has pull data up to: %f%%. wait to EVICT. break", t6*100);
                        // test_client_func_pull_flag = 1;
                        break;
                }
                else
                {
                        DBG(L(TEST), "client while(4) pagefault ing...! from_remote_num:%d",from_remote_num);
                }
                sleep(1);
        }

        //client端不新增数据
        char *release_new;
        while (1)
        {
                get_memory_rate();
                //判断再拉取一个是否就会超过T1值
                if (server_lru_size / 1024.0 / global_mem_info->total + global_mem_info->mem_rate > T1)
                {
                        DBG(L(TEST), "client while(5) has pulled data up to: %lf%%", (from_remote_num * server_lru_size * 100.0 / 1024.0) / global_mem_info->total);
                        // test_client_func_pull_flag = 0;
                        get_memory_rate();

                        //此时client的内存使用率接近T1，在malloc一个buffer
                        DBG(L(TEST), "client 2) before malloc: %lf%%", global_mem_info->mem_rate);
                        int size = global_mem_info->total * t5 * 1024;
                        release_new = (char *)malloc(size);
                        get_memory_rate();
                        DBG(L(TEST), "client 2) after malloc: %lf%%", global_mem_info->mem_rate);
                        DBG(L(TEST), "client while(5) not add new lru_data");
                        //完成后，就等待对方执行缺页中断数据访问了
                        break;
                }
                sleep_ms(50);
        }

        sleep(120);
        free(release_new);

        DBG_WRITE();
        DBG(L(TEST), "clien test_func done!");
        // exit(0);
}

/**
 * @description: 测试rdma通信和rpc时用，创建一个rdma监听线程用于接收rdma连接请求，并循环获取本机的内存使用率
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void server_mode()
{
        pthread_t server_thread;

        int server_thread_code = pthread_create(&server_thread, NULL, (void *)&rc_server_loop, (void *)DEFAULT_PORT);
        if (server_thread_code)
        {
                DBG(L(INFO), "create pthread failed!");
        }
        else
        {
                DBG(L(INFO), "create pthread success!");
        }

        while (1)
        {
                get_memory_rate();
                DBG(L(INFO), "memory rate: %f%% ", global_mem_info->mem_rate);

                print_curr_info();

                sleep(cycle);
        }
}

/**
 * @description: 测试rdma通信和rpc时用，该函数为客户端模式，同时也会运行一个server线程，用于接收其他机器发送的rdma请求。
 *               该函数时交互式，可以通过输入来进行不同测试。
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void client_mode()
{

        pthread_t server_thread;

        int server_thread_code = pthread_create(&server_thread, NULL, (void *)&rc_server_loop, (void *)DEFAULT_PORT);
        if (server_thread_code)
        {
                DBG(L(INFO), "create pthread failed!");
        }
        else
        {
                DBG(L(INFO), "create pthread success!");
        }

        while (1)
        {
                print_curr_info();

                get_remote_memory();
                print_remote_memory();

                printf("------------------------------------\n");
                printf("| > client mode \n");
                printf("| operate case as follows:\n");
                printf("| * 1 : get memory rate\n");
                printf("| * 2 : pull data\n");
                printf("| * 3 : evict data\n");
                printf("| * 4 : evicted data\n");
                printf("| * 5 : page fault evicted data\n");
                printf("| please input your opierate:\n");
                printf("| ");

                int op = 0;
                scanf("%d", &op);

                //char host[MAX_IP_ADDR];
                switch (op)
                {
                case 1:
                        get_remote_memory();
                        break;
                case 2:
                        pull_data();
                        break;
                case 3:
                        evict_data();
                        break;
                case 4:
                        request_evicted_data();
                        break;
                case 5:
                        request_pagefault_evicted_data();
                        break;

                default:
                        printf("| operate error, please input again!\n");
                        continue;
                }

                print_curr_info();

                get_memory_rate();
                DBG(L(INFO), "local memory rate: %f%% ", global_mem_info->mem_rate);
        }
}

/**
 * @description: 测试函数的封装，通过参数控制程序处于server模式还是client模式
 * @param {int, char **}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void test(int argc, char **argv)
{

        migrate_init();

        if (argc == 2 && !strcmp(argv[1], "c"))
        {
                client_mode();
        }
        else
        {
                server_mode();
        }
}
