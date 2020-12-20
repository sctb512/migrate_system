#include "list.h"

/**
 * 根据buffer_in_remote双向链表当前节点curr_node的热度值进行后移或前移，使得链表有序
 * @参数是传入当前节点的地址
 * @判断是否需要往后移动
 *      先根据条件向后循环直到找到合适的插入点
 *      然后将节点进行插入（先将节点脱离链表，然后插入到合适位置，最后更改新位置之前节点的前后指向）
 * @否则，判断是否需要往前移动
 *      先根据条件向前循环直到找到合适的插入点
 *      然后将节点进行插入（先将节点脱离链表，然后插入到合适位置，最后更改新位置之前节点的前后指向）
 * @若都不满足，则表示当前节点位置正好，不需移动，直接返回
*/
void buffer_in_remote_sorting(struct buffer_in_remote *curr_node)
{
        struct buffer_in_remote *tmp_node = curr_node;
        if (tmp_node->next != NULL && curr_node->operate_num > tmp_node->next->operate_num)
        { //后移判断
                while (tmp_node->next != NULL && curr_node->operate_num > tmp_node->next->operate_num)
                        tmp_node = tmp_node->next;
                curr_node->pre->next = curr_node->next;
                curr_node->next->pre = curr_node->pre;

                curr_node->next = tmp_node->next;
                curr_node->pre = tmp_node;
                //最后，判断插入位置tmp节点是否为尾结点
                if (tmp_node == in_remote_tail)
                        in_remote_tail = curr_node;
                else
                        tmp_node->next->pre = curr_node;
                tmp_node->next = curr_node;
        }
        else if (curr_node->operate_num < tmp_node->pre->operate_num)
        { //前移判断，头结点不可能为空，且tmp节点不可能等于头结点(头结点的热度值为-1)
                while (curr_node->operate_num < tmp_node->pre->operate_num)
                        tmp_node = tmp_node->pre;
                curr_node->pre->next = curr_node->next;
                if (curr_node != in_remote_tail)
                        curr_node->next->pre = curr_node->pre;
                else
                        in_remote_tail = curr_node->pre;

                curr_node->pre = tmp_node->pre;
                curr_node->next = tmp_node;

                tmp_node->pre->next = curr_node;
                tmp_node->pre = curr_node;
        }
        else
                return;
}

/**
 * 根据buffer_from_remote双向链表当前节点curr_node的热度值进行后移或前移，使得链表有序
 * @参数是传入当前节点的地址
 * @判断是否需要往后移动
 *      先根据条件向后循环直到找到合适的插入点
 *      然后将节点进行插入（先将节点脱离链表，然后插入到合适位置，最后更改新位置之前节点的前后指向）
 * @否则，判断是否需要往前移动
 *      先根据条件向前循环直到找到合适的插入点
 *      然后将节点进行插入（先将节点脱离链表，然后插入到合适位置，最后更改新位置之前节点的前后指向）
 * @若都不满足，则表示当前节点位置正好，不需移动，直接返回
*/
void buffer_from_remote_sorting(struct buffer_from_remote *curr_node)
{
        struct buffer_from_remote *tmp_node = curr_node;
        if (tmp_node->next != NULL && curr_node->hot_degree > tmp_node->next->hot_degree)
        {
                while (tmp_node->next != NULL && curr_node->hot_degree > tmp_node->next->hot_degree)
                        tmp_node = tmp_node->next;
                curr_node->pre->next = curr_node->next;
                curr_node->next->pre = curr_node->pre;

                curr_node->next = tmp_node->next;
                curr_node->pre = tmp_node;

                if (tmp_node == from_remote_tail)
                        from_remote_tail = curr_node;
                else
                        tmp_node->next->pre = curr_node;
                tmp_node->next = curr_node;
        }
        else if (curr_node->hot_degree < tmp_node->pre->hot_degree)
        {
                while (curr_node->hot_degree < tmp_node->pre->hot_degree)
                        tmp_node = tmp_node->pre;

                curr_node->pre->next = curr_node->next;
                if (curr_node != from_remote_tail)
                        curr_node->next->pre = curr_node->pre;
                else
                        from_remote_tail = curr_node->pre;

                curr_node->pre = tmp_node->pre;
                curr_node->next = tmp_node;

                tmp_node->pre->next = curr_node;
                tmp_node->pre = curr_node;
        }
        else
                return;
}

/**
 * 根据lru_data双向链表当前节点curr_node的热度值进行后移或前移，使得链表有序
 * @参数是传入当前节点的地址
 * @判断是否需要往后移动
 *      先根据条件向后循环直到找到合适的插入点
 *      然后将节点进行插入（先将节点脱离链表，然后插入到合适位置，最后更改新位置之前节点的前后指向）
 * @否则，判断是否需要往前移动
 *      先根据条件向前循环直到找到合适的插入点
 *      然后将节点进行插入（先将节点脱离链表，然后插入到合适位置，最后更改新位置之前节点的前后指向）
 * @若都不满足，则表示当前节点位置正好，不需移动，直接返回
*/
void lru_data_sorting(struct lru_data *curr_node)
{
        struct lru_data *tmp_node = curr_node;

        if (tmp_node->next != NULL && curr_node->hot_degree > tmp_node->next->hot_degree)
        {
                while (tmp_node->next != NULL && curr_node->hot_degree > tmp_node->next->hot_degree)
                        tmp_node = tmp_node->next;

                curr_node->pre->next = curr_node->next;
                curr_node->next->pre = curr_node->pre;

                curr_node->next = tmp_node->next;
                curr_node->pre = tmp_node;

                if (tmp_node == lru_data_tail)
                        lru_data_tail = curr_node;
                else
                        tmp_node->next->pre = curr_node;
                tmp_node->next = curr_node;
        }
        else if (curr_node->hot_degree < tmp_node->pre->hot_degree)
        {
                while (curr_node->hot_degree < tmp_node->pre->hot_degree)
                        tmp_node = tmp_node->pre;
                curr_node->pre->next = curr_node->next;
                if (curr_node != lru_data_tail)
                        curr_node->next->pre = curr_node->pre;
                else
                        lru_data_tail = curr_node->pre;

                curr_node->pre = tmp_node->pre;
                curr_node->next = tmp_node;

                tmp_node->pre->next = curr_node;
                tmp_node->pre = curr_node;
        }
        else
                return;
}

/**
 * 向buffer_in_remote双向链表尾部增加新节点new_node
*/
void add_in_remote_node_to_tail(struct buffer_in_remote *new_node)
{
        new_node->next = NULL;
        new_node->pre = in_remote_tail;
        in_remote_tail->next = new_node;
        in_remote_tail = new_node;
}

/**
 * 向buffer_from_remote双向链表尾部增加新节点new_node
*/
void add_from_remote_node_to_tail(struct buffer_from_remote *new_node)
{
        new_node->next = NULL;
        new_node->pre = from_remote_tail;
        from_remote_tail->next = new_node;
        from_remote_tail = new_node;
}

/**
 * 向lru_data双向链表尾部增加新节点new_node
*/
void add_lru_data_node_to_tail(struct lru_data *new_node)
{
        new_node->next = NULL;
        new_node->pre = lru_data_tail;
        lru_data_tail->next = new_node;
        lru_data_tail = new_node;
}

/**
 * 在buffer_from_remote双向链表中删除del_node节点，但并不释放该节点
 * @首先判断是否为尾结点
 * @否则，直接从链表中删除
*/
void del_from_remote_node(struct buffer_from_remote *del_node)
{
        if (del_node == from_remote_tail)
        {
                del_node->pre->next = NULL;
                from_remote_tail = del_node->pre;
        }
        else
        {
                del_node->pre->next = del_node->next;
                del_node->next->pre = del_node->pre;
        }
}

/**
 * 在buffer_in_remote双向链表中删除del_node节点，但并不释放该节点
 * @首先判断是否为尾结点
 * @否则，直接从链表中删除
*/
void del_in_remote_node(struct buffer_in_remote *del_node)
{
        // DBG(L(TEST), "+ delte node: \033[35m%p\033[0m, in_remote_tail: %p", del_node, in_remote_tail);

        // DBG(L(TEST), "+ local addr: \033[36m%p\033[0m", del_node->local_addr);
        // DBG(L(TEST), "+ remote addr: %p", del_node->remote_addr);
        // DBG(L(TEST), "+ pre: %p", del_node->pre);
        // DBG(L(TEST), "+ next: %p", del_node->next);

        if (del_node == in_remote_tail)
        {
                del_node->pre->next = NULL;
                in_remote_tail = del_node->pre;
        }
        else
        {
                del_node->pre->next = del_node->next;

                // DBG(L(TEST), "del_node->pre->next: %p", del_node->pre->next);
                // DBG(L(TEST), "del_node->next: %p", del_node->next);
                del_node->next->pre = del_node->pre;
        }
        // DBG(L(TEST), "del_in_remote_node ok!");
}

/**
 * 在lru_data双向链表中删除del_node节点，但并不释放该节点
 * @首先判断是否为尾结点
 * @否则，直接从链表中删除
*/
void del_lru_data_node(struct lru_data *del_node)
{
        if (del_node == lru_data_tail)
        {
                del_node->pre->next = NULL;
                lru_data_tail = del_node->pre;
        }
        else
        {
                del_node->pre->next = del_node->next;
                del_node->next->pre = del_node->pre;
        }
}

/**
 * 初始化系统中的链表的头尾节点
 * 为memory_info, buffer_in_remote, buffer_from_remote, memory_from_remote的头尾部节点初始化
*/
void init_list()
{

        global_mem_info = (struct memory_info *)malloc(sizeof(struct memory_info));
        global_mem_info->ip = (char *)malloc(sizeof(char)*MAX_IP_ADDR);
        strcpy(global_mem_info->ip, "");

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

        remote_memory_head = (struct memory_from_remote *)malloc(sizeof(struct memory_from_remote));
        remote_memory_head->next = NULL;
        remote_memory_tail = remote_memory_head;
}


/**
 * 从指定文件夹中读取文件列表数据作为lru_data链表的buffer节点
 * 即文件夹下面有多少文件就会产生多少lru_node
 * @参数是传入路径
*/
void initial_lru_data(char *path)
{
        lru_data_head = (struct lru_data *)malloc(sizeof(struct lru_data));
        lru_data_head->next = NULL;
        lru_data_head->pre = NULL;
        lru_data_head->hot_degree = -1;
        lru_data_tail = lru_data_head;

        //read all file name under dir
        DIR *d;
        struct dirent *file_list;
        struct stat sb;

        int depth = 3; // open the depth of dir

        if (!(d = opendir(path)))
        {
                DBG(L(CRITICAL), "error opendir %s!!!", path);
                return;
        }

        while ((file_list = readdir(d)) != NULL)
        {
                FILE *pFile;
                long lSize;
                char *buffer;
                size_t result;

                //初始化节点路径，文件名信息
                char *tmp_path = (char *)malloc(sizeof(char)*MID_STR_LEN);
                struct lru_data *tmp_lru_data = (struct lru_data *)malloc(sizeof(struct lru_data));
                strcpy(tmp_path, path);
                //curr dir, pre dir and .. dir, to avoid dead cycle in dir
                if (strncmp(file_list->d_name, ".", 1) == 0)
                        continue;
                strcpy(tmp_lru_data->file_name, file_list->d_name);

                tmp_lru_data->type = file_list->d_type;
                tmp_lru_data->hot_degree = rand();
                tmp_lru_data->next = NULL;

                //拼接好tmp_path路径，然后打开文件
                strcat(tmp_path, file_list->d_name);

                pFile = fopen(tmp_path, "rb");
                if (pFile == NULL)
                {
                        DBG(L(CRITICAL), "open file: %s with error! ", stderr);
                        // exit (1);
                        return;
                }

                /* get the size of file */
                fseek(pFile, 0, SEEK_END);
                //规范buffer的吃尺寸大小，为基准页大小的倍数，以便后面缺页中断能处理
                int page_size = sysconf(_SC_PAGESIZE);
                lSize = ftell(pFile);
                int buffer_size = (lSize / page_size) * page_size;
                if (lSize % page_size)
                {
                        buffer_size += page_size;
                }

                posix_memalign((void **)&buffer, sysconf(_SC_PAGESIZE), buffer_size);

                rewind(pFile);

                /* malloc buffer to storage file */
                if (buffer == NULL)
                {
                        fputs("Memory error", stderr);
                        DBG(L(CRITICAL),"malloc memory error!");
                        // exit (2);
                        return;
                }

                /* put the file to buffer */
                result = fread(buffer, 1, lSize, pFile);
                if (result != lSize)
                {
                        fputs("Reading error", stderr);
                        DBG(L(CRITICAL),"file to buffer failed!!");
                        // exit (3);
                        return;
                }
                tmp_lru_data->buffer = buffer;
                tmp_lru_data->size = buffer_size;
                tmp_lru_data->status = PULLED_WAIT;

                //add tmp node to list
                /*lru_data_tail->next = tmp_lru_data;
                lru_data_tail = tmp_lru_data;*/
                rbtree_insert(lru_data_tree, (unsigned long)tmp_lru_data->buffer, (void *)tmp_lru_data);
                add_lru_data_node_to_tail(tmp_lru_data);
                lru_data_sorting(tmp_lru_data);

                fclose(pFile);
                free(tmp_path);
                //if the file is dir and have searched dir depth = 3
                if (stat(file_list->d_name, &sb) >= 0 && S_ISDIR(sb.st_mode) && depth <= 3)
                {
                        initial_lru_data(file_list->d_name);
                }
        }
        closedir(d);
}

/**
 * 彻底释放lru_data双向链表所有节点
 * @注意，对节点对应的buffer也需要进行释放
*/
void release_lru_data()
{
        while (lru_data_head->next != NULL)
        {
                struct lru_data *tmp_lru_data = lru_data_head->next;
                free(tmp_lru_data->buffer);
                lru_data_head->next = tmp_lru_data->next;
                free(tmp_lru_data);
        }
        free(lru_data_head);
}

/**
 * 更新lru节点的信息
 * 参数：lru_data->buffer
 * 更新lru_data节点的热度值、 状态
 * 并且根据热度值重新排列在双向链表中的位置
*/
void update_lru_data(unsigned long addr)
{

        rbtree_node *rb_lru_node = rbtree_search(lru_data_tree, addr);
        struct lru_data *tmp_lru_data = NULL;

        if (rb_lru_node != NULL)
        {
                tmp_lru_data = (struct lru_data *)rb_lru_node->struct_addr;
        }

        if (tmp_lru_data != NULL)
        {
                tmp_lru_data->hot_degree = rand();
                tmp_lru_data->status = PULLED_WAIT;
                lru_data_sorting(tmp_lru_data);
        }
        else
        {
                DBG(L(WARNING),"tmp lru data not found!");
        }
}