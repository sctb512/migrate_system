#include "rbtree.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

/**
 * @description: 传入红黑树头指针，初始化一个红黑树
 * @param {rbtree *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rbtree_init(rbtree *rbtree)
{
	rbtree->nil = (rbtree_node *)malloc(sizeof(rbtree_node));
	rbtree->nil->local_addr = 0;
	rbtree->nil->struct_addr = NULL;
	rbtree->nil->color = BLACK;
	rbtree->nil->left = NULL;
	rbtree->nil->right = NULL;
	rbtree->nil->parent = NULL;
	rbtree->root = rbtree->nil;
}

/**
 * @description: 红黑树搜索，传入一个红黑树，根据地址快速搜索，返回搜索到的节点
 * @param {rbtree *, unsigned long}
 * @return {rbtree_node *}
 * @author: abin&xy
 * @last_editor: abin
 */
rbtree_node *rbtree_search(rbtree *rbtree, unsigned long local_addr)
{
	rbtree_node *x = NULL;
	x = rbtree->root;
	while (x != rbtree->nil)
	{
		if (local_addr > x->local_addr)
			x = x->right;
		else if (local_addr < x->local_addr)
			x = x->left;
		else if (local_addr == x->local_addr)
			return x;
	}
	x = NULL;
	return x;
}

/**
 * @description: 红黑树操作：左旋
 * @param {rbtree *， rbtree_node *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void left_rotate(rbtree *rbtree, rbtree_node *x)
{
	rbtree_node *y;
	y = x->right;
	x->right = y->left;
	if (y->left != rbtree->nil)
		y->left->parent = x;
	if (x != rbtree->nil)
		y->parent = x->parent;
	if (x->parent == rbtree->nil)
		rbtree->root = y;
	else
	{
		if (x->parent->left == x)
			x->parent->left = y;
		else
			x->parent->right = y;
	}
	y->left = x;
	if (x != rbtree->nil)
		x->parent = y;
}

/**
 * @description: 红黑树操作：右旋
 * @param {rbtree *， rbtree_node *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void right_rotate(rbtree *rbtree, rbtree_node *x)
{
	rbtree_node *y;
	y = x->left;
	x->left = y->right;
	if (y->right != rbtree->nil)
		y->right->parent = x;
	if (x != rbtree->nil)
		y->parent = x->parent;
	if (x->parent == rbtree->nil)
		rbtree->root = y;
	else
	{
		if (x->parent->left == x)
			x->parent->left = y;
		else
			x->parent->right = y;
	}
	y->right = x;
	if (x != rbtree->nil)
		x->parent = y;
}

/**
 * @description: 红黑树，插入之后进行修正
 * @param {rbtree *}
 * @return {rbtree_node *}
 * @author: abin&xy
 * @last_editor: abin
 */
void rb_insert_fixup(rbtree *rbtree, rbtree_node *z)
{
	rbtree_node *y;
	while ((z->parent != NULL) && (z->parent->color == RED))
	{
		if (z->parent == z->parent->parent->left)
		{
			y = z->parent->parent->right;
			if ((y != NULL) && (y->color == RED))
			{
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else
			{
				if (z == z->parent->right)
				{
					z = z->parent;
					left_rotate(rbtree, z);
				}
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				right_rotate(rbtree, z->parent->parent);
			}
		}
		else
		{
			y = z->parent->parent->left;
			if ((y != NULL) && (y->color == RED))
			{
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else
			{
				if (z == z->parent->left)
				{
					z = z->parent;
					right_rotate(rbtree, z);
				}
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				left_rotate(rbtree, z->parent->parent);
			}
		}
	}
	rbtree->root->color = BLACK;
	rbtree->nil->color = BLACK;
}

/**
 * @description: 红黑树，插入一个节点，能够通过local_addr快速搜索
 * @param {rbtree *, unsigned long, void *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rbtree_insert(rbtree *rbtree, unsigned long local_addr, void *struct_addr)
{
	rbtree_node *x, *y, *z;
	x = rbtree->root;
	y = rbtree->nil;
	z = (rbtree_node *)malloc(sizeof(rbtree_node));
	assert(z);
	z->color = RED;
	z->local_addr = local_addr;
	z->struct_addr = struct_addr;
	z->parent = NULL;
	z->left = NULL;
	z->right = NULL;
	while (x != rbtree->nil)
	{
		y = x;
		if (z->local_addr < x->local_addr)
			x = x->left;
		else if (z->local_addr > x->local_addr)
			x = x->right;
		else if (z->local_addr == x->local_addr)
		{
			free(z);
			return;
		}
	}
	z->parent = y;
	if (y != rbtree->nil)
	{
		if (z->local_addr > y->local_addr)
			y->right = z;
		else if (z->local_addr < y->local_addr)
			y->left = z;
	}
	else
	{
		rbtree->root = z;
	}
	z->left = rbtree->nil;
	z->right = rbtree->nil;
	rb_insert_fixup(rbtree, z);
}

/**
 * @description: 删除节点之后进行修正
 * @param {type *， rbtree_node *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rbtree_delete_fixup(rbtree *rbtree, rbtree_node *x)
{
	rbtree_node *w;
	while ((x != rbtree->root) && (x->color == BLACK))
	{
		if (x == x->parent->left)
		{
			w = x->parent->right;
			if (w->color == RED)
			{
				w->color = BLACK;
				x->parent->color = RED;
				left_rotate(rbtree, x->parent);
				w = x->parent->right;
			}
			if (w->left->color == BLACK && w->right->color == BLACK)
			{
				w->color = RED;
				x = x->parent;
			}
			else
			{
				if (w->right->color == BLACK)
				{
					w->left->color = BLACK;
					w->color = RED;
					right_rotate(rbtree, w);
					w = x->parent->right;
				}
				w->color = w->parent->color;
				x->parent->color = BLACK;
				w->right->color = BLACK;
				left_rotate(rbtree, x->parent);
				x = rbtree->root;
			}
		}
		else
		{
			w = x->parent->left;
			if (w->color == RED)
			{
				w->color = BLACK;
				x->parent->color = RED;
				right_rotate(rbtree, x->parent);
				w = x->parent->left;
			}
			if (w->right->color == BLACK && w->left->color == BLACK)
			{
				w->color = RED;
				x = x->parent;
			}
			else
			{
				if (w->left->color == BLACK)
				{
					w->right->color = BLACK;
					w->color = RED;
					left_rotate(rbtree, w);
					w = x->parent->left;
				}
				w->color = w->parent->color;
				x->parent->color = BLACK;
				w->left->color = BLACK;
				right_rotate(rbtree, x->parent);
				x = rbtree->root;
			}
		}
	}
	x->color = BLACK;
}

/**
 * @description: 删除节点
 * @param {rbtree *, rbtree_node *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rbtree_delete(rbtree *rbtree, rbtree_node *z)
{
	rbtree_node *y, *x;
	if (z->left == rbtree->nil || z->right == rbtree->nil)
		y = z;
	else
	{
		y = z->right;
		while (y->left != rbtree->nil)
			y = y->left;
	}
	if (y->left != rbtree->nil)
		x = y->left;
	else
		x = y->right;
	x->parent = y->parent;
	if (y->parent == rbtree->nil)
		rbtree->root = x;
	else
	{
		if (y->parent->left == y)
			y->parent->left = x;
		else
			y->parent->right = x;
	}
	// 这里有个bug，已修复
	if (y != z)
	{
		z->local_addr = y->local_addr;
		z->struct_addr = y->struct_addr;
	}
	if (y->color == BLACK)
		rbtree_delete_fixup(rbtree, x);
	free(y);
}


/**
 * @description: 对红黑树进行重新中序遍历
 * @param {rbtree_node *, rbtree_node *}
 * @return {int}
 * @author: abin&xy
 * @last_editor: abin
 */
int rbtree_inorder(rbtree_node *rbtnode, rbtree_node *nil)
{
	int l = 0, r = 0, h = 0;
	if (rbtnode == nil)
	{
		return 0;
	}
	else
	{
		if (rbtnode->color == RED && rbtnode->parent->color == RED)
		{
			exit(0);
		}
		l = rbtree_inorder(rbtnode->left, nil);
		// DBG(L(TEST), "$ local addr: \033[35m%p\033[0m, struct addr: \033[36m%p\033[0m", rbtnode->local_addr, rbtnode->struct_addr);
		r = rbtree_inorder(rbtnode->right, nil);
	}
	h = (rbtnode->color == RED) ? 0 : 1;
	h += (l > r) ? l : r;

	return h;
}

/**
 * @description: 根据value对红黑树进行中序遍历，测试用
 * @param {rbtree_node *, rbtree_node *}
 * @return {int}
 * @author: abin&xy
 * @last_editor: abin
 */
int rbtree_inorder_by_value(rbtree_node *rbtnode, rbtree_node *nil, unsigned long val)
{
	int l = 0, r = 0, h = 0;
	if (rbtnode == nil)
	{
		return 0;
	}
	else
	{
		if (rbtnode->color == RED && rbtnode->parent->color == RED)
		{
			exit(0);
		}
		if ((unsigned long)(rbtnode->struct_addr) == val)
		{
			// DBG(L(TEST), "$ tree addr: \033[31m%p\033[0m \t local addr: \033[35m%p\033[0m", rbtnode->struct_addr, rbtnode->local_addr);
			return 0;
		}
		l = rbtree_inorder_by_value(rbtnode->left, nil, val);
		r = rbtree_inorder_by_value(rbtnode->right, nil, val);
	}
	h = (rbtnode->color == RED) ? 0 : 1;
	h += (l > r) ? l : r;

	return h;
}

/**
 * @description: 根据key对红黑树进行中序遍历，测试用
 * @param {rbtree_node *, rbtree_node *}
 * @return {int}
 * @author: abin&xy
 * @last_editor: abin
 */
int rbtree_inorder_by_key(rbtree_node *rbtnode, rbtree_node *nil, unsigned long key)
{
	int l = 0, r = 0, h = 0;
	if (rbtnode == nil)
	{
		return 0;
	}
	else
	{
		if (rbtnode->color == RED && rbtnode->parent->color == RED)
		{
			exit(0);
		}

		l = rbtree_inorder_by_value(rbtnode->left, nil, key);
		if ((unsigned long)(rbtnode->local_addr) == key)
		{
			// DBG(L(TEST), "$ tree addr: \033[31m%p\033[0m, local addr: \033[35m%p\033[0m", rbtnode->struct_addr, rbtnode->local_addr);
			return 0;
		}
		r = rbtree_inorder_by_value(rbtnode->right, nil, key);
	}
	h = (rbtnode->color == RED) ? 0 : 1;
	h += (l > r) ? l : r;

	return h;
}



/**
 * @description: 交换两个指针
 * @param {int *, int*}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void swap(int *pm, int *pn)
{
	int temp;
	temp = *pm;
	*pm = *pn;
	*pn = temp;
}


/**
 * @description: 初始化三个红黑树，in_remote_tree、from_remote_tree、lru_data_tree
 * @param {void}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void init_tree()
{
        in_remote_tree = (rbtree *)malloc(sizeof(rbtree));
        rbtree_init(in_remote_tree);
        from_remote_tree = (rbtree *)malloc(sizeof(rbtree));
        rbtree_init(from_remote_tree);
        lru_data_tree = (rbtree *)malloc(sizeof(rbtree));
        rbtree_init(lru_data_tree);
}