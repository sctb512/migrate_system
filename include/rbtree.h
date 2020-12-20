/*
 * @Description: 红黑树模块，加快查询速度，包括插入、删除和排序模块
 * @Version: 2.0
 * @Autor: abin&xy
 * @Date: 2020-07-21 11:32:58
 * @LastEditors: abin
 * @LastEditTime: 2020-08-20 17:08:16
 */
#ifndef RBTREE_H
#define RBTREE_H


#include "message.h"
#include "log.h"

typedef struct rbtreenode_item rbtree_node;
typedef struct rbtree_item rbtree;

enum RB_COLOR
{
	BLACK,
	RED
};

//rbtree node
struct rbtreenode_item
{
	enum RB_COLOR color;
	unsigned long local_addr;
	void *struct_addr;
	struct rbtreenode_item *left;
	struct rbtreenode_item *right;
	struct rbtreenode_item *parent;
};

//rbtree
struct rbtree_item
{
	rbtree_node *root;
	rbtree_node *nil;
};

//tree
rbtree *in_remote_tree;
rbtree *from_remote_tree;
rbtree *lru_data_tree;

//----------------------rbtree functions--------------------

void rbtree_init(rbtree *rbtree);
void rbtree_insert(rbtree *rbtree, unsigned long local_addr, void *struct_addr);
void rbtree_delete(rbtree *rbtree, rbtree_node *z);
int rbtree_inorder(rbtree_node *rbtnode, rbtree_node *nil);
rbtree_node *rbtree_search(rbtree *rbtree, unsigned long local_addr);

// for test
int rbtree_inorder_by_value(rbtree_node *rbtnode, rbtree_node *nil, unsigned long val);
int rbtree_inorder_by_key(rbtree_node *rbtnode, rbtree_node *nil, unsigned long key);

void swap(int *pm, int *pn);

void init_tree();

#endif
