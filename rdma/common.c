#include "common.h"

const int TIMEOUT_IN_MS = 500;

struct context
{
        struct ibv_context *ctx;
        struct ibv_pd *pd;
        struct ibv_cq *cq;
        struct ibv_comp_channel *comp_channel;

        pthread_t cq_poller_thread;
};

static struct context *s_ctx = NULL;
static pre_conn_cb_fn s_on_pre_conn_cb = NULL;
static connect_cb_fn s_on_connect_cb = NULL;
static completion_cb_fn s_on_completion_cb = NULL;
static disconnect_cb_fn s_on_disconnect_cb = NULL;

static void build_context(struct ibv_context *verbs);
static void build_qp_attr(struct ibv_qp_init_attr *qp_attr);
static void event_loop(struct rdma_event_channel *ec, int exit_on_disconnect, struct request_message *req_msg);
static void *poll_cq(void *);

/**
 * @description: 构建rdma连接，该函数通过调用rdma接口函数实现
 * @param {struct rdma_cm_id *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void build_connection(struct rdma_cm_id *id)
{
        struct ibv_qp_init_attr qp_attr;

        build_context(id->verbs);
        build_qp_attr(&qp_attr);

        // TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));
        int qp_ret = rdma_create_qp(id, s_ctx->pd, &qp_attr);
        if (qp_ret != 0) {
                DBG(L(TEST), "rdma_create_qp error: %s", strerror(errno));
        }
}

/**
 * @description: 构建rdma通信的上下文
 * @param {struct ibv_context *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void build_context(struct ibv_context *verbs)
{
        if (s_ctx)
        {
                if (s_ctx->ctx != verbs)
                        rc_die("cannot handle events in more than one context.");

                return;
        }

        s_ctx = (struct context *)malloc(sizeof(struct context));

        s_ctx->ctx = verbs;

        // TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
        s_ctx->pd = ibv_alloc_pd(s_ctx->ctx);
        if (s_ctx->pd == NULL) {
                DBG(L(TEST), "ibv_alloc_pd error: %s", strerror(errno));
        }
        // TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
        s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx);
        if (s_ctx->comp_channel == NULL) {
                DBG(L(TEST), "ibv_create_comp_channel error: %s", strerror(errno));
        }
        // TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0)); /* cqe=10 is arbitrary */
        s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0); /* cqe=10 is arbitrary */
        if (s_ctx->cq == NULL) {
                DBG(L(TEST), "ibv_create_cq error: %s", strerror(errno));
        }
        // TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));
        int cp_ret = ibv_req_notify_cq(s_ctx->cq, 0);
        if (cp_ret != 0) {
                DBG(L(TEST), "ibv_req_notify_cq error: %s", strerror(errno));
        }

        // TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));

        int poller_ret = pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL);

        if (poller_ret != 0) {
                DBG(L(TEST), "cq_poller_thread create error: %s", strerror(errno));
        }
}

/**
 * @description: 构建参数
 * @param {struct rdma_conn_param *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void build_params(struct rdma_conn_param *params)
{
        params->initiator_depth = params->responder_resources = 1;
        params->rnr_retry_count = 7; /* infinite retry */
}

/**
 * @description: 构建qp的参数
 * @param {struct ibv_qp_init_attr *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
        memset(qp_attr, 0, sizeof(*qp_attr));

        qp_attr->send_cq = s_ctx->cq;
        qp_attr->recv_cq = s_ctx->cq;
        qp_attr->qp_type = IBV_QPT_RC;

        qp_attr->cap.max_send_wr = 10;
        qp_attr->cap.max_recv_wr = 10;
        qp_attr->cap.max_send_sge = 1;
        qp_attr->cap.max_recv_sge = 1;
}

/**
 * @description: 事件循环，根据rdma的不同事件会执行不同的代码逻辑
 * @param {struct rdma_event_channel *, int, struct request_message *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void event_loop(struct rdma_event_channel *ec, int exit_on_disconnect, struct request_message *req_msg)
{
        struct rdma_cm_event *event = NULL;
        struct rdma_conn_param cm_params;
        memset(&cm_params, 0, sizeof(cm_params));

        if (req_msg != NULL)
        {
                // DBG(L(TEST),"req_msg addr: %p, msg type: %d", req_msg->addr, req_msg->msg_type);
                cm_params.private_data = req_msg;
                cm_params.private_data_len = sizeof(struct request_message);
        }
        build_params(&cm_params);

        while (rdma_get_cm_event(ec, &event) == 0)
        {
                struct rdma_cm_event event_copy;

                memcpy(&event_copy, event, sizeof(*event));
                rdma_ack_cm_event(event);

                if (event_copy.event == RDMA_CM_EVENT_ADDR_RESOLVED)
                {
                        build_connection(event_copy.id);

                        if (s_on_pre_conn_cb)
                                s_on_pre_conn_cb(event_copy.id, NULL);

                        // TEST_NZ(rdma_resolve_route(event_copy.id, TIMEOUT_IN_MS));
                        int route_ret = rdma_resolve_route(event_copy.id, TIMEOUT_IN_MS);
                        if (route_ret != 0) {
                                DBG(L(TEST), "rdma_resolve_route error: %s", strerror(errno));
                        }
                }
                else if (event_copy.event == RDMA_CM_EVENT_ROUTE_RESOLVED)
                {

                        // TEST_NZ(rdma_connect(event_copy.id, &cm_params));
                        int connect_ret = rdma_connect(event_copy.id, &cm_params);
                        if (connect_ret != 0) {
                                DBG(L(TEST), "rdma_connect error: %s", strerror(errno));
                        }
                }
                else if (event_copy.event == RDMA_CM_EVENT_CONNECT_REQUEST)
                {

                        struct request_message *recv_req_msg = NULL;
                        if (event->param.conn.private_data_len < 4)
                        {
                                DBG(L(CRITICAL), "unexpected private data len: %d", event->param.conn.private_data_len);
                        }
                        else
                        {
                                struct request_message tmp_msg;
                                memcpy(&tmp_msg, (struct request_message *)event->param.conn.private_data, sizeof(struct request_message));
                                recv_req_msg = &tmp_msg;
                        }

                        build_connection(event_copy.id);

                        if (s_on_pre_conn_cb)
                                s_on_pre_conn_cb(event_copy.id, recv_req_msg);

                        // TEST_NZ(rdma_accept(event_copy.id, &cm_params));
                        int accept_ret = rdma_accept(event_copy.id, &cm_params);
                        if (accept_ret != 0) {
                                DBG(L(TEST), "rdma_accept error: %s", strerror(errno));
                        }
                }
                else if (event_copy.event == RDMA_CM_EVENT_ESTABLISHED)
                {

                        if (s_on_connect_cb && req_msg == NULL)
                                s_on_connect_cb(event_copy.id);
                }
                else if (event_copy.event == RDMA_CM_EVENT_DISCONNECTED)
                {
                        rdma_destroy_qp(event_copy.id);

                        if (s_on_disconnect_cb)
                        {

                                s_on_disconnect_cb(event_copy.id, req_msg == NULL ? 1 : 0);
                        }

                        rdma_destroy_id(event_copy.id);

                        if (exit_on_disconnect)
                                break;
                }
                else if (event_copy.event == RDMA_CM_EVENT_REJECTED)
                {
                        rdma_destroy_qp(event_copy.id);

                        if (s_on_disconnect_cb)
                        {
                                s_on_disconnect_cb(event_copy.id, req_msg == NULL ? 1 : 0);
                        }

                        rdma_destroy_id(event_copy.id);

                        return;
                } else
                {
                        //rc_die("unknown event\n");
                        DBG(L(CRITICAL), "other rdma event: %s", rdma_event_str(event_copy.event));

                        return;
                }
        }
}

/**
 * @description: work_complication事件轮询，捕获rdma产生的完成事件并执行on_completion函数
 * @param {void *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void *poll_cq(void *ctx)
{
        struct ibv_cq *cq;
        struct ibv_wc wc;

        while (1)
        {
                // TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
                int cq_event_ret = ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx);
                if (cq_event_ret != 0) {
                        DBG(L(TEST), "ibv_get_cq_event error: %s", strerror(errno));
                }
                ibv_ack_cq_events(cq, 1);
                // TEST_NZ(ibv_req_notify_cq(cq, 0));
                int notify_cq_ret = ibv_req_notify_cq(cq, 0);
                if (notify_cq_ret != 0) {
                        DBG(L(TEST), "ibv_req_notify_cq error: %s", strerror(errno));
                }

                while (ibv_poll_cq(cq, 1, &wc))
                {
                        if (wc.status == IBV_WC_SUCCESS)
                                s_on_completion_cb(&wc);
                        else if(wc.status == IBV_WC_WR_FLUSH_ERR)
                                continue;
                        else
                        {
                                DBG(L(INFO),"! rdma_poll_cq, wc status: %s", ibv_wc_status_str(wc.status));
                                //rc_die("poll_cq: status is not IBV_WC_SUCCES\n");
                                continue;
                        }
                }
        }

        return NULL;
}

/**
 * @description: 初始化函数，将四个函数的指针赋值给s_on_pre_conn_cb、s_on_connect_cb、s_on_completion_cb和s_on_disconnect_cb
 * @param {pre_conn_cb_fn, connect_cb_fn, completion_cb_fn, disconnect_cb_fn}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rc_init(pre_conn_cb_fn pc, connect_cb_fn conn, completion_cb_fn comp, disconnect_cb_fn disc)
{
        s_on_pre_conn_cb = pc;
        s_on_connect_cb = conn;
        s_on_completion_cb = comp;
        s_on_disconnect_cb = disc;
}

/**
 * @description: 发起rdma请求，在断开连接之退出本函数
 * @param {const char *, const char *, void *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rc_client_loop(const char *host, const char *port, void *context)
{
        struct addrinfo *addr;
        struct rdma_cm_id *conn = NULL;
        struct rdma_event_channel *ec = NULL;

        // TEST_NZ(getaddrinfo(host, port, NULL, &addr));
        int addr_ret = getaddrinfo(host, port, NULL, &addr);
        if (addr_ret != 0) {
                DBG(L(TEST), "getaddrinfo error: %s", strerror(errno));
        }

        // TEST_Z(ec = rdma_create_event_channel());
        ec = rdma_create_event_channel();
        if (ec == NULL) {
                DBG(L(TEST), "rdma_create_event_channel error: %s", strerror(errno));
        }
        // TEST_NZ(rdma_create_id(ec, &conn, NULL, RDMA_PS_TCP));
        int create_id_ret = rdma_create_id(ec, &conn, NULL, RDMA_PS_TCP);
        if (create_id_ret != 0) {
                DBG(L(TEST), "rdma_create_id error: %s", strerror(errno));
        }
        // TEST_NZ(rdma_resolve_addr(conn, NULL, addr->ai_addr, TIMEOUT_IN_MS));
        int resolve_addr_ret = rdma_resolve_addr(conn, NULL, addr->ai_addr, TIMEOUT_IN_MS);
        if (resolve_addr_ret != 0) {
                DBG(L(TEST), "rdma_resolve_addr error: %s", strerror(errno));
        }

        freeaddrinfo(addr);

        conn->context = context;

        struct request_message *req_msg = ((struct rdma_context *)context)->request_msg;

        // if (req_msg->msg_type == PAGEFAULT_EVICTED_REQUEST)
        // {
        //         DBG(L(TEST), "int rc loop, target host: %s, addr: %p", host, req_msg->addr);
        // }
        event_loop(ec, 1, req_msg); // exit on disconnect

        rdma_destroy_event_channel(ec);
}

/**
 * @description: 接收rdma连接请求，在每次断开连接之后不退出本函数而继续监听
 * @param {const char *}
 * @return {type}
 * @author: abin&xy
 * @last_editor: abin
 */
void rc_server_loop(const char *port)
{
        struct sockaddr_in6 addr;
        struct rdma_cm_id *listener = NULL;
        struct rdma_event_channel *ec = NULL;

        memset(&addr, 0, sizeof(addr));
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(atoi(port));

        // TEST_Z(ec = rdma_create_event_channel());
        ec = rdma_create_event_channel();
        if (ec == NULL) {
                DBG(L(TEST), "rdma_create_event_channel error: %s", strerror(errno));
        }
        // TEST_NZ(rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP));
        int create_id_ret = rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP);
        if (create_id_ret != 0) {
                DBG(L(TEST), "rdma_create_id error: %s", strerror(errno));
        }

        // TEST_NZ(rdma_bind_addr(listener, (struct sockaddr *)&addr));
        int bind_addr_ret = rdma_bind_addr(listener, (struct sockaddr *)&addr);
        if (bind_addr_ret != 0) {
                DBG(L(TEST), "rdma_bind_addr error: %s", strerror(errno));
                exit(0);
        }
        // TEST_NZ(rdma_listen(listener, 10));  /* backlog=10 is arbitrary */
        int listen_ret = rdma_listen(listener, 10);     /* backlog=10 is arbitrary */
        if (listen_ret != 0) {
                DBG(L(TEST), "rdma_listen error: %s", strerror(errno));
        }

        event_loop(ec, 0, NULL); // don't exit on disconnect

        rdma_destroy_id(listener);
        rdma_destroy_event_channel(ec);
}

/**
 * @description: 断开rdma连接
 * @param {struct rdma_cm_id *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rc_disconnect(struct rdma_cm_id *id)
{
        rdma_disconnect(id);
}

/**
 * @description: 打印错误信息并退出程序，未使用本函数
 * @param {char *}
 * @return {void}
 * @author: abin&xy
 * @last_editor: abin
 */
void rc_die(const char *reason)
{
        fprintf(stderr, "%s", reason);
        exit(EXIT_FAILURE);
}

struct ibv_pd *rc_get_pd()
{
        return s_ctx->pd;
}
