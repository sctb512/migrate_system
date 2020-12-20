#include "perf_comm.h"
#include "migrate.h"

struct thread_parm
{
        long long total_size;
        long long str_size;
        int test_time;
        int threads;
        int times;
};


void throughtput_test_base(long long total_size, long long str_size, int test_time, int threads, int times);
void throughtput_test_migrate_thread(struct thread_parm *parm);
void throughtput_test_tossd_thread(long long total_size, long long str_size, int test_time, int threads, int times, float t1, float t2);
void t1t2_client();
void throughput_list_init();