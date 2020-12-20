#include "perf_comm.h"

/**
 * @description: 将数据日志文件名拼为路径，数据长度设置为0
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: xy
 */
void perf_data_init()
{
        data_size = 0;

        data_file_name = (char *)malloc(sizeof(char) * LOG_NAME_LEN);
        sprintf(data_file_name, "%s%s", "./log/", data_file);
}


/**
 * @description: 性能测试初始化，主要是一些全局变量
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: xy
 */
// void perf_init()
// {
        // lru_size_total = 0;
        // test_flag = 0;

        // lru_data_init();
// }

/**
 * @description: 初始化global_mem_info全局变量
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void global_memory_init()
{
        global_mem_info = (struct memory_info *)malloc(sizeof(struct memory_info));
        global_mem_info->ip = (char *)malloc(sizeof(char) * MAX_IP_ADDR);
        strcpy(global_mem_info->ip, "");
}

/**
 * 创建一个新的lru数据块
 * @数据块的尺寸是系统基准页的倍数
 * @数据块的大小范围是：min_lru_size ~ server_lru_size
 * @初始化lru_data_node节点的基本变量信息
 * @将新节点加入到红黑树，双向链表中并排序
*/
int new_one_lru_data()
{
        // int min_lru_size = 1024 * 1024 *1;
        struct lru_data *tmp_lru_data = (struct lru_data *)malloc(sizeof(struct lru_data));
        char *buffer;
        int page_size = sysconf(_SC_PAGESIZE);
        int rand_size = rand() % (max_lru_size - min_lru_size) + min_lru_size;
        int size = (rand_size / page_size) * page_size;
        if (rand_size % page_size)
        {
                size += page_size;
        }

        posix_memalign((void **)&buffer, sysconf(_SC_PAGESIZE), size);

        int content = rand() % 26 + 65;
        // char curr_content = (buffer_content - '0') % ('9' - '0') + '0';
        // buffer_content += 1;
        memset(buffer, content, size);

        tmp_lru_data->buffer = buffer;
        tmp_lru_data->size = size;
        tmp_lru_data->status = PULLED_WAIT;
        tmp_lru_data->hot_degree = rand();
        strcpy(tmp_lru_data->file_name, "");

        rbtree_insert(lru_data_tree, (unsigned long)tmp_lru_data->buffer, (void *)tmp_lru_data);
        add_lru_data_node_to_tail(tmp_lru_data);

        lru_data_sorting(tmp_lru_data);
        return size;
}



/**
 * @description: 创建一个新的lru数据块
 * 初始化lru_data_node节点的基本变量信息
 * 数据块的尺寸是固定的值
 * 因为是固定值，所以肯定是基准页的倍数
 * @param {int} size
 * @return {type}
 * @author: abin&xy
 * @last_editor: xy
 */
int new_one_lru_data_solid_size(int size)
{
        struct lru_data *tmp_lru_data = (struct lru_data *)malloc(sizeof(struct lru_data));
        char *buffer;
        int page_size = sysconf(_SC_PAGESIZE);

        if(size % page_size) //如果不等于0
        {
                size = (size/page_size + 1) * page_size; // 加一个page页
        }

        posix_memalign((void **)&buffer, sysconf(_SC_PAGESIZE), size);
        int content = rand() % 26 + 65;
        // char curr_content = (buffer_content - '0') % ('9' - '0') + '0';
        // buffer_content += 1;
        memset(buffer, content, size);

        tmp_lru_data->buffer = buffer;
        tmp_lru_data->size = size;
        tmp_lru_data->status = PULLED_WAIT;
        tmp_lru_data->hot_degree = rand();
        strcpy(tmp_lru_data->file_name, "");

        rbtree_insert(lru_data_tree, (unsigned long)tmp_lru_data->buffer, (void *)tmp_lru_data);
        add_lru_data_node_to_tail(tmp_lru_data);

        lru_data_sorting(tmp_lru_data);
        return size;
}