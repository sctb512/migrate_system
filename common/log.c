#include "log.h"

/**
 * @description: 日志函数，用于保存日志信息，分成两种：DBG_CRITICAL表示信息日志，保存在log.txt中，DBG_DATA表示数据日志，
 *               保存的文件名需要通过全局变量data_file_name指定。使用DBG_INFO、DBG_WARNING、DBG_ERR三种日志等级时不会
 *               输出到文件，并且在debug=0时，也不会有打印信息。
 * @param {int, char *, ...}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void DBG(int level,const char *func, const char *file, const int line, const char *fmt, ...)
{
        //debug不为0，并且level<DBG_CRITICAL时，将信息日志打印输出
        if (level >= LOG_LEVEL && level < TEST)
        {
                time_t t = time(NULL);
                struct tm *tmm = localtime(&t);

                char str1[128];

                va_list arg;
                va_start(arg, fmt);

                char * log_type;
                if(level == INFO)
                {
                        log_type = "INFO";
                }
                if(level == WARNING)
                {
                        log_type = "WARNING";
                }
                if(level == ERR)
                {
                        log_type = "ERR";
                }
                if(level == CRITICAL)
                {
                        log_type = "CRITICAL";
                }

                sprintf(str1, "[%d-%02d-%02d %02d:%02d:%02d][%s][%s:%d] %s: ", tmm->tm_year + 1900, tmm->tm_mon + 1, tmm->tm_mday, tmm->tm_hour, tmm->tm_min, tmm->tm_sec, func, file, line, log_type);
                vsprintf(str1 + strlen(str1), fmt, arg);
                sprintf(str1 + strlen(str1), "%s", "\n");
                va_end(arg);

                #ifdef LOG_WRITE
                int len = strlen(str1);
                if (log_size + len > LOG_LEN)
                {
                        DBG_WRITE_LOG();
                }
                sprintf(log_str + strlen(log_str), "%s", str1);
                log_size = log_size + len;
                #endif

                #ifdef DEBUG
                printf("%s", str1);
                #endif

        }

        if (level == TEST)
        {
                va_list arg;
                va_start(arg, fmt);

                printf("[TEST LOG] ");
                vprintf(fmt, arg);
                printf("\n");
                va_end(arg);
        }

        if (level == DATA)
        {
                char str2[96];

                va_list arg;
                va_start(arg, fmt);

                vsprintf(str2, fmt, arg);
                sprintf(str2 + strlen(str2), "%s", "\n");
                va_end(arg);

                int len = strlen(str2);
                if (data_size + len > LOG_LEN)
                {
                        DBG_WRITE_DATA();
                }

                sprintf(data_str + strlen(data_str), "%s", str2);
                data_size = data_size + len;
        }
}

/**
 * @description: 写日志文件，报错写信息日志和数据日志
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void DBG_WRITE()
{
        if (log_file_name)
        {
                DBG_WRITE_LOG();
        }

        if (data_file_name)
        {
                DBG_WRITE_DATA();
        }
}

/**
 * @description: 当处于内存中buffer中有内容时，以追加的方式写进信息日志文件。
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void DBG_WRITE_LOG()
{
        /*printf("log file name: %p\n", log_file_name);
        printf("data file name: %p\n", data_file_name);
        printf("log file name: %s\n", log_file_name);
        printf("data file name: %s\n", data_file_name);
        printf("write log!!\n");*/
        // #ifdef LOG_DEGBU
        // DBG(L(TEST), "log.c 135 before DBG_WRITE_LOG");
        // #endif

        g_log_fp = fopen(log_file_name, "ab+");
        if (g_log_fp == NULL)
        {
                #ifdef LOG_DEGBU
                DBG(L(TEST), "log.c 142. open log file error!");
                #endif
                printf_log(L(ERR), "open log file error!");
        }
        
        // #ifdef LOG_DEGBU
        // DBG(L(TEST), "log.c 148 after DBG_WRITE_LOG");
        // #endif

        if (log_size > 0)
        {
                fprintf(g_log_fp, "%s", log_str);
                log_str[0] = '\0';
                log_size = 0;
        }

        fflush(g_log_fp);
        fclose(g_log_fp);
}

/**
 * @description: 当处于内存中buffer中有内容时，以追加的方式写进数据日志文件。
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void DBG_WRITE_DATA()
{
        /*printf("log file name: %p\n", log_file_name);
        printf("data file name: %p\n", data_file_name);
        printf("log file name: %s\n", log_file_name);
        printf("data file name: %s\n", data_file_name);
        printf("write data!!\n");
        printf("max size: %d\n", LOG_LEN);
        printf("data size: %d\n", data_size);*/
        // printf("data file name: %p\n", data_file_name);
        // printf("log file name: %s\n", log_file_name);
        // printf("data file name: %s\n", data_file_name);

        g_data_fp = fopen(data_file_name, "ab+");

        // while(!(g_data_fp == NULL)){
        //         printf_log(L(ERR), "open data file error!\n");
        //         g_data_fp = fopen(data_file_name, "ab+");
        // }

        if (g_data_fp == NULL)
        {
                printf_log(L(ERR), "open data file error!\n");
                g_data_fp = fopen(data_file_name, "ab+");
        }

        if (data_size > 0)
        {
                fprintf(g_data_fp, "%s", data_str);
                data_str[0] = '\0';
                data_size = 0;
        }

        fflush(g_data_fp);
        fclose(g_data_fp);
}

/**
 * @description: 初始化函数，判断是否存在log文件夹，不存在则创建，同时通过连接得到日志文件的路径
 * @param {char *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void log_init(char *log_file)
{
        log_size = 0;

        if (access("log", R_OK) != 0)
        {
                if (mkdir("log", 0777))
                {
                        printf_log(L(ERR), "creat floder log failed!\n");
                }
        }

        log_file_name = (char *)malloc(sizeof(char) * LOG_NAME_LEN);

        sprintf(log_file_name, "%s%s", "./log/", log_file);
}

/**
 * @description: 打印日志，传入的参数和DBG一样，只是不会输出到文件，该函数主要用在log模块
 * @param {int, const char *, const char *, const int, const char *, ...}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void printf_log(int level, const char *func, const char *file, const int line, const char *fmt, ...)
{
        time_t t = time(NULL);
        struct tm *tmm = localtime(&t);

        char str1[128];

        va_list arg;
        va_start(arg, fmt);

        char * log_type;
        if(level == INFO)
        {
                log_type = "INFO";
        }
        if(level == WARNING)
        {
                log_type = "WARNING";
        }
        if(level == ERR)
        {
                log_type = "ERR";
        }
        if(level == CRITICAL)
        {
                log_type = "CRITICAL";
        }

        sprintf(str1, "[%d-%02d-%02d %02d:%02d:%02d][%s][%s:%d] %s: ", tmm->tm_year + 1900, tmm->tm_mon + 1, tmm->tm_mday, tmm->tm_hour, tmm->tm_min, tmm->tm_sec, func, file, line, log_type);
        vsprintf(str1 + strlen(str1), fmt, arg);
        sprintf(str1 + strlen(str1), "%s", "\n");
        va_end(arg);

        printf("%s", str1);
}