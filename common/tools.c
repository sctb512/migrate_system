/*
 * @Author: your name
 * @Date: 2020-07-22 09:14:55
 * @LastEditTime: 2020-08-23 10:15:34
 * @LastEditors: xy
 * @Description: In User Settings Edit
 * @FilePath: /test_uf_abin_new_my_rdma_new/common/tools.c
 */
#include "tools.h"

/**
 * 将addr的地址数据写入到指定fd句柄中
 * @使用fwrite进行写入
 * @判断写入的size是否非0，非0则代表成功
*/
void write_file(void *addr, int size, FILE *fd)
{

        size_t write_size = fwrite(addr, size, 1, fd);

        if (write_size)
        {
                DBG(L(INFO),"write file successful!");
        }
        else
        {
                DBG(L(WARNING), "write file failed!");
        }

        fclose(fd);
}

/**
 * TODO 待补充注释
*/
char *read_attr(char *line)
{
        char *token;

        token = strtok(line, "=");
        token = strtok(NULL, "=");

        return token;
}

/**
 * 获取当前的准确时间
*/
char *get_now_time() {
        struct tm *newtime;
        char *tmpbuf = (char *)malloc(sizeof(char)*64);//?此处有声明，但后面没有释放，是否会占用内存
        time_t lt;
        lt=time( NULL );
        newtime=localtime(&lt);
        strftime( tmpbuf, 20, "%F %T", newtime);
        return tmpbuf;
}

/*
        获取当前的秒时间
        return: time_now.tv_sec, time_now.tv_nsec
        打印的话通过：%llu ns 格式打印
*/
long get_now_time_s(){

        struct timeval tv;
        gettimeofday(&tv, NULL);

        return tv.tv_sec;
}

/*
        获取当前的毫秒时间
        return: time_now.tv_sec, time_now.tv_nsec
        打印的话通过：%llu ns 格式打印
*/
long get_now_time_ms(){

        struct timespec time_now={0, 0};
        clock_gettime(CLOCK_REALTIME, &time_now);

        return time_now.tv_sec * 1000 + time_now.tv_nsec / 1000000;
}

/*
        获取当前的微秒时间
        return: time_now.tv_sec, time_now.tv_nsec
        打印的话通过：%llu ns 格式打印
*/
long get_now_time_us(){

        struct timespec time_now={0, 0};
        clock_gettime(CLOCK_REALTIME, &time_now);

        return time_now.tv_sec * 1000000 + time_now.tv_nsec / 1000;
}

/*
        获取当前的毫纳秒时间
        return: time_now.tv_sec, time_now.tv_nsec
        打印的话通过：%llu ns 格式打印
*/
long long get_now_time_ns(){

        struct timespec time_now={0, 0};
        clock_gettime(CLOCK_REALTIME, &time_now);

        return time_now.tv_sec * 1000000000 + time_now.tv_nsec;
}

/**
 * @description: sleep函数，毫秒级别
 * @param {int}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void sleep_ms(int ms)
{
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = ms * 1000;
	select(0, NULL, NULL, NULL, &delay);
}
