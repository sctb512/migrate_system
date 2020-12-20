#include "throughput.h"

void throughtput_test_base(long long total_size, long long str_size, int test_time, int threads, int times)
{
        global_memory_init();
        struct arr_struct
        {
                char item[str_size];
        };
        long arr_num = total_size / str_size;

        struct arr_struct *arr[arr_num];
        int m;
        for(m=0;m<times;m++)
        {
                DBG(L(TEST), "trun: %d", m);

                DBG(L(TEST), "sleep 6s...");
                sleep(6);

                int i;
                for(i=0;i<arr_num;i++)
                {
                        struct arr_struct *tmp_arr_struct = (struct arr_struct*)malloc(sizeof(struct arr_struct));
                        int j;
                        for(j=0;j<str_size;j++)
                        {
                                tmp_arr_struct->item[j] = 'M';
                        }
                        arr[i] = tmp_arr_struct;
                }

                DBG(L(TEST), "start thoughtput base testing...");

                long start_time = get_now_time_s();
                long now_time = start_time;
                long long num = 0;
                while(now_time < start_time + test_time)
                {
                        int now_index = rand() % arr_num;
                        int j=0,flag=1;
                        struct arr_struct *tmp_arr_struct = arr[now_index];
                        for(j=0;j<str_size;j++)
                        {
                                if (tmp_arr_struct->item[0] != 'M')
                                {
                                        flag = 0;
                                }
                        }
                        if (flag)
                        {
                                num++;
                        }

                        now_time = get_now_time_s();

                }

                int access_time = now_time-start_time;

                double total_access_size = num/1024.0/1024.0/1024.0*str_size;
                double throughput = total_access_size / access_time;
                DBG(L(TEST), "after times: %d, thoughtput is: %.2lf GBps", num, throughput);
                DBG(L(DATA), "%d,%d,%d,%d,%lf,%d", threads, str_size, total_size, num, throughput ,access_time);
                DBG_WRITE();

                for(i=0;i<arr_num;i++)
                {
                        struct arr_struct *tmp_arr_struct = arr[i];
                        free(tmp_arr_struct);
                }
        }
        // exit(0);
}

void throughput_list_init()
{
        in_remote_head = (struct buffer_in_remote *)malloc(sizeof(struct buffer_in_remote));
        in_remote_head->next = NULL;
        in_remote_head->pre = NULL;
        in_remote_head->status = HEAD_FLAG;
        in_remote_head->operate_num = -1;
        in_remote_tail = in_remote_head;

        from_remote_head = (struct buffer_from_remote *)malloc(sizeof(struct buffer_from_remote));
        from_remote_head->next = NULL;
        from_remote_head->pre = NULL;
        from_remote_head->status = HEAD_FLAG;
        from_remote_head->hot_degree = -1;
        from_remote_tail = from_remote_head;
}

void throughtput_test_migrate_thread(struct thread_parm *parm)
{
        sleep(2);
        long long str_size = parm->str_size;
        int test_time = parm->test_time;
        int threads = parm->threads;
        int times = parm->times;
        long long total_size = parm->total_size;

        sleep(1);
        while(T1 == 0 && T2 == 0);
        T1 = 0.0;
        T2 = 90.0;
        
        get_memory_rate();
        long malloc_size = global_mem_info->total * 0.1 * 1024;

        char *malloc_buffer = (char *)malloc(malloc_size);

        memset(malloc_buffer, 'A', malloc_size);
        get_memory_rate();

        struct arr_struct
        {
                char item[str_size];
        };
        
        long arr_num = total_size / str_size;

        int m;
        for(m=0;m<times;m++){
                DBG(L(TEST), "turn: %d", m);
                DBG(L(TEST), "sleep 6s...");
                sleep(6);
                if (in_remote_num > 0) {
                        lru_data_init();
                        throughput_list_init();
                        init_tree();

                        in_remote_num = 0;
                }

                char *arr[arr_num];
                int i;
                for(i=0;i<arr_num;i++)
                {
                        char *tmp_buffer;
                        int page_size = sysconf(_SC_PAGESIZE);
                        if(str_size % page_size) //如果不等于0
                        {
                                str_size = (str_size/page_size + 1) * page_size; // 加一个page页
                        }
                        posix_memalign((void **)&tmp_buffer, page_size, str_size);
                        memset(tmp_buffer, 'M', str_size);

                        arr[i] = tmp_buffer;
                }

                int loop_num = (int)(arr_num*0.4);
                int start_index = arr_num-loop_num;
                for(i=0;i<loop_num;i++)
                {
                        struct lru_data *tmp_lru_data = (struct lru_data *)malloc(sizeof(struct lru_data));
                        tmp_lru_data->buffer = (void *)(arr[start_index+i]);
                        tmp_lru_data->size = str_size;
                        tmp_lru_data->status = PULLED_WAIT;
                        tmp_lru_data->hot_degree = rand();
                        strcpy(tmp_lru_data->file_name, "");
                        rbtree_insert(lru_data_tree, (unsigned long)tmp_lru_data->buffer, (void *)tmp_lru_data);
                        add_lru_data_node_to_tail(tmp_lru_data);
                        lru_data_sorting(tmp_lru_data);
                }

                while (1)
                {
                        if (in_remote_num == loop_num)
                        {
                                break;
                        }
                        sleep_ms(20);
                }
                sleep(2);
                DBG(L(TEST), "start thoughtput migrate testing...");

                long start_time = get_now_time_s();
                long now_time = start_time;
                long long num = 0;
                while(now_time < start_time + test_time)
                {
                        int now_index = rand() % arr_num;
                        int j=0,flag=1;
                        char *tmp_buffer = arr[now_index];
                        for(j=0;j<str_size;j++)
                        {
                                if (tmp_buffer[i] != 'M')
                                {
                                        flag = 0;
                                }
                        }
                        if (flag)
                        {
                                num++;
                        }
                        now_time = get_now_time_s();

                }

                int access_time = now_time-start_time;

                double total_access_size = num/1024.0/1024.0/1024.0*str_size;
                double throughput = total_access_size / access_time;
                DBG(L(TEST), "after times: %d, thoughtput is: %.2lf GBps", num, throughput);
                DBG(L(DATA), "%d,%d,%lld,%d,%lf,%d", threads, str_size, total_size, num, throughput ,access_time);

                DBG_WRITE();

                for(i=0;i<arr_num;i++)
                {
                        char *tmp_buffer = arr[i];
                        char *ch = (char *)malloc(sizeof(char)*MID_STR_LEN);
                        sprintf(ch,"%c", tmp_buffer[0]);
                        sleep(1);
                        free(ch);
                        free(tmp_buffer);
                }
                // DBG(L(TEST), "in remote num: %d", in_remote_num);
        }

        exit(0);
}

void throughtput_test_tossd_thread(long long total_size, long long str_size, int test_time, int threads, int times, float t1, float t2)
{
        sleep(2);

        migrate_init();
        get_memory_rate();

        struct arr_struct
        {
                char item[str_size];
        };
        long arr_num = total_size / str_size;

        int m;
        for(m=0;m<times;m++)
        {
                DBG(L(TEST), "turn %d", m);

                DBG(L(TEST), "sleep 6s..");
                sleep(6);
                
                if (to_ssd_num > 0) {
                        system("rm -rf file/*");

                        lru_data_init();
                        init_tree();
                        init_list();

                        to_ssd_num = 0;
                }

                char *arr[arr_num];
                int i;
                for(i=0;i<arr_num;i++)
                {
                        char *tmp_buffer;
                        int page_size = sysconf(_SC_PAGESIZE);
                        if(str_size % page_size)        //如果不等于0
                        {
                                str_size = (str_size/page_size + 1) * page_size;        // 加一个page页
                        }
                        posix_memalign((void **)&tmp_buffer, page_size, str_size);
                        memset(tmp_buffer, 'M', str_size);

                        arr[i] = tmp_buffer;
                }

                int loop_num = (int)(arr_num*0.4);
                int start_index = arr_num-loop_num;
                for(i=0;i<loop_num;i++)
                {
                        struct lru_data *tmp_lru_data = (struct lru_data *)malloc(sizeof(struct lru_data));
                        tmp_lru_data->buffer = (void *)(arr[start_index+i]);
                        tmp_lru_data->size = str_size;
                        tmp_lru_data->status = PULLED_WAIT;
                        tmp_lru_data->hot_degree = rand();
                        strcpy(tmp_lru_data->file_name, "");
                        rbtree_insert(lru_data_tree, (unsigned long)tmp_lru_data->buffer, (void *)tmp_lru_data);
                        add_lru_data_node_to_tail(tmp_lru_data);
                        lru_data_sorting(tmp_lru_data);
                }

                T1 = t1;
                T2 = t2;
                pthread_t ssd_thread;
                int ssd_thread_code = pthread_create(&ssd_thread, NULL, (void *)&to_ssd, NULL);
                if (ssd_thread_code)
                {
                        DBG(L(INFO), "create to_ssd() pthread failed!");
                }
                else
                {
                        DBG(L(INFO), "create to_ssd() pthread success!");
                }
                while (1)
                {
                        if (to_ssd_num == loop_num)
                        {
                                break;
                        }
                        
                        sleep_ms(50);
                }
                sleep(2);
                DBG(L(TEST), "start thoughtput tossd testing...");

                long start_time = get_now_time_s();
                long now_time = start_time;
                long long num = 0;
                while(now_time < start_time + test_time)
                {
                        int now_index = rand() % arr_num;
                        int j=0,flag=1;
                        char *tmp_buffer = arr[now_index];
                        for(j=0;j<str_size;j++)
                        {
                                if (tmp_buffer[i] != 'M')
                                {
                                        flag = 0;
                                }
                        }
                        if (flag)
                        {
                                num++;
                        }
                        now_time = get_now_time_s();
                }

                int access_time = now_time-start_time;

                double total_access_size = num/1024.0/1024.0/1024.0*str_size;
                double throughput = total_access_size / access_time;
                DBG(L(TEST), "after times: %d, thoughtput is: %.2lf GBps", num, throughput);
                DBG(L(DATA), "%d,%d,%lld,%d,%lf,%d", threads, str_size, total_size, num, throughput ,access_time);

                for(i=0;i<arr_num;i++)
                {
                        char *tmp_buffer = arr[i];
                        char *ch = (char *)malloc(sizeof(char)*MID_STR_LEN);
                        sprintf(ch,"%c", tmp_buffer[0]);
                        sleep_ms(20);
                        free(ch);
                        free(tmp_buffer);
                }

                // DBG(L(TEST), "to_ssd_num: %d", to_ssd_num);
                // system("ls file/* | wc -l");

                // to_ssd_num = 0;
                // system("rm -rf file/*");
                // DBG(L(TEST), "after free, files: ");
                // system("ls file/* | wc -l");

                DBG_WRITE();
        }
}

void t1t2_client() 
{
        sleep(5);
        while(T1 == 0 && T2 == 0);

        T1 = 90.0;
        T2 = 92.0;
}