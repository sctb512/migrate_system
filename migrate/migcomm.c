
#include "migcomm.h"

/**
 * @description: 迁移程序相关全局变量的初始化
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void migcomm_init()
{
        IPS_FILE = "ips.cfg";

        T1 = 0.3 * 100.0;
        T2 = 0.5 * 100.0;

        cycle = 5; // time of every turn

        path = "file/";

        next_lru_data = NULL;

        in_remote_num = 0;
        from_remote_num = 0;
        to_ssd_num = 0;
        from_remote_total_size = 0;

        global_mem_info = NULL;
        // base_sys_page = sysconf(_SC_PAGESIZE);

        #ifdef EVICT_DEBUG
        debug_num = 0;
        #endif
}

/**
 * @description: 获取远程机器的内存使用率
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void get_remote_memory()
{
        struct rdma_context ctx;
        struct request_message req_msg;

        req_msg.msg_type = MEM_INFO;
        ctx.request_msg = &req_msg;

        struct ip *next_ip = ips;
        while (next_ip->next != NULL)
        {
                next_ip = next_ip->next;
                rc_client_loop(next_ip->ip, DEFAULT_PORT, &ctx);
        }
}

/**
 * @description: 获取下一个被拉取的数据
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void get_next_pulled_data()
{
        struct lru_data *tmp_lru_data = lru_data_head;
        struct lru_data *pulled_lru_data = NULL;

        if (tmp_lru_data->next != NULL)
        {

                while (tmp_lru_data->next != NULL)
                {

                        tmp_lru_data = tmp_lru_data->next;

                        if (tmp_lru_data->status == PULLED_WAIT)
                        {
                                pulled_lru_data = tmp_lru_data;
                                next_lru_data = tmp_lru_data;
                                // global_mem_info->need_pulled = 1;
                                break;
                        }
                }

                if (pulled_lru_data == NULL)
                {
                        next_lru_data = NULL;
                        // global_mem_info->need_pulled = 0;
                }
        }
        else
        {
                next_lru_data = NULL;
                // global_mem_info->need_pulled = 0;
        }
}

/**
 * @description: 通过rdma连接获取本机的ip地址
 * @param {struct rdma_cm_id *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void get_ip(struct rdma_cm_id *id)
{
        if (!strcmp(global_mem_info->ip, ""))
        {
                struct sockaddr_in *sock = (struct sockaddr_in *)&id->route.addr.src_addr;
                struct in_addr in = sock->sin_addr;
                char str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &in, str, sizeof(str));
                strcpy(global_mem_info->ip, str);
        }
}

/**
 * @description: 从ips.cfg文件中读取配置信息
 * @param {void}
 * @return {void}}
 * @author: abin&xy
 * @last_editor: abin
 */
int read_cfgs()
{
        FILE *ips_file = fopen(IPS_FILE, "r");
        if (ips_file == NULL)
        {
                DBG(L(CRITICAL),"open file %s failed!", IPS_FILE);
                return 0;
        }

        char line[MAX_IP_ADDR];
        int is_ip = 0;
        ips = malloc(sizeof(struct ip));
        struct ip *next_ip = ips;
        while (!feof(ips_file))
        {
                fgets(line, MAX_IP_ADDR, ips_file);

                if (strstr(line, "done"))
                {
                        break;
                }

                if (strstr(line, "#") || !strcmp(line, "\n"))
                {
                        continue;
                }

                if (strstr(line, "end"))
                {
                        is_ip = 0;
                        continue;
                }

                if (strstr(line, "lip"))
                {
                        char *tmp = malloc(sizeof(char)*MAX_IP_ADDR);
                        line[strlen(line) - 1] = '\0';
                        strcpy(tmp, line);
                        char *res = read_attr(tmp);

                        strcpy(global_mem_info->ip, res);
                        DBG(L(INFO), "read local_ip: %s", global_mem_info->ip);
                        
                        continue;
                }

                if (strstr(line, "t1"))
                {
                        char *tmp = malloc(sizeof(char)*MAX_IP_ADDR);
                        strcpy(tmp, line);
                        char *res = read_attr(tmp);

                        T1 = atof(res) * 100.0;

                        DBG(L(INFO), "read T1: %.2f", T1 / 100.0);
                        continue;
                }

                if (strstr(line, "t2"))
                {
                        char *tmp = malloc(sizeof(char)*MAX_IP_ADDR);
                        strcpy(tmp, line);
                        char *res = read_attr(tmp);

                        T2 = atof(res) * 100.0;

                        DBG(L(INFO), "read T2: %.2f", T2 / 100.0);
                        continue;
                }

                if (strstr(line, "cycle"))
                {
                        char *tmp = malloc(sizeof(char)*MAX_IP_ADDR);
                        strcpy(tmp, line);
                        char *res = read_attr(tmp);

                        cycle = atoi(res);

                        DBG(L(INFO), "read cycle: %d", cycle);
                        continue;
                }

                if (strstr(line, "ips"))
                {
                        is_ip = 1;
                        continue;
                }

                if (is_ip)
                {
                        struct ip *tmp_ip = malloc(sizeof(struct ip));
                        tmp_ip->next = NULL;
                        line[strlen(line) - 1] = '\0';
                        strcpy(tmp_ip->ip, line);
                        next_ip->next = tmp_ip;
                        next_ip = next_ip->next;
                }
        }
        fclose(ips_file);

        return 1;
}

/**
 * @description: 通过/proc/meminfo文件获取当前机器的内存使用率，
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void get_memory_rate()
{
        float temp;

        struct meminfo *mem_info = (struct meminfo *)malloc(sizeof(struct meminfo));
        if (mem_info == NULL)
        {
                DBG(L(CRITICAL), "memory allocate failed");
                exit(1);
        }
        FILE *f1 = fopen("/proc/meminfo", "r");

        fscanf(f1, "%s\t%d\t%s", mem_info->mem_str, &mem_info->mem_total, mem_info->mem_size);
        fscanf(f1, "%s\t%d\t%s", mem_info->mem_str, &mem_info->mem_free, mem_info->mem_size);

        fscanf(f1, "%s\t%d\t%s", mem_info->mem_str, &mem_info->mem_available, mem_info->mem_size);

        fscanf(f1, "%s\t%d\t%s", mem_info->mem_str, &mem_info->mem_buffer, mem_info->mem_size);
        fscanf(f1, "%s\t%d\t%s", mem_info->mem_str, &mem_info->mem_cached, mem_info->mem_size);

        fclose(f1);

        float total_without_cache = mem_info->mem_total - mem_info->mem_cached - mem_info->mem_buffer;
        float used_without_cache = total_without_cache - mem_info->mem_free;

        temp = (used_without_cache * 100.0) / mem_info->mem_total;

        global_mem_info->total = mem_info->mem_total;
        global_mem_info->used = used_without_cache;
        global_mem_info->mem_rate = temp;

        free(mem_info);
}

/**
 * @description: 打印当前运行状态下，in_remote链表和from_remote链表中的信息
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void print_curr_info()
{
        DBG(L(INFO),"- buffer in remote num: %d", in_remote_num);

        struct buffer_in_remote *tmp_in_remote = in_remote_head;
        while (tmp_in_remote->next != NULL)
        {
                tmp_in_remote = tmp_in_remote->next;
                DBG(L(INFO), "+ ip: %s", tmp_in_remote->ip);
                DBG(L(INFO), "+ local address: 0x%lx", tmp_in_remote->local_addr);
                DBG(L(INFO), "+ size: %d", tmp_in_remote->size);
                DBG(L(INFO), "+ remote address: 0x%lx", tmp_in_remote->remote_addr);
                DBG(L(INFO), "+ rkey: 0x%lx", tmp_in_remote->rkey);
        }

        DBG(L(INFO), "- buffer from remote num: %d", from_remote_num);

        struct buffer_from_remote *tmp_from_remote = from_remote_head;
        while (tmp_from_remote->next != NULL)
        {
                tmp_from_remote = tmp_from_remote->next;
                DBG(L(INFO), "+ ip: %s", tmp_from_remote->ip);
                DBG(L(INFO), "+ local address: 0x%lx", tmp_from_remote->local_addr);
                DBG(L(INFO), "+ size: %d", tmp_from_remote->size);
                DBG(L(INFO), "+ origin address: 0x%lx", tmp_from_remote->origin_addr);
        }
}

/**
 * @description: 打印struct buffer_from_remote结构体的一个节点信息
 * @param {struct buffer_from_remote *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void printf_from_remote_head_node(struct buffer_from_remote *tmp_from_remote)
{
        if (tmp_from_remote != NULL)
        {
                DBG(L(INFO), "++ ip: %s", tmp_from_remote->ip);
                DBG(L(INFO), "++ local address: 0x%lx", tmp_from_remote->local_addr);
                DBG(L(INFO), "++ size: %d", tmp_from_remote->size);
                DBG(L(INFO), "++ origin address: 0x%lx", tmp_from_remote->origin_addr);
        }
        else
        {
                DBG(L(INFO), "this node is NULL!");
        }
        DBG(L(INFO), "");
}

/**
 * @description: 打印一个struct lru_data结构体中的信息
 * @param {struct lru_data *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void printf_lru_data(struct lru_data *tmp_lru_data)
{
        if (tmp_lru_data != NULL)
        {
                DBG(L(INFO), "tmp_lru_data->file_name = %s", tmp_lru_data->file_name);
                DBG(L(INFO), "tmp_lru_data->type = %hhu", tmp_lru_data->type);
                DBG(L(INFO), "tmp_lru_data->size = %ld", tmp_lru_data->size);
                DBG(L(INFO), "tmp_lru_data->buffer = 0x%lx", (uintptr_t)tmp_lru_data->buffer);
                DBG(L(INFO), "tmp_lru_data->hot_degree = %d", tmp_lru_data->hot_degree);
                DBG(L(INFO), "tmp_lru_data->next = %p ", tmp_lru_data->next);
        }
        else
        {
                DBG(L(INFO), "the lru_data_node is NULL!!!");
        }
}

/**
 * @description: 打印保存远程机器内存使用率的链表
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void print_remote_memory()
{
        struct memory_from_remote *tmp_remote_memory = remote_memory_head;

        // DBG(L(TEST), "tmp_remote_memory->next: %p", tmp_remote_memory->next);
        if (tmp_remote_memory->next != NULL)
        {
                DBG(L(TEST), "- memory rate from remote as follows:");
        }
        while (tmp_remote_memory->next != NULL)
        {
                tmp_remote_memory = tmp_remote_memory->next;
                DBG(L(TEST), "+ ip: %s", tmp_remote_memory->ip);
                DBG(L(TEST), "+ memory_rate: %lf%%", tmp_remote_memory->mem_rate);
                DBG(L(TEST), "+ pulled num: %d", tmp_remote_memory->pulled_num);
                DBG(L(TEST), "+ need pulled: %s", tmp_remote_memory->need_pulled ? "yes" : "no");
        }
}