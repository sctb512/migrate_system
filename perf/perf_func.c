#include "perf_func.h"

/*
server端
测试主动回迁程序
1先新增数据
2然后malloc
3等待client拉取 一直睡眠等待拉取和主动回迁完成
*/
void test_func_evcit_server()
{
    float st1,st2;
    
    #ifdef DEV
    st1 = 15;
    st2 = 0.43;
    #else
    //测试被动回迁的参数
    // st1 = 1.92;
    // st2 = 0.044;
    //mem_total = 109G
    // st1 = 11.74;//先新增32%的数据
    // st2 = 0.1581;//然后malloc 45%的数据
    st1 = 6.24;
    st2 = 0.157;
    #endif

    //先初始化，然后初始化log日志
    perf_data_init();
    DBG(L(TEST), "test_func_evcit_server will start after 8s...");
    sleep(8);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
        DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    //first step 开始第一个循环，新增lru_data 
    float total = 0;
    while (1)
    {
        get_memory_rate();
        //当新增加数据本身量超过t1, 就退出新增循环
        if (total*100.0 / (global_mem_info->total * 1024.0) > st1)   //
        {
                DBG(L(TEST), "server evict step1 add new_lru_data up to > %lf%%", st1);
                break;
        }

        sleep_ms(20);//每隔50ms产生新的一块数据
        int t_size = new_one_lru_data_solid_size(server_lru_size);
        total += t_size;
    }

    //two step 接着新增malloc，等待client拉取数据
    //并且malloc后内存使用率达到77%
    get_memory_rate();
    DBG(L(TEST), "server evict step2 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * st2 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    memset(release_buffer, 'A', buffer_size);
    get_memory_rate();
    DBG(L(TEST), "server evict step2 after malloc: %lf%%", global_mem_info->mem_rate);

    //setp3
    int is = 50;    //关键变量 睡眠时间控制:使其为被动回迁还是主动回迁
    // sleep(is);
    while(is){
        sleep(1);
        DBG(L(TEST), "server evict wait pull&&evict... %d", is);
        is--;
    }

    sleep(2);
    DBG_WRITE();
    DBG(L(TEST), "test_func_evcit_server done! from_remote_num:%d", from_remote_num);
    // exit(0);
}



/*
client端
测试被动回迁程序    119
1.先拉取足够量
2.然后malloc到75,触发主动回迁

*/
void test_func_evcit_client()
{
    float ct1,ct2;

    #ifdef DEV
    ct1 = 15;   //ct1 < st1 1.792
    ct2 = 0.40;
    #else
    //测试被动回迁时候的配置
    // ct1 = 1.00;
    // ct2 = 0.0412;
    //mem_total=125G

    // ct1 = 2.8;//先拉取数据ct1=15%
    // ct2 = 0.192;//然后malloc ct2=60%的
    ct1 = 0.81;
    ct2 = 0.25;
    #endif

    //先初始化
    perf_data_init();
    DBG(L(TEST), "test_func_evcit_client will start after 8s...");
    sleep(8);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
            DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    //first step1 等待拉取数量足够
    while(1)
    {
        sleep(10);
        if(from_remote_total_size*100.0 / (global_mem_info->total * 1024.0) > ct1)
        {
            DBG(L(TEST), "client step1 done.Pull enough. break.");
            break;
        }
        else{
            DBG(L(TEST), "client evict step1 NOT pull enough.mem_rate:%f%%",global_mem_info->mem_rate);
        }
    }

    //two step 接着新增malloc，等待server端被动回迁数据
    get_memory_rate();
    DBG(L(TEST), "client step2 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * ct2 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    DBG(L(TEST), "client step2 before memset release_buffer:%p", release_buffer);
    memset(release_buffer, 'A', buffer_size);
    DBG(L(TEST), "client step2 after memset release_buffer:%p", release_buffer);
    get_memory_rate();
    DBG(L(TEST), "client step2 after malloc: %lf%%. wait evict!", global_mem_info->mem_rate);

    //three setp
    // DBG(L(TEST), "client step3 sleep(60). wait server evicted.", global_mem_info->mem_rate);
    // sleep(25);//关键控制睡眠变量：使其为被动回迁还是主动回迁
    int cstime = 30;//关键控制睡眠变量：使其为被动回迁还是主动回迁
    while(cstime){
        sleep(1);
        DBG(L(TEST), "client evicting... %d.", cstime);
        cstime--;
    }

    sleep(2);
    DBG_WRITE();
    DBG(L(TEST), "test_func_evcit_client done! from_remote_num:%d", from_remote_num);
    // exit(0);

}



/*
server端
测试被动回迁程序
1先新增数据
2然后malloc
3等待client拉取
4最后释放，启动被动回迁
*/
void test_func_evcited_server()
{
    float st1,st2;
    
    #ifdef DEV
    st1 = 15;
    st2 = 0.43;
    #else
    //测试被动回迁的参数
    // st1 = 1.92;
    // st2 = 0.044;
    //mem_total = 109G
    // st1 = 11.74;//先新增32%的数据
    // st2 = 0.1051;//然后malloc 45%的数据，发生缺页中断后再释放
    st1 = 10.24;
    st2 = 0.0916;
    #endif

    //先初始化，然后初始化log日志
    perf_data_init();
    DBG(L(TEST), "test_func_evcited_server will start after 5s...");
    sleep(8);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
        DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    //first step 开始第一个循环，新增lru_data 
    float total = 0;
    while (1)
    {
        get_memory_rate();
        //当新增加数据本身量超过t1, 就退出新增循环
        if (total*100.0 / (global_mem_info->total * 1024.0) > st1)   //
        {
                DBG(L(TEST), "server step1 add new_lru_data up to > %lf%%", st1);
                break;
        }

        sleep_ms(25);//每隔50ms产生新的一块数据
        int t_size = new_one_lru_data_solid_size(server_lru_size);
        total += t_size;
    }

    //two step 接着新增malloc，等待client拉取数据
    //并且malloc后内存使用率达到77%
    get_memory_rate();
    DBG(L(TEST), "server step2 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * st2 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    memset(release_buffer, 'A', buffer_size);
    get_memory_rate();
    DBG(L(TEST), "server step2 after malloc: %lf%%", global_mem_info->mem_rate);

    //setp3
    int is = 30;    //关键变量 睡眠时间控制:使其为被动回迁还是主动回迁
    // sleep(is);
    while(is){
        sleep(1);
        DBG(L(TEST), "server evicted step3 sleep(%d). mem_rate: %lf%%", is, global_mem_info->mem_rate);
        is--;
    }

    //step3 然后释放malloc后，让server内存掉下T1,这样可以触发主动回迁
    // DBG(L(TEST), "before get_memory_rate()");
    get_memory_rate();
    // DBG(L(TEST), "after get_memory_rate()");

    DBG(L(TEST), "server step4 before free. mem rate: %lf%%", global_mem_info->mem_rate);
    madvise(release_buffer, buffer_size, MADV_DONTNEED);
    free(release_buffer);
    get_memory_rate();
    DBG(L(TEST), "server step4 after free. mem rate: %lf%%", global_mem_info->mem_rate);

    //final step
    sleep(30);
    DBG_WRITE();
    DBG(L(TEST), "test_func_evcited_server done! in_remote_num:%d",in_remote_num);
    // exit(0);
}



/*
client端
测试被动回迁程序
1.先拉取数据量
2.malloc超过T1，使其在测试主动回迁的时候
*/
void test_func_evcited_client()
{
    float ct1,ct2;

    #ifdef DEV
    ct1 = 15;   //ct1 < st1 1.792
    ct2 = 0.40;
    #else
    //测试被动回迁时候的配置
    // ct1 = 1.00;
    // ct2 = 0.0412;
    //mem_total=125G
    // ct1 = 4.8;//先拉取数据ct1=15%
    // ct2 = 0.192;//然后malloc ct2=60%的，等待缺页中断访问完成后再释放
    ct1 = 5.5;
    ct2 = 0.22;
    #endif

    //先初始化
    perf_data_init();
    DBG(L(TEST), "test_func_evcited_client will start after 5s...");
    sleep(8);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
            DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    //first step1 等待拉取数量足够
    while(1)
    {
        sleep(10);
        if(from_remote_total_size*100.0 / (global_mem_info->total * 1024.0) > ct1)
        {
            DBG(L(TEST), "client step1 done.Pull enough. break.");
            break;
        }
        else{
            DBG(L(TEST), "client step1 pull NOT pull enough.mem_rate:%f%%",global_mem_info->mem_rate);
        }
    }

    //two step 接着新增malloc，等待server端被动回迁数据
    get_memory_rate();
    DBG(L(TEST), "client step2 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * ct2 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    memset(release_buffer, 'A', buffer_size);
    get_memory_rate();
    DBG(L(TEST), "client step2 after malloc: %lf%%. wait evict!", global_mem_info->mem_rate);

    //three setp
    // DBG(L(TEST), "client step3 sleep(60). wait server evicted.", global_mem_info->mem_rate);
    // sleep(25);//关键控制睡眠变量：使其为被动回迁还是主动回迁
    int cstime = 10;//关键控制睡眠变量：使其为被动回迁还是主动回迁
    while(cstime){
        sleep(1);
        DBG(L(TEST), "client step3 sleep(%d). mem_rate: %lf%%.wait server evicted", cstime, global_mem_info->mem_rate);
        cstime--;
    }

    //four step 然后释放malloc后，让server内存掉下T1,这样可以触发主动回迁
    // DBG(L(TEST), "client: before get_memory_rate().");
    get_memory_rate();
    // DBG(L(TEST), "client: after get_memory_rate()..");
    DBG(L(TEST), "client step4 before free. mem rate: %lf%%", global_mem_info->mem_rate);
    madvise(release_buffer, buffer_size, MADV_DONTNEED);
    free(release_buffer);
    get_memory_rate();
    DBG(L(TEST), "client step4 after free. mem rate: %lf%%", global_mem_info->mem_rate);

    sleep(10);
    DBG_WRITE();
    DBG(L(TEST), "test_func_evcited_client done! from_remote_num:%d", from_remote_num);
    // exit(0);
}



/*
server端
测试被动回迁程序
1.先新增40%的lru_data
2.然后mallco 15%的buffer超过T1
3.让client拉取15%的数据
4.然后进行访问
*/
void test_func_pagefault_server()
{
    float st1,st2,st3;
    
    #ifdef DEV
    st1 = 42;
    st2 = 0.15;
    st3 = 68;
    #else
    // st1 = 21.38;//先新增数据58%
    // st2 = 0.0734;//    再malloc 20%
    // st3 = 24.03;// 最后判断内存使用率>st3=62% 时触发缺页中断回迁
    st1 = 13.64;
    st2 = 0.104;
    st3 = 71.80;
    #endif

    perf_data_init();
    DBG(L(TEST), "test_func_pagefault_server will start after 8s...");
    sleep(8);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
            DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    //step1
    float total = 0;
    while (1)
    {
        get_memory_rate();
        //当新增加数据本身量超过t1, 就退出新增循环
        if (total*100.0 / (global_mem_info->total * 1024.0) > st1)   //
        {
                DBG(L(TEST), "server step1 add new_lru_data up to > %lf%%", st1);
                break;
        }
        sleep_ms(10);//每隔50ms产生新的一块数据
        int t_size = new_one_lru_data_solid_size(server_lru_size);
        total += t_size;
    }


    //step2 接着新增malloc，等待client拉取数据
    //并且malloc后内存使用率达到70%
    get_memory_rate();
    DBG(L(TEST), "server step2 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * st2 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    memset(release_buffer, 'A', buffer_size);
    get_memory_rate();
    DBG(L(TEST), "server step2 after malloc: %lf%%", global_mem_info->mem_rate);

    //setp3
    int sleept3 = 20;   //8m-20s, 4m-25s
    while(sleept3){
        sleep(1);
        DBG(L(TEST), "server step3 sleep. wait client pull. sleeptime=%d", global_mem_info->mem_rate,sleept3);
        sleept3--;
    }

    //step4
    //等待对方拉数据完成后，再开始测试缺页访问中断
    while (1)
    {
            // DBG(L(TEST), "server while(4) start to test page_fault_fd fun");
            get_memory_rate();
            if(global_mem_info->mem_rate > st3)
            {
                    DBG(L(TEST), "server step4 before access_in_remote. Mem_rate %f%%.access_num:%d",global_mem_info->mem_rate,in_remote_num);
                    access_in_remote(in_remote_num/2);
                    DBG(L(TEST), "server step4 access in remote buffer done, mem rate: %lf%%", global_mem_info->mem_rate);
                    break;
            }
            // else{
            //         DBG(L(TEST), "server step4 ready to page_fault. mem_rate:%f%%",global_mem_info->mem_rate);
            // }
            sleep_ms(50);
            new_one_lru_data_solid_size(server_lru_size);
    }

    // sleep(100);//等待缺页中断访问完成
    //step4 然后释放malloc后，让server内存掉下T1,这样可以触发主动回迁
    get_memory_rate();
    DBG(L(TEST), "server step5 before free. mem rate: %lf%%", global_mem_info->mem_rate);
    madvise(release_buffer, buffer_size, MADV_DONTNEED);
    free(release_buffer);
    get_memory_rate();
    DBG(L(TEST), "server step5 after free. mem rate: %lf%%", global_mem_info->mem_rate);

    sleep(2);
    DBG_WRITE();
    DBG(L(TEST), "test_func_pagefault_server done!");
    // exit(0);
}

/*
client端
测试被动回迁程序
1.先拉取数据量
2.malloc超过T1，使其在测试主动回迁的时候
3.然后新增数据，使其超过72%
4.释放malloc的空间
*/
void test_func_pagefault_client()
{
    float ct0,ct1,ct2;

    #ifdef DEV
    ct0 = 15;
    ct1 = 0.32;
    ct2 = 72;
    #else
    // ct0 = 4.8;//拉取ct0 =15% 的数据就停止
    // ct1 = 0.118;//然后malloc ct1=40%    关键控制变量！！！出问题先调我
    // ct2 = 70.04;//然后自己新增数据至内存使用率的ct2=71%
    ct0 = 3.5;
    ct1 = 0.155;
    ct2 = 21.97;
    #endif

    perf_data_init();
    DBG(L(TEST), "test_func_pagefault_client will start after 8s...");
    sleep(8);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
            DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    sleep(8);//  8M-30s, 4M-55s
    //step1 判断是否已经开始拉取数据
    while(1)
    {
        if(from_remote_num == 0){
            DBG(L(TEST), "client step1 Wait... pull data.");
        }
        else{
            DBG(L(TEST), "client step1 Start pull data.");
            break;
        }
        sleep(1);
    }

    //step2 等待拉取数据完成
    // DBG(L(TEST), "client step2 pulling data.mem_rate:%f%%",global_mem_info->mem_rate);
    // int sleept = 15; //8m-15s 4M-7s
    // while(sleept){
    //     sleep(1);
    //     DBG(L(TEST), "client step2 pulling data. Sleep time:%d. mem_rate:%f%%",sleept,global_mem_info->mem_rate);
    //     sleept--;
    // }
    while(1){
        if(from_remote_total_size*100.0 / (global_mem_info->total * 1024.0) > ct0){
            break;
        }else{
            DBG(L(TEST), "client step2 pulling data ing...");
        }
        sleep(3);
    }

    //step3 接着新增malloc，等待client拉取数据
    //并且malloc后内存使用率达到70%
    get_memory_rate();
    DBG(L(TEST), "client step3 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * ct1 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    memset(release_buffer, 'A', buffer_size);
    get_memory_rate();
    DBG(L(TEST), "client step3 after malloc: %lf%%", global_mem_info->mem_rate);


    //step4 慢慢新增数据，以防再次拉取数据
    while(1)
    {
        if(global_mem_info->mem_rate > ct2){
            break;
        }
        // DBG(L(TEST), "client step4. add new_lru_data ing...");
        new_one_lru_data_solid_size(client_lru_size);
        sleep_ms(200);
    }

    // DBG(L(TEST), "client step4. . ");
    int sleept2 = 50;    //8M-6s, 4M-15s
    while(sleept2){
        sleep(2);//等待缺页中断访问完成
        DBG(L(TEST), "client step4. pagefault ing.sleeptime:%d", sleept2);
        sleept2-=2;
    }
    
    
    //step5 然后释放malloc后，让server内存掉下T1,这样可以触发主动回迁
    // get_memory_rate();
    // DBG(L(TEST), "client step5 before free. mem rate: %lf%%", global_mem_info->mem_rate);
    // madvise(release_buffer, buffer_size, MADV_DONTNEED);
    // free(release_buffer);
    // get_memory_rate();
    // DBG(L(TEST), "client step5 after free. mem rate: %lf%%", global_mem_info->mem_rate);

    sleep(2);
    DBG_WRITE();
    DBG(L(TEST), "test_func_pagefault_client done!");
    // exit(0);
}



/*
server端
测试to_ssd()函数
1.新增malloc 39%的buffer
2.新增数据直到85%
3.睡眠一段时间
4.最后释放39%的malloc
*/
void test_func_tossd_server()
{
    float st1,st2;
    
    #ifdef DEV
    st1 = 85;
    st2 = 0.39;
    #else
    st1 = 22.02;//新建数据总量/内存总量>st1
    st2 = 0.1101;//malloc数据量/内存总量 = st2
    #endif

    //先初始化，然后初始化log日志
    perf_data_init();
    DBG(L(TEST), "test_func_tossd_server will start after 5s...");
    sleep(5);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
        DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    //step2 开始第一个循环，新增lru_data
    float total = 0;
    while (1)
    {
        get_memory_rate();
        //当新增加数据本身量超过t1, 就退出新增循环
        if (total*100.0 / (global_mem_info->total * 1024.0) > st1)   //
        {
                DBG(L(TEST), "server step1 add new_lru_data up to > %lf%%", st1);
                break;
        }
        sleep_ms(10);//每隔50ms产生新的一块数据
        int t_size = new_one_lru_data_solid_size(server_lru_size);
        total += t_size;
    }

    //step1 接着新增malloc，等待client拉取数据
    //并且malloc后内存使用率达到77%
    get_memory_rate();
    DBG(L(TEST), "server step2 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * st2 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    memset(release_buffer, 'A', buffer_size);
    get_memory_rate();
    DBG(L(TEST), "server step2 after malloc: %lf%%", global_mem_info->mem_rate);
    // sleep(2);

    //setp3 
    // DBG(L(TEST), "server step3 sleep(100). wait client pull.", global_mem_info->mem_rate);
    // sleep(100);//睡眠100秒，等待server端执行to_ssd()
    int tst = 10;
    while(tst){
        sleep(1);//等待缺页中断访问完成
        DBG(L(TEST), "server step3 sleep(%d). wait to_ssd() doing...", tst);
        tst--;
    }

    //step4 然后释放malloc后，让server内存掉下T1,这样可以触发主动回迁
    get_memory_rate();
    DBG(L(TEST), "server step4 before free. mem rate: %lf%%", global_mem_info->mem_rate);
    madvise(release_buffer, buffer_size, MADV_DONTNEED);
    free(release_buffer);
    get_memory_rate();
    DBG(L(TEST), "server step4 after free. mem rate: %lf%%", global_mem_info->mem_rate);

    //final step
    DBG_WRITE();
    DBG(L(TEST), "server test to_ssd() done! in_remote_num:%d",in_remote_num);
    sleep(5);
    // exit(0);
}




/*
client端
测试to_ssd()函数
1.新增malloc 39%的buffer
2.新增数据直到85%
3.睡眠一段时间
4.最后释放39%的malloc
*/
void test_func_tossd_client()
{
    float st1,st2;
    #ifdef DEV
    st1 = 0.47;
    st2 = 85;
    #else
    st1 = 19.2;//新建数据总量/内存总量>st1
    st2 = 0.096;//malloc数据量/内存总量 = st2
    #endif

    //先初始化，然后初始化log日志
    perf_data_init();
    DBG(L(TEST), "test_func_tossd_client will start after 5s...");
    sleep(5);
    while(!global_mem_info);
    if (log_file_name != NULL && access(log_file_name, R_OK) == 0)
    {
        DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "lru_size", "func", "start_time_us", "end_time_us", "elapsed_time", "time");
    }

    //step2 开始第一个循环，新增lru_data
    float total = 0;
    while (1)
    {
        get_memory_rate();
        //当新增加数据本身量超过t1, 就退出新增循环
        if (total*100.0 / (global_mem_info->total * 1024.0) > st1)   //
        {
                DBG(L(TEST), "client step1 add new_lru_data up to > %lf%%", st1);
                break;
        }
        sleep_ms(10);//每隔50ms产生新的一块数据
        int t_size = new_one_lru_data_solid_size(server_lru_size);
        total += t_size;
    }

    //step1 接着新增malloc，等待client拉取数据
    //并且malloc后内存使用率达到77%
    get_memory_rate();
    DBG(L(TEST), "client step2 before malloc: %lf%%", global_mem_info->mem_rate);
    long buffer_size = global_mem_info->total * st2 * 1024;
    char *release_buffer = (char *)malloc(buffer_size);
    memset(release_buffer, 'A', buffer_size);
    get_memory_rate();
    DBG(L(TEST), "client step2 after malloc: %lf%%", global_mem_info->mem_rate);

    //setp3 
    // DBG(L(TEST), "client step3 sleep(100). wait client pull.", global_mem_info->mem_rate);
    // sleep(100);//睡眠100秒，等待server端执行to_ssd()
    int cst = 8;
    while(cst){
        sleep(1);//等待缺页中断访问完成
        DBG(L(TEST), "client step3 sleep(%d). wait to_ssd() doing...", cst);
        cst--;
    }

    //step4 然后释放malloc后，让server内存掉下T1,这样可以触发主动回迁
    get_memory_rate();
    DBG(L(TEST), "client step4 before free. mem rate: %lf%%", global_mem_info->mem_rate);
    madvise(release_buffer, buffer_size, MADV_DONTNEED);
    free(release_buffer);
    get_memory_rate();
    DBG(L(TEST), "client step4 after free. mem rate: %lf%%", global_mem_info->mem_rate);

    //final step
    DBG_WRITE();
    DBG(L(TEST), "client test to_ssd() done! in_remote_num:%d",in_remote_num);
    sleep(5);
    // exit(0);
}