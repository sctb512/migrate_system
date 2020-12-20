
#include "migrate.h"

#ifdef PERF_TEST
#include "throughput.h"
#endif


/**
 * 系统主函数入口
*/
int main(int argc, char **argv)
{
        #ifdef PERF_TEST
        if (argc == 2 && !strcmp(argv[1], "tb"))
        {
                log_init("log.txt");
                data_file = "throughtput_test_base_1G.csv";   //日志文件名初始化
                perf_data_init();       //给日志文件名赋值
                if ((access(data_file_name,F_OK)) == -1)
                {
                        DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "threads", "single_size(B)", "total_size(B)", "access_times","throughput(GBps)" , "elapsed_time");
                }

                long long total_size = 1024LL * 1024 * 1024 * 10;
                long long str_size = 1024LL * 1024 * 1024;
                int test_time = 20;     //测试时间，单位：s
                int threads = 1;
                int times = 5;          //测试次数

                throughtput_test_base(total_size, str_size, test_time, threads, times);

                exit(0);
        }
        #endif

        #ifdef PERF_TEST
        if (argc == 2 && !strcmp(argv[1], "tt"))
        {
                log_init("log.txt");
                
                data_file = "throughtput_test_tossd_1G.csv";//日志文件名初始化
                perf_data_init();//给日志文件名赋值
                if ((access(data_file_name,F_OK)) == -1)
                {
                        DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "threads", "single_size(B)", "total_size(B)", "access_times","throughput(GBps)" , "elapsed_time");
                }
                
                float t1 = 0.0;
                float t2 = 0.0;

                long long total_size = 1024LL * 1024 * 1024 * 10;
                long long str_size = 1024LL * 1024 * 1024;
                int test_time = 20;     //测试时间，单位：s
                int threads = 1;
                int times = 5;          //测试次数

                throughtput_test_tossd_thread(total_size, str_size, test_time, threads, times, t1, t2);

                exit(0);
        }
        #endif

        #ifdef PERF_TEST
        if (argc == 2 && !strcmp(argv[1], "tm"))
        {
                data_file = "throughtput_test_migrate_1G.csv";//日志文件名初始化
                perf_data_init();//给日志文件名赋值
                if ((access(data_file_name,F_OK)) == -1)
                {
                        DBG(L(DATA), "%s,%s,%s,%s,%s,%s", "threads", "single_size(B)", "total_size(B)", "access_times","throughput(GBps)" , "elapsed_time");
                }
                
                struct thread_parm parm;
                parm.total_size = 1024LL * 1024 * 1024 * 10;
                parm.str_size = 1024LL * 1024 * 1024;
                parm.test_time = 20;    //测试时间，单位：s
                parm.threads = 1;
                parm.times = 5;         //测试次数
                pthread_t mig_thread;
                int mig_thread_code = pthread_create(&mig_thread, NULL, (void *)&throughtput_test_migrate_thread, (void *)&parm);
                if (mig_thread_code)
                {
                        DBG(L(INFO), "create throughtput_test_migrate_thread() pthread failed!");
                }
                else
                {
                        DBG(L(INFO), "create throughtput_test_migrate_thread() pthread success!");
                }
        }
        #endif

        #ifdef PERF_TEST
        if (argc == 2 && !strcmp(argv[1], "tc"))
        {
                pthread_t t_thread;
                int t_thread_code = pthread_create(&t_thread, NULL, (void *)&t1t2_client, NULL);
                if (t_thread_code)
                {
                        DBG(L(INFO), "create t1t2_client() pthread failed!");
                }
                else
                {
                        DBG(L(INFO), "create t1t2_client() pthread success!");
                }
        }
        #endif

        DBG(L(TEST), "main runinng...");
        run();

        return 0;
}