#include "userfaultfd.h"

/**
 * @description: pagefault处理函数，该函数会以线程的方式后台运行
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void *fault_handler_thread()
{
        static struct uffd_msg msg; /* Data read from userfaultfd */
        // struct uffdio_copy uffdio_copy;
        ssize_t nread;


        /* Loop, handling incoming events on the userfaultfd file descriptor */
        for (;;)
        {
                /* See what poll() tells us about the userfaultfd */
                struct pollfd pollfd;
                int nlru;
                pollfd.fd = uffd;
                pollfd.events = POLLIN;
                nlru = poll(&pollfd, 1, -1);
                if (nlru == -1)
                        errExit("poll");

                DBG(L(INFO), "\nfault_handler_thread():");
                DBG(L(INFO), "    poll() returns: nlru = %d; "
                       "POLLIN = %d; POLLERR = %d",
                       nlru,
                       (pollfd.revents & POLLIN) != 0,
                       (pollfd.revents & POLLERR) != 0);

                /* Read an event from the userfaultfd */

                nread = read(uffd, &msg, sizeof(msg));
                if (nread == 0)
                {
                        DBG(L(CRITICAL), "EOF on userfaultfd!");
                        exit(EXIT_FAILURE);
                }

                if (nread == -1)
                        errExit("read");

                /* We expect only one kind of event; verify that assumption */

                if (msg.event != UFFD_EVENT_PAGEFAULT)
                {
                        DBG(L(CRITICAL), "Unexpected event on userfaultfd");
                        exit(EXIT_FAILURE);
                }

                /* Display info about the page-fault event */
                long long pagefault_addr = msg.arg.pagefault.address;

                DBG(L(INFO), "    UFFD_EVENT_PAGEFAULT event: ");
                DBG(L(INFO), "flags = %llx; ", msg.arg.pagefault.flags);
                // DBG(L(TEST), "pagefault addr: 0x%llx", pagefault_addr);

                /* Copy the page pointed to by 'page' into the faulting
                region. Vary the contents that are copied in, so that it
                is more obvious that each fault is handled separately. */

                //根据地址找到链表结构体
                #ifdef TEST_BREAKDOWN_PAGEFAULT
                unsigned long long pagefault_step1_start = get_now_time_us();
                #endif

                struct buffer_in_remote *tmp_in_remote = in_remote_head;
                struct buffer_in_remote *curr_in_remote = NULL;
                while (tmp_in_remote->next != NULL)
                {
                        tmp_in_remote = tmp_in_remote->next;

                        if (tmp_in_remote->local_addr <= pagefault_addr && pagefault_addr < (tmp_in_remote->local_addr + tmp_in_remote->size))
                        {
                                curr_in_remote = tmp_in_remote;
                                break;
                        }
                }
                
                #ifdef TEST_BREAKDOWN_PAGEFAULT
                unsigned long long pagefault_step1_end = get_now_time_us();
                DBG(L(DATA), "301,pagefault_step1,find_list_in_remote_node_by_va,%lld,%lld,%lld,%s", pagefault_step1_start, pagefault_step1_end, pagefault_step1_end-pagefault_step1_start, get_now_time());
                #endif
                
                // DBG(L(TEST), "curr_in_remote: %lld", tmp_in_remote->local_addr + tmp_in_remote->size);
                // DBG(L(TEST), "curr_in_remote: %p", curr_in_remote);

                if (curr_in_remote != NULL)     //在in_remote链表里面找，判断是否在远程
                {
                        // DBG(L(TEST), "ok");
                        pagefault_evicted_data(curr_in_remote->local_addr);
                        
                        //容错处理
                        int i =0;
                        while (!curr_in_remote->evicted)
                        {
                                if (i == 2)
                                {
                                        i = 0;
                                        curr_in_remote->status = EVICT_WAIT;
                                        pagefault_evicted_data(curr_in_remote->local_addr);
                                }

                                sleep_ms(50);
                                i++;
                        }

                        // uffdio_copy.src = (unsigned long)curr_in_remote->new_addr;

                        /* We need to handle page faults in units of pages(!).
                        So, round faulting address down to page boundary */

                        // uffdio_copy.dst = (unsigned long)pagefault_addr;
                        // uffdio_copy.len = curr_in_remote->size;
                        // uffdio_copy.mode = 0;
                        // uffdio_copy.copy = 0;

                        // if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
                        // {
                        //         errExit("ioctl-UFFDIO_COPY");
                        //         // uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
                        //         register_userfaultfd(curr_in_remote->local_addr, curr_in_remote->size);
                        //         if (ioctl(uffd, UFFDIO_COPY, &uffdio_copy) == -1)
                        //         {
                        //                 perror("ioctl-UFFDIO_COPY");
                        //         }
                        // }

                        // DBG(L(INFO), "(uffdio_copy.copy returned %lld)", uffdio_copy.copy);

                        // madvise((void *)curr_in_remote->new_addr, curr_in_remote->size, MADV_DONTNEED);
                        // free((void *)curr_in_remote->new_addr);

                        // #ifdef TEST_BREAKDOWN_PAGEFAULT
                        // unsigned long long pagefault_step12_start = get_now_time_us();
                        // #endif
                        
                        // unregister_userfaultfd((unsigned long)curr_in_remote->local_addr, curr_in_remote->size);

                        // #ifdef TEST_BREAKDOWN_PAGEFAULT
                        // unsigned long long pagefault_step12_end = get_now_time_us();
                        // DBG(L(DATA), "301,pagefault_step12,unregister_userfaultfd,%lld,%lld,%lld,%s", pagefault_step12_start, pagefault_step12_end, pagefault_step12_end-pagefault_step12_start, get_now_time());
                        
                        // unsigned long long pagefault_step13_start = get_now_time_us();
                        // #endif

                        // update_lru_data((unsigned long)curr_in_remote->local_addr);

                        // #ifdef TEST_BREAKDOWN_PAGEFAULT
                        // unsigned long long pagefault_step13_end = get_now_time_us();
                        // DBG(L(DATA), "301,pagefault_step13,update_lru_data,%lld,%lld,%lld,%s", pagefault_step13_start, pagefault_step13_end, pagefault_step13_end-pagefault_step13_start, get_now_time());
                        // #endif

                        curr_in_remote->status = EVICT_DONE;
                        free(curr_in_remote);
                }
                else
                { //如果远程找不到数据，则到本地磁盘找寻，这也符合带宽不同背景
                        struct lru_data *tmp_lru_data = lru_data_head;
                        struct lru_data *curr_lru_data = NULL;
                        while (tmp_lru_data->next != NULL)
                        {
                                tmp_lru_data = tmp_lru_data->next;
                                //要这样找，因为访问的地址不一定是LRU的起始地址，而可能是中间某一个地址
                                if ((unsigned long)tmp_lru_data->buffer <= pagefault_addr && pagefault_addr <= (unsigned long)tmp_lru_data->buffer + tmp_lru_data->size)
                                {
                                        curr_lru_data = tmp_lru_data;
                                        break;
                                }
                        }

                        if (curr_lru_data != NULL)
                        {
                                // DBG(L(TEST), "ok!");
                                FILE *fd = fopen(curr_lru_data->file_name, "rb");
                                // int fd = open(curr_lru_data->file_name, O_RDONLY | O_NONBLOCK);
                                if(fd == NULL)
                                {
                                        DBG(L(ERR), "open file failed!");
                                }else
                                {
                                        DBG(L(INFO), "open file success!");
                                }
                                
                                // DBG(L(TEST), "curr_lru_data->buffer: %p, curr_lru_data->size: %d", (unsigned long)curr_lru_data->buffer, curr_lru_data->size);
                                
                                unregister_userfaultfd((unsigned long)curr_lru_data->buffer, curr_lru_data->size);
                                size_t lSize = fread(curr_lru_data->buffer, 1, curr_lru_data->size, fd);
                                if (lSize == curr_lru_data->size)
                                {
                                        to_ssd_num -- ;
                                        curr_lru_data->status = PULLED_WAIT;
                                        curr_lru_data->hot_degree = rand();
                                        lru_data_sorting(curr_lru_data);

                                        if (!remove(curr_lru_data->file_name))
                                        {
                                                // DBG(L(INFO), "remove file %s successful!", curr_from_remote->file_name);
                                                strcpy(curr_lru_data->file_name, "");
                                        }
                                        else
                                        {
                                                DBG(L(ERR), "remove file %s failed!", curr_lru_data->file_name);
                                        }
                                        fclose(fd);
                                }
                                else
                                {
                                        DBG(L(ERR), "file to lru_data->buffer failed!!");
                                }
                        }
                        else
                        {
                                DBG(L(WARNING), "Cant't find lru_node or in_remote node by pagefault_addr!");
                        }
                }
                // sleep_ms(5);
        }
}

/**
 * @description: 初始化，主要进行一额些必要的初始化和开启缺页中断处理函数的线程
 * @param {void}
 * @return {int}
 * @author: abin&xy
 * @last_editor: abin
 */
int init_userfaultfd()
{

        struct uffdio_api uffdio_api;
        int s;
        pthread_t thr;

        uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
        if (uffd == -1)
                errExit("userfaultfd");

        uffdio_api.api = UFFD_API;
        uffdio_api.features = 0;
        if (ioctl(uffd, UFFDIO_API, &uffdio_api) == -1)
                errExit("ioctl-UFFDIO_API");

        s = pthread_create(&thr, NULL, fault_handler_thread, NULL);
        if (s != 0)
        {
                DBG(L(INFO), "create userfaultfd pthread unsuccessful!");
        }
        else
        {
                DBG(L(INFO), "create userfaultfd pthread successful!");
        }

        return 0;
}

/**
 * @description: 传入地址和长度，将该地址对应长度注册进缺页中断处理程序，如果该地址发生缺页中断，将会触发缺页中断处理程序线程进行处理
 * @param {unsigned long, unsigned long }
 * @return {int}
 * @author: abin&xy
 * @last_editor: abin
 */
int register_userfaultfd(unsigned long addr, unsigned long len)
{
        uffdio_register.range.start = addr;
        uffdio_register.range.len = len;
        uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;

        if (ioctl(uffd, UFFDIO_REGISTER, &uffdio_register) == -1)
        {
                errExit("ioctl-UFFDIO_REGISTER");
        }

        return 0;
}

/**
 * @description: 根据传入地址和长度，取消注册缺页中断，取消后，该地址发生缺页中断将不会出发缺页中断处理线程
 * @param {unsigned long ,unsigned long}
 * @return {int}
 * @author: abin&xy
 * @last_editor: abin
 */
int unregister_userfaultfd(unsigned long addr, unsigned long len)
{

        uffdio_register.range.start = addr;
        uffdio_register.range.len = len;
        uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
        if (ioctl(uffd, UFFDIO_UNREGISTER, &uffdio_register) == -1)
                errExit("ioctl-UFFDIO_REGISTER");

        return 0;
}
