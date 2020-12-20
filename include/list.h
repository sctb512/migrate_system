/*
 * @Description: 列表操作模块，包括插入、删除和排序等
 * @Version: 2.0
 * @Autor: xy&xy
 * @Date: 2020-07-22 15:01:19
 * @LastEditors: abin
 * @LastEditTime: 2020-08-06 22:16:19
 */


#ifndef LIST_H
#define list_h

#include <dirent.h>

#include "migcomm.h"

void add_from_remote_node_to_tail(struct buffer_from_remote *new_node);
void add_in_remote_node_to_tail(struct buffer_in_remote *new_node);
void add_lru_data_node_to_tail(struct lru_data *new_node);

void del_from_remote_node(struct buffer_from_remote *del_node);
void del_in_remote_node(struct buffer_in_remote *del_node);
void del_lru_data_node(struct lru_data *del_node);

void lru_data_sorting(struct lru_data *curr_node);
void buffer_from_remote_sorting(struct buffer_from_remote *curr_node);
void buffer_in_remote_sorting(struct buffer_in_remote *curr_node);

void init_list();
void initial_lru_data(char *path);
void release_lru_data();

void update_lru_data(unsigned long addr);


#endif