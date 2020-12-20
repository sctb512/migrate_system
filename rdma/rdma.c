#include "rdma.h"

/**
 * @description: 通过RDMA读取远程机器内存中的数据，保存在ctx->buffer中，需要提前知道远程机器的buffer地址和rkey
 * @param {struct rdma_cm_i, uint32_t}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void read_remote(struct rdma_cm_id *id, uint32_t len)
{
        struct rdma_context *ctx = (struct rdma_context *)id->context;

        struct ibv_send_wr wr, *bad_wr = NULL;
        struct ibv_sge sge;

        memset(&wr, 0, sizeof(wr));

        wr.wr_id = (uintptr_t)id;
        wr.opcode = IBV_WR_RDMA_READ;
        wr.send_flags = IBV_SEND_SIGNALED;
        //wr.imm_data = htonl(len);
        wr.wr.rdma.remote_addr = ctx->peer_addr;
        wr.wr.rdma.rkey = ctx->peer_rkey;

        if (len)
        {
                wr.sg_list = &sge;
                wr.num_sge = 1;

                sge.addr = (uintptr_t)ctx->buffer;
                sge.length = len;
                sge.lkey = ctx->buffer_mr->lkey;
        }

        // TEST_NZ(ibv_post_send(id->qp, &wr, &bad_wr));
        int send_ret = ibv_post_send(id->qp, &wr, &bad_wr);
        if (send_ret != 0) {
                DBG(L(TEST), "ibv_post_send error: %s", strerror(errno));
        }
}

/**
 * @description: 通过rdma协议将ctx->buffer的数据写入远程机器的buffer，需要提前获取远程机器的buffer地址和rkey
 * @param {struct rdma_cm_id *, uint32_t }
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void write_remote(struct rdma_cm_id *id, uint32_t len)
{
        struct rdma_context *ctx = (struct rdma_context *)id->context;

        struct ibv_send_wr wr, *bad_wr = NULL;
        struct ibv_sge sge;

        memset(&wr, 0, sizeof(wr));

        wr.wr_id = (uintptr_t)id;
        wr.opcode = IBV_WR_RDMA_WRITE;
        wr.send_flags = IBV_SEND_SIGNALED;
        //wr.imm_data = htonl(len);
        wr.wr.rdma.remote_addr = ctx->peer_addr;
        wr.wr.rdma.rkey = ctx->peer_rkey;

        if (len)
        {
                wr.sg_list = &sge;
                wr.num_sge = 1;

                sge.addr = (uintptr_t)ctx->buffer;
                sge.length = len;
                sge.lkey = ctx->buffer_mr->lkey;
        }

        // TEST_NZ(ibv_post_send(id->qp, &wr, &bad_wr));
        int send_ret = ibv_post_send(id->qp, &wr, &bad_wr);
        if (send_ret != 0) {
                DBG(L(TEST), "ibv_post_send error: %s", strerror(errno));
        }
}

/**
 * @description: 创建一个线程用于接收消息，如果不执行此函数，将不能收到对方机器发来的消息
 * @param {struct rdma_cm_id *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void post_receive(struct rdma_cm_id *id)
{
        struct rdma_context *ctx = (struct rdma_context *)id->context;

        struct ibv_recv_wr wr, *bad_wr = NULL;
        struct ibv_sge sge;

        memset(&wr, 0, sizeof(wr));

        wr.wr_id = (uintptr_t)id;
        wr.sg_list = &sge;
        wr.num_sge = 1;

        sge.addr = (uintptr_t)ctx->msg;
        sge.length = sizeof(*ctx->msg);
        sge.lkey = ctx->msg_mr->lkey;

        // TEST_NZ(ibv_post_recv(id->qp, &wr, &bad_wr));
        int recv_ret = ibv_post_recv(id->qp, &wr, &bad_wr);
        if (recv_ret != 0) {
                DBG(L(TEST), "ibv_post_recv error: %s", strerror(errno));
        }
}

/**
 * @description: 通过rdma协议中的send发送消息
 * @param {struct rdma_cm_id *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void send_message(struct rdma_cm_id *id)
{
        struct rdma_context *ctx = (struct rdma_context *)id->context;

        struct ibv_send_wr wr, *bad_wr = NULL;
        struct ibv_sge sge;

        memset(&wr, 0, sizeof(wr));

        wr.wr_id = (uintptr_t)id;
        wr.opcode = IBV_WR_SEND;
        wr.sg_list = &sge;
        wr.num_sge = 1;
        wr.send_flags = IBV_SEND_SIGNALED;

        sge.addr = (uintptr_t)ctx->msg;
        sge.length = sizeof(*ctx->msg);
        sge.lkey = ctx->msg_mr->lkey;

        // TEST_NZ(ibv_post_send(id->qp, &wr, &bad_wr));
        int send_ret = ibv_post_send(id->qp, &wr, &bad_wr);
        if (send_ret != 0) {
                DBG(L(TEST), "ibv_post_send error: %s", strerror(errno));
        }
}