/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/connector.h>
#include "common.h"
#include "w1_netlink.h"
#include "librasp/w1.h"

#define DBG_PRINTF_DATA_MAX     32U

#define ACK_STAT_VAL            0xffffffffU

/* netlink/connector sequences varies in range 0..(SEQ_RANGE-1) */
#define SEQ_RANGE               (ACK_STAT_VAL-1)

#define PREP_DATA_MSG_BUF(b, m) \
    if ((m)->len>0) { \
        sprintf((b), " data[%shex bts]:", \
            (m)->len>DBG_PRINTF_DATA_MAX ? "trunc " : ""); \
        bts2hex((m)->data, MIN((m)->len, \
            DBG_PRINTF_DATA_MAX), (b)+strlen((b))); \
    }

#define W1CMD_OK(c,l)   ((l)>=(sizeof(struct w1_netlink_cmd)+((c)->len)))
#define W1CMD_NEXT(c,l) ((l)-=(sizeof(struct w1_netlink_cmd)+((c)->len)), \
    (struct w1_netlink_cmd*)(((uint8_t*)((c)+1))+((c)->len)))

#define IS_RESP_REQUIRED(t) ((t)==w1_cmd_read || (t)==w1_cmd_touch)


/* Convert command type into w1 netlink type */
static uint8_t convert_cmd_type(w1_cmd_type tpy)
{
    uint8_t ret=(uint8_t)-1;
    switch (tpy)
    {
    case w1_cmd_read:
        ret=W1_CMD_READ;
        break;
    case w1_cmd_write:
        ret=W1_CMD_WRITE;
        break;
    case w1_cmd_touch:
        ret=W1_CMD_TOUCH;
        break;
    case w1_cmd_bus_reset:
        ret=W1_CMD_RESET;
        break;
    case w1_cmd_write_pullup:
#ifdef CONFIG_WRITE_PULLUP
        ret=W1_CMD_WRITE_PULLUP;
#endif
        break;
    }
    return ret;
}

typedef enum _w1msg_more_stat_t
{
    w1msg_no_more=0,    /* no more messages required */
    w1msg_more_req,     /* more message(s) required */
    w1msg_more_opt      /* more message(s) may be provided but not required */
} w1msg_more_stat_t;

/* The callback function prototype called by recv_w1msg() for each received w1
   message.

   p_cb_priv_dta: private data passed by recv_w1msg() untouched,

   p_w1msg: received w1 message; it's guaranteed the message corresponds to
       the request message, therefore the callback need not to check the message
       against such correctness,

   is_status: If TRUE the message contains execution status of the sent message.
       Due to the COMPLETE MESS in the w1 netlink kernel code, some messages
       are acknowledged by their statuses only if an error occurs (like
       W1_LIST_MASTERS), for other the status is always reported. Moreover
       the behaviour is kernel specific and for some kernels (e.g. =<3.12)
       the message status is always sent regardless of an error or success
       (which is documentation compliant BTW). To handle this mess it seems
       to be feasible to assume success of the command unless the status
       will be reported, in which case there is a need to check if it is an
       error or not.

   Callback function returns "more message(s)" status (w1msg_more_stat_t).
 */
typedef w1msg_more_stat_t (*recv_w1msg_cb_t)(
    void *p_cb_priv_dta, const struct w1_netlink_msg *p_w1msg, bool_t is_status);

/* Receive w1 response(s) corresponding to a request 'p_req' and callback them
   by recv_cb().
 */
static lr_errc_t recv_w1msg(w1_hndl_t *p_hndl,
    const struct cn_msg *p_req, recv_w1msg_cb_t recv_cb, void *p_cb_priv_dta)
{
    lr_errc_t ret=LREC_SUCCESS;

    w1msg_more_stat_t more_stat;
    uint8_t recv_buf[MAX_W1_NETLINK_MSG_SZ];
    unsigned int n_rcv_msgs, n_pyld_msgs;
    ssize_t dta_len;
    size_t offs;

    uint32_t *p_ack;
    struct nlmsghdr *p_nlmsg;
    struct cn_msg *p_cnmsg;
    struct w1_netlink_msg *p_w1msg, *p_w1req;
    struct pollfd fds;

    if (p_req->len <= 0) {
        /* no response for no request */
        goto finish;
    }
    p_w1req = (struct w1_netlink_msg*)(p_req->data);

    more_stat = w1msg_more_req;

    fds.fd = p_hndl->sock_nl;
    fds.events = POLLIN;
    fds.revents = 0;

    for (n_rcv_msgs=0;; n_rcv_msgs++)
    {
        switch(poll(&fds, 1, (more_stat==w1msg_more_req ? 1000 :
            (more_stat==w1msg_more_opt ? 50 : 1))))
        {
        case 0:
            if (!n_rcv_msgs) {
                err_printf(
                    "[%s] No response on the w1 netlink connector received. Make "
                    "sure the \"wire\" kernel module is loaded with the netlink "
                    "support compiled!\n", __func__);
                ret=LREC_NO_RESP;
                goto finish;
            } else
            if (more_stat==w1msg_more_req) {
                err_printf("[%s] Response expected but not received; timeout\n",
                    __func__);
                ret=LREC_NO_RESP;
                goto finish;
            }
            goto break_recv_loop;
        case -1:
            err_printf("[%s] poll() on the w1 netlink connector error %d; %s\n",
                __func__, errno, strerror(errno));
            ret=LREC_COMM_ERR;
            goto finish;
        }

        /* get the message */
        dta_len = recv(p_hndl->sock_nl, recv_buf, sizeof(recv_buf), 0);
        if (dta_len<0) {
            err_printf("[%s] recv() on the w1 netlink connector error %d; %s\n",
                __func__, errno, strerror(errno));
            ret=LREC_COMM_ERR;
            goto finish;
        }

        /* parse netlink msgs chain */
        for (p_nlmsg=(struct nlmsghdr*)recv_buf, n_pyld_msgs=0, offs=0;
            NLMSG_OK(p_nlmsg, dta_len);
            n_pyld_msgs++)
        {
            struct nlmsghdr nlmsg = *p_nlmsg;

            dbg_printf("[NLSCK:%d]: %s [header]: len:0x%08x type:0x%04x "
                "flags:0x%04x seq:0x%08x pid:0x%08x\n", p_hndl->sock_nl,
                (!n_pyld_msgs ? "<- netlink" : "   netlink"), nlmsg.nlmsg_len,
                nlmsg.nlmsg_type, nlmsg.nlmsg_flags, nlmsg.nlmsg_seq,
                nlmsg.nlmsg_pid);

            switch (nlmsg.nlmsg_type)
            {
            case NLMSG_NOOP:
                continue;
            case NLMSG_ERROR:
                err_printf("[%s] Netlink error message received\n", __func__);
                ret=LREC_PROTO_ERR;
                goto finish;
            }

            if (nlmsg.nlmsg_type==NLMSG_DONE || (nlmsg.nlmsg_flags & NLM_F_MULTI))
            {
                if (nlmsg.nlmsg_len > NLMSG_HDRLEN) {
                    memcpy(&recv_buf[offs],
                        NLMSG_DATA(p_nlmsg), nlmsg.nlmsg_len-NLMSG_HDRLEN);
                    offs += (nlmsg.nlmsg_len-NLMSG_HDRLEN);
                }
                if (nlmsg.nlmsg_type==NLMSG_DONE) break;
            } else {
                err_printf("[%s] Unexpected netlink message\n", __func__);
                ret=LREC_BAD_MSG;
                goto finish;
            }

            /* move to the next nlmsg */
            dta_len -= NLMSG_ALIGN(nlmsg.nlmsg_len);
            p_nlmsg = (struct nlmsghdr*)
                (((uint8_t*)p_nlmsg)+NLMSG_ALIGN(nlmsg.nlmsg_len));
        }

#define __CNMSG_OK(c,l) \
    ((l)>=(sizeof(struct cn_msg)+((c)->len)))

#define __ACK_W1MSG_INIT(a,w,b) \
    ((a)=(uint32_t*)(b), (w)=(struct w1_netlink_msg*)((a)+1))

#define __ACK_W1MSG_OK(w,l) \
    ((l)>=(sizeof(uint32_t)+sizeof(struct w1_netlink_msg)+((w)->len)))

#define __ACK_W1MSG_NEXT(a,w,l) \
    ((l)-=(sizeof(uint32_t)+sizeof(struct w1_netlink_msg)+((w)->len)), \
    (a)=(uint32_t*)(((uint8_t*)((a)+1))+sizeof(struct w1_netlink_msg)+((w)->len)), \
    (w)=(struct w1_netlink_msg*)(((uint8_t*)((w)+1))+((w)->len)+sizeof(uint32_t)))

        /* parse & filter out connector msgs chain */
        for (p_cnmsg=(struct cn_msg*)recv_buf,
                dta_len=(ssize_t)offs, n_pyld_msgs=0, offs=0;
            __CNMSG_OK(p_cnmsg, dta_len);
            n_pyld_msgs++)
        {
            int fltr=0;
            struct cn_msg cnmsg = *p_cnmsg;

            if (cnmsg.id.idx!=CN_W1_IDX || cnmsg.id.val!=CN_W1_VAL) {
                /* no w1 subsystem message */
                fltr=1;
            } else
            if (cnmsg.seq!=p_req->seq) {
                /* request/response sequences not match */
                fltr=2;
            }

            if (get_librasp_log_level()<=LRLOG_DEBUG)
            {
                char fltr_msg[32] = "";

                if (fltr) sprintf(fltr_msg, "; filtered out: %s",
                    (fltr==1 ? "no w1 msg" : "req/resp seqs not match"));

                dbg_printf("[NLSCK:%d]:    nest. cn_msg [header]: id:0x%08x|0x%08x "
                    "seq:0x%08x ack:0x%08x len:0x%04x flags:0x%04x%s\n",
                    p_hndl->sock_nl, cnmsg.id.idx, cnmsg.id.val, cnmsg.seq,
                    cnmsg.ack, cnmsg.len, cnmsg.flags, fltr_msg);
            }

            if (!fltr && cnmsg.len>0) {
                *(uint32_t*)&recv_buf[offs] = cnmsg.ack;
                offs += sizeof(uint32_t);
                memcpy(&recv_buf[offs], p_cnmsg->data, cnmsg.len);
                offs += cnmsg.len;
            }

            /* move to the next cn_msg */
            dta_len -= sizeof(struct cn_msg)+cnmsg.len;
            p_cnmsg = (struct cn_msg*)(((uint8_t*)(p_cnmsg+1))+cnmsg.len);
        }

        /* parse & filter out w1 msgs chain */
        for (__ACK_W1MSG_INIT(p_ack, p_w1msg, recv_buf),
                dta_len=(ssize_t)offs, n_pyld_msgs=0;
            __ACK_W1MSG_OK(p_w1msg, dta_len);
            __ACK_W1MSG_NEXT(p_ack, p_w1msg, dta_len), n_pyld_msgs++)
        {
            int fltr=0;

            if (p_w1msg->type!=p_w1req->type) {
                /* request/response types not match */
                fltr=1;
            } else
            if ((p_w1req->type==W1_MASTER_CMD &&
                    p_w1msg->id.mst.id!=p_w1req->id.mst.id) ||
                (p_w1req->type==W1_SLAVE_CMD &&
                    memcmp(&p_w1msg->id, &p_w1req->id, sizeof(p_w1msg->id))))
            {
                /* request/response ids not match */
                fltr=2;
            }

            if (get_librasp_log_level()<=LRLOG_DEBUG)
            {
                char fltr_msg[32] = "";
                char data_msg[2*DBG_PRINTF_DATA_MAX+32] = "";

                PREP_DATA_MSG_BUF(data_msg, p_w1msg);
                if (fltr) {
                    sprintf(fltr_msg, "; filtered out: req/resp %s not match",
                        (fltr==1 ? "types" : "ids"));
                }
                dbg_printf("[NLSCK:%d]:    nest. w1_msg [header]: type:0x%02x "
                    "status:0x%02x len:0x%04x id:0x%08x|0x%08x%s%s\n",
                    p_hndl->sock_nl, p_w1msg->type, p_w1msg->status, p_w1msg->len,
                    p_w1msg->id.mst.id, p_w1msg->id.mst.res, data_msg, fltr_msg);
            }

            if (!fltr && more_stat!=w1msg_no_more) {
                /* callback the caller for processing the received frame */
                more_stat = recv_cb(p_cb_priv_dta, p_w1msg,
                    (*p_ack==ACK_STAT_VAL ? TRUE : FALSE));
            }
        }
#undef __CNMSG_OK
#undef __ACK_W1MSG_INIT
#undef __ACK_W1MSG_OK
#undef __ACK_W1MSG_NEXT
    }
break_recv_loop:

finish:
    return ret;
}

/* Send a single w1 netlink message */
static lr_errc_t send_recv_w1msg(w1_hndl_t *p_hndl,
    const struct w1_netlink_msg *p_w1msg, recv_w1msg_cb_t recv_cb,
    void *p_cb_priv_dta)
{
    lr_errc_t ret=LREC_SUCCESS;
    uint8_t send_buf[MAX_W1_NETLINK_MSG_SZ];

    struct cn_msg *p_cnmsg;
    struct nlmsghdr *p_nlmsg;
    size_t w1msg_len, nlmsg_len;

    w1msg_len = sizeof(struct w1_netlink_msg)+p_w1msg->len;
    nlmsg_len = NLMSG_LENGTH(sizeof(struct cn_msg)+w1msg_len);

    if (NLMSG_ALIGN(nlmsg_len) > sizeof(send_buf)) {
        err_printf("[%s] w1 message too long (%d vs %d)\n",
            __func__, NLMSG_ALIGN(nlmsg_len), sizeof(send_buf));
        ret=LREC_MSG_SIZE;
        goto finish;
    }

    memset(send_buf, 0, NLMSG_ALIGN(nlmsg_len));
    p_nlmsg = (struct nlmsghdr*)send_buf;

    p_nlmsg->nlmsg_len = nlmsg_len;
    p_nlmsg->nlmsg_type = NLMSG_DONE;   /* single netlink msg */
    p_nlmsg->nlmsg_flags = 0;
    p_nlmsg->nlmsg_seq = (p_hndl->seq++)%SEQ_RANGE;
    p_nlmsg->nlmsg_pid = 0;             /* send to the kernel */

    p_cnmsg = (struct cn_msg*)NLMSG_DATA(p_nlmsg);

    p_cnmsg->id.idx = CN_W1_IDX;
    p_cnmsg->id.val = CN_W1_VAL;
    p_cnmsg->seq = p_nlmsg->nlmsg_seq;
    /* specific ack value used to distinguish status msg from response msgs */
    p_cnmsg->ack = ACK_STAT_VAL;
    p_cnmsg->len = w1msg_len;
    memcpy(p_cnmsg->data, p_w1msg, w1msg_len);

    if (send(p_hndl->sock_nl, p_nlmsg, NLMSG_ALIGN(nlmsg_len), 0)==-1) {
        err_printf("[%s] send() on the netlink socket error %d; %s\n",
            __func__, errno, strerror(errno));
        ret=LREC_COMM_ERR;
        goto finish;
    }

    if (get_librasp_log_level()<=LRLOG_DEBUG)
    {
        char data_msg[2*DBG_PRINTF_DATA_MAX+32] = "";

        PREP_DATA_MSG_BUF(data_msg, p_w1msg);
        dbg_printf("[NLSCK:%d]: -> netlink [header]: len:0x%08x type:0x%04x "
            "flags:0x%04x seq:0x%08x pid:0x%08x\n", p_hndl->sock_nl,
            p_nlmsg->nlmsg_len, p_nlmsg->nlmsg_type, p_nlmsg->nlmsg_flags,
            p_nlmsg->nlmsg_seq, p_nlmsg->nlmsg_pid);

        dbg_printf("[NLSCK:%d]:    nest. cn_msg [header]: id:0x%08x|0x%08x "
            "seq:0x%08x ack:0x%08x len:0x%04x flags:0x%04x\n", p_hndl->sock_nl,
            p_cnmsg->id.idx, p_cnmsg->id.val, p_cnmsg->seq, p_cnmsg->ack,
            p_cnmsg->len, p_cnmsg->flags);

        dbg_printf("[NLSCK:%d]:    nest. w1_msg [header]: type:0x%02x "
            "status:0x%02x len:0x%04x id:0x%08x|0x%08x%s\n", p_hndl->sock_nl,
            p_w1msg->type, p_w1msg->status, p_w1msg->len, p_w1msg->id.mst.id,
            p_w1msg->id.mst.res, data_msg);
    }

    ret = recv_w1msg(p_hndl, p_cnmsg, recv_cb, p_cb_priv_dta);
finish:
    return ret;
}

/* exported; see header for details */
void w1_free(w1_hndl_t *p_hndl)
{
    if (p_hndl->sock_nl != -1) {
        close(p_hndl->sock_nl);
        p_hndl->sock_nl = -1;
    }
}

/* exported; see header for details */
lr_errc_t w1_init(w1_hndl_t *p_hndl)
{
    lr_errc_t ret=LREC_SUCCESS;
    struct sockaddr_nl saddr_nl;

    memset(p_hndl, 0, sizeof(*p_hndl));
    p_hndl->sock_nl = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (p_hndl->sock_nl==-1) {
        err_printf("[%s] socket() for the netlink connector failed: %d; %s\n",
            __func__, errno, strerror(errno));
        ret=LREC_COMM_ERR;
        goto finish;
    }

    memset(&saddr_nl, 0, sizeof(saddr_nl));
    saddr_nl.nl_family = AF_NETLINK;
    saddr_nl.nl_pid = 0;            /* let the system choose unique port id */
    saddr_nl.nl_groups = -1;

    if (bind(p_hndl->sock_nl, (struct sockaddr*)&saddr_nl, sizeof(saddr_nl))==-1)
    {
        err_printf("[%s] bind() for the netlink connector failed: %d; %s\n",
            __func__, errno, strerror(errno));
        ret=LREC_COMM_ERR;
        goto finish;
    }

finish:
    if (ret!=LREC_SUCCESS) w1_free(p_hndl);
    return ret;
}

typedef struct _list_cb_dta_t
{
    /* in: size of a single list element (in bytes) */
    size_t elem_sz;
    /* out; pointer to receiving list buffer; must be initialized to the out buf */
    void *p_list;
    /* in; capacity of the list buffer in number of its elements */
    size_t max_elems;
    /* out; number of list's elements written to it; must be initialized to 0 */
    size_t n_elems;
    /* out; overall number of received list's elems; must be initialized to 0 */
    size_t n_recv_elems;
    /* out; must be initialized to 0 */
    uint8_t status;
    /* in: search command used for listing slaves (ignored for masters) */
    uint8_t srch_cmd;
} list_cb_dta_t;

#define __COPY_LIST_ELEMS(m) \
    if (n_recv_elems>0 && (p_cb_dta->n_elems < p_cb_dta->max_elems)) { \
        size_t list_offs = p_cb_dta->elem_sz*p_cb_dta->n_elems; \
        size_t n_copy_elems = \
            MIN(n_recv_elems, p_cb_dta->max_elems-p_cb_dta->n_elems); \
        memcpy((uint8_t*)p_cb_dta->p_list+list_offs, \
            (m)->data, p_cb_dta->elem_sz*n_copy_elems); \
        p_cb_dta->n_elems += n_copy_elems; \
    }

/* List masters callback */
static w1msg_more_stat_t list_masts_cb(
    void *p_cb_priv_dta, const struct w1_netlink_msg *p_w1msg, bool_t is_status)
{
    w1msg_more_stat_t ret = w1msg_more_opt;
    list_cb_dta_t *p_cb_dta = (list_cb_dta_t*)p_cb_priv_dta;
    size_t n_recv_elems = p_w1msg->len/p_cb_dta->elem_sz;

    if (is_status) {
        ret = w1msg_no_more;
        if ((p_cb_dta->status=p_w1msg->status)!=0) goto finish;
    }

    p_cb_dta->n_recv_elems += n_recv_elems;

    __COPY_LIST_ELEMS(p_w1msg);

finish:
    return ret;
}

typedef list_cb_dta_t search_cb_dta_t;

/* search slaves callback */
static w1msg_more_stat_t search_slavs_cb(
    void *p_cb_priv_dta, const struct w1_netlink_msg *p_w1msg, bool_t is_status)
{
    w1msg_more_stat_t ret = w1msg_more_opt;
    search_cb_dta_t *p_cb_dta = (search_cb_dta_t*)p_cb_priv_dta;
    struct w1_netlink_cmd *p_w1cmd = (struct w1_netlink_cmd*)p_w1msg->data;
    size_t dta_len = (size_t)p_w1msg->len;

    if (is_status) {
        ret = w1msg_no_more;
        if ((p_cb_dta->status=p_w1msg->status)!=0) goto finish;
    }

    for (; W1CMD_OK(p_w1cmd, dta_len); p_w1cmd=W1CMD_NEXT(p_w1cmd, dta_len))
    {
        size_t n_recv_elems;

        if (p_w1cmd->cmd!=p_cb_dta->srch_cmd)
        {
            p_cb_dta->status = -ENOMSG;
            ret = w1msg_no_more;

            err_printf("[%s] w1 command [%d] not match searching command [%d]\n",
                __func__, p_w1cmd->cmd, p_cb_dta->srch_cmd);
            goto finish;
        }

        n_recv_elems = p_w1cmd->len/p_cb_dta->elem_sz;
        p_cb_dta->n_recv_elems += n_recv_elems;

        __COPY_LIST_ELEMS(p_w1cmd);
    }

finish:
    return ret;
}

#undef __COPY_LIST_ELEMS

typedef struct _w1msg_exec_cb_dta_t
{
    /* out; num of processed commands; must be initialized to 0 */
    unsigned int n_prc_cmds;
    /* out; original w1_msg_t msg; will be filled with received data */
    w1_msg_t *p_w1msg;
    /* out; must be initialized to 0 */
    uint8_t status;
    /* internal; must be initialized to 0 */
    unsigned int cmd_msgs;
} w1msg_exec_cb_dta_t;

/* w1 msg exec callback */
static w1msg_more_stat_t w1msg_exec_cb(
    void *p_cb_priv_dta, const struct w1_netlink_msg *p_w1msg, bool_t is_status)
{
    w1msg_more_stat_t ret = w1msg_more_opt;
    w1msg_exec_cb_dta_t *p_cb_dta = (w1msg_exec_cb_dta_t*)p_cb_priv_dta;
    struct w1_netlink_cmd *p_w1cmd = (struct w1_netlink_cmd*)p_w1msg->data;
    size_t dta_len = (size_t)p_w1msg->len;
    unsigned int i;

    w1_cmd_t *p_w1cmd_exec;
    uint8_t exec_cmd;

check_next_exec_cmd:
    if (p_cb_dta->n_prc_cmds >= p_cb_dta->p_w1msg->n_cmds) {
        ret = w1msg_no_more;
        goto finish;
    }
    if (is_status && p_w1msg->status) {
        p_cb_dta->status = p_w1msg->status;
        ret = w1msg_no_more;
        goto finish;
    }

    p_w1cmd_exec = &p_cb_dta->p_w1msg->cmds[p_cb_dta->n_prc_cmds];
    exec_cmd = convert_cmd_type(p_w1cmd_exec->type);

    for (; W1CMD_OK(p_w1cmd, dta_len); p_w1cmd=W1CMD_NEXT(p_w1cmd, dta_len))
    {
        if (p_w1cmd->cmd!=exec_cmd)
        {
            if (!IS_RESP_REQUIRED(p_w1cmd_exec->type) || p_cb_dta->cmd_msgs>0)
            {
                /* check the next msg in the executed
                   commands chain to match the received msg */
                p_cb_dta->cmd_msgs=0;
                p_cb_dta->n_prc_cmds++;
                goto check_next_exec_cmd;
            } else {
                p_cb_dta->status = -ENOMSG;
                ret = w1msg_no_more;

                err_printf("[%s] Exec. command no. %d replied by unexpected w1 "
                    "command [%d]\n", __func__, p_cb_dta->n_prc_cmds+1,
                    p_w1cmd->cmd);
                goto finish;
            }
        } else
            p_cb_dta->cmd_msgs++;

        /* copy the outcome data content */
        if (IS_RESP_REQUIRED(p_w1cmd_exec->type) && p_w1cmd->len>0)
        {
            if ((p_w1cmd->len != p_w1cmd_exec->len) ||
                !(p_w1cmd->data && p_w1cmd_exec->p_data))
            {
                /* shall never happen */
                p_cb_dta->status = -ENOMSG;
                ret = w1msg_no_more;

                err_printf("[%s] w1 command data content error\n", __func__);
                goto finish;
            }
            memcpy(p_w1cmd_exec->p_data, p_w1cmd->data, p_w1cmd->len);
        }
    }

    for (i=p_cb_dta->n_prc_cmds+1; i < p_cb_dta->p_w1msg->n_cmds; i++) {
        if (IS_RESP_REQUIRED(p_cb_dta->p_w1msg->cmds[i].type)) {
            ret = w1msg_more_req;
            break;
        }
    }

finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t list_w1_masters(
    w1_hndl_t *p_hndl, w1_masters_t *p_masts, size_t *p_n_masts)
{
    lr_errc_t ret=LREC_SUCCESS;
    struct w1_netlink_msg w1msg;
    list_cb_dta_t cb_dta;

    p_masts->sz = 0;

    memset(&w1msg, 0, sizeof(w1msg));
    w1msg.type = W1_LIST_MASTERS;

    memset(&cb_dta, 0, sizeof(cb_dta));
    cb_dta.elem_sz = sizeof(p_masts->ids[0]);
    cb_dta.p_list = p_masts->ids;
    cb_dta.max_elems = p_masts->max_sz;

    EXEC_RG(send_recv_w1msg(p_hndl, &w1msg, list_masts_cb, &cb_dta));

    if (cb_dta.status) {
        err_printf("[%s] w1 status error: 0x%02x\n", __func__, cb_dta.status);
        ret=LREC_PROTO_ERR;
        goto finish;
    }

    if (p_n_masts) *p_n_masts = cb_dta.n_recv_elems;
    p_masts->sz = cb_dta.n_elems;

finish:
    return ret;
}

/* Support function for searching slaves */
static lr_errc_t __search_slaves(w1_hndl_t *p_hndl, uint8_t srch_cmd,
    w1_master_id_t master, w1_slaves_t *p_slavs, size_t *p_n_slavs)
{
    lr_errc_t ret=LREC_SUCCESS;

    uint8_t w1msg_buf[
        sizeof(struct w1_netlink_msg)+sizeof(struct w1_netlink_cmd)];
    struct w1_netlink_msg *p_w1msg = (struct w1_netlink_msg*)w1msg_buf;
    struct w1_netlink_cmd *p_w1cmd = (struct w1_netlink_cmd*)(p_w1msg+1);
    search_cb_dta_t cb_dta;

    p_slavs->sz = 0;

    memset(w1msg_buf, 0, sizeof(w1msg_buf));
    p_w1msg->type = W1_MASTER_CMD;
    p_w1msg->len = sizeof(struct w1_netlink_cmd);
    p_w1msg->id.mst.id = master;
    p_w1cmd->cmd = srch_cmd;

    memset(&cb_dta, 0, sizeof(cb_dta));
    cb_dta.elem_sz = sizeof(p_slavs->ids[0]);
    cb_dta.p_list = p_slavs->ids;
    cb_dta.max_elems = p_slavs->max_sz;
    cb_dta.srch_cmd = srch_cmd;

    EXEC_RG(send_recv_w1msg(p_hndl, p_w1msg, search_slavs_cb, &cb_dta));

    if (cb_dta.status) {
        err_printf("[%s] w1 status error: 0x%02x\n", __func__, cb_dta.status);
        ret=LREC_PROTO_ERR;
        goto finish;
    }

    if (p_n_slavs) *p_n_slavs = cb_dta.n_recv_elems;
    p_slavs->sz = cb_dta.n_elems;

finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t search_w1_slaves(w1_hndl_t *p_hndl,
    w1_master_id_t master, w1_slaves_t *p_slavs, size_t *p_n_slavs)
{
    return __search_slaves(p_hndl, W1_CMD_SEARCH, master, p_slavs, p_n_slavs);
}

/* exported; see header for details */
lr_errc_t search_w1_alarms(w1_hndl_t *p_hndl,
    w1_master_id_t master, w1_slaves_t *p_slavs, size_t *p_n_slavs)
{
    return __search_slaves(
        p_hndl, W1_CMD_ALARM_SEARCH, master, p_slavs, p_n_slavs);
}

/* exported; see header for details */
void w1_msg_set_id(w1_msg_t *p_w1msg, const void *p_id)
{
    if (p_w1msg->type==w1_master_msg) {
        p_w1msg->id.mst = *(w1_master_id_t*)(p_id);
    } else {
        p_w1msg->id.slv = *(w1_slave_id_t*)(p_id);
    }
}

/* Initialize w1 msg (generic) */
static void w1_msg_init(w1_msg_t *p_w1msg,
    w1_msg_type_t type, const void *p_id, unsigned int max_cmds)
{
    memset(p_w1msg, 0, get_w1_cmds_bufsz(max_cmds));

    p_w1msg->type = type;
    w1_msg_set_id(p_w1msg, p_id);
    p_w1msg->max_cmds = max_cmds;
}

/* exported; see header for details */
void w1_master_msg_init(
    w1_msg_t *p_w1msg, w1_master_id_t master, unsigned int max_cmds)
{
    w1_msg_init(p_w1msg, w1_master_msg, &master, max_cmds);
}

/* exported; see header for details */
void w1_slave_msg_init(
    w1_msg_t *p_w1msg, w1_slave_id_t slave, unsigned int max_cmds)
{
    w1_msg_init(p_w1msg, w1_slave_msg, &slave, max_cmds);
}

/* exported; see header for details */
void w1_msg_flush_cmds(w1_msg_t *p_w1msg)
{
    p_w1msg->n_cmds = 0;
    memset(p_w1msg+1, 0, p_w1msg->max_cmds*sizeof(w1_cmd_t));
}

/* Add w1 cmd (generic) */
static lr_errc_t add_w1_cmd(w1_msg_t *p_w1msg,
    w1_cmd_type type, uint8_t *p_data, size_t len, w1_cmd_extra_dta_t extra)
{
    lr_errc_t ret=LREC_SUCCESS;
    w1_cmd_t *p_cmd = &p_w1msg->cmds[p_w1msg->n_cmds];

    if (type==w1_cmd_bus_reset && p_w1msg->type==w1_slave_msg) {
        err_printf(
            "[%s] Bus reset command may be added to a master message only\n",
            __func__);
        ret=LREC_INV_ARG;
        goto finish;
    }
    if (p_w1msg->n_cmds>=p_w1msg->max_cmds) {
        err_printf("[%s] No space in w1 message - command can not be added\n",
            __func__);
        ret=LREC_NO_SPACE;
        goto finish;
    }

    memset(p_cmd, 0, sizeof(*p_cmd));

    p_cmd->type = type;
    if (len && p_data) {
        p_cmd->len = len;
        p_cmd->p_data = p_data;
    }
    p_cmd->extra = extra;

    p_w1msg->n_cmds++;

finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t add_read_w1_cmd(w1_msg_t *p_w1msg, uint8_t *p_data, size_t len)
{
    w1_cmd_extra_dta_t extra = {};
    return add_w1_cmd(p_w1msg, w1_cmd_read, p_data, len, extra);
}

/* exported; see header for details */
lr_errc_t add_write_w1_cmd(w1_msg_t *p_w1msg, uint8_t *p_data, size_t len)
{
    w1_cmd_extra_dta_t extra = {};
    return add_w1_cmd(p_w1msg, w1_cmd_write, p_data, len, extra);
}

/* exported; see header for details */
lr_errc_t add_touch_w1_cmd(w1_msg_t *p_w1msg, uint8_t *p_data, size_t len)
{
    w1_cmd_extra_dta_t extra = {};
    return add_w1_cmd(p_w1msg, w1_cmd_touch, p_data, len, extra);
}

/* exported; see header for details */
lr_errc_t add_bus_reset_w1_cmd(w1_msg_t *p_w1msg)
{
    w1_cmd_extra_dta_t extra = {};
    return add_w1_cmd(p_w1msg, w1_cmd_bus_reset, NULL, 0, extra);
}

/* exported; see header for details */
lr_errc_t add_write_pullup_w1_cmd(
    w1_msg_t *p_w1msg, uint8_t *p_data, size_t len, unsigned int pullup)
{
#ifdef CONFIG_WRITE_PULLUP
    w1_cmd_extra_dta_t extra;

    extra.pullup = pullup;
    return add_w1_cmd(p_w1msg, w1_cmd_write_pullup, p_data, len, extra);
#else
    return LREC_NOT_SUPP;
#endif
}

/* exported; see header for details */
lr_errc_t w1_msg_exec(w1_hndl_t *p_hndl, w1_msg_t *p_w1msg)
{
    lr_errc_t ret=LREC_SUCCESS;

    size_t cmds_len, msg_len;
    unsigned int i;
    w1msg_exec_cb_dta_t cb_dta;

    uint8_t msg_buf[MAX_W1_NETLINK_MSG_SZ-NLMSG_SPACE(sizeof(struct cn_msg))];
    struct w1_netlink_msg *p_w1msg_nl = (struct w1_netlink_msg*)msg_buf;
    struct w1_netlink_cmd *p_w1cmd_nl = (struct w1_netlink_cmd*)(p_w1msg_nl+1);

    if (p_w1msg->n_cmds<=0) {
        err_printf("[%s] No commands to execute in the w1 message\n", __func__);
        ret=LREC_EMPTY;
        goto finish;
    }

    for (cmds_len=0, i=0; i<p_w1msg->n_cmds; i++) {
        cmds_len += sizeof(struct w1_netlink_cmd)+p_w1msg->cmds[i].len;
#ifdef CONFIG_WRITE_PULLUP
        if (p_w1msg->cmds[i].type==w1_cmd_write_pullup)
            cmds_len += sizeof(uint16_t);
#endif
    }
    msg_len = sizeof(struct w1_netlink_msg)+cmds_len;

    if (msg_len > sizeof(msg_buf)) {
        err_printf("[%s] w1 message too long (%d vs %d)\n",
            __func__, msg_len, sizeof(msg_buf));
        ret=LREC_MSG_SIZE;
        goto finish;
    }

    memset(msg_buf, 0, msg_len);

    p_w1msg_nl->len = cmds_len;
    if (p_w1msg->type==w1_master_msg) {
        p_w1msg_nl->type = W1_MASTER_CMD;
        p_w1msg_nl->id.mst.id = p_w1msg->id.mst;
    } else {
        p_w1msg_nl->type = W1_SLAVE_CMD;
        memcpy(p_w1msg_nl->id.id, &p_w1msg->id.slv, sizeof(p_w1msg->id.slv));
    }

    for (i=0; i<p_w1msg->n_cmds; i++)
    {
        p_w1cmd_nl->cmd = convert_cmd_type(p_w1msg->cmds[i].type);
        p_w1cmd_nl->len = p_w1msg->cmds[i].len;

#ifdef CONFIG_WRITE_PULLUP
        if (p_w1msg->cmds[i].type==w1_cmd_write_pullup)
        {
            *(uint16_t*)p_w1cmd_nl->data =
                (uint16_t)(p_w1msg->cmds[i].extra.pullup);
            if (p_w1msg->cmds[i].len>0) {
                memcpy(&p_w1cmd_nl->data[sizeof(uint16_t)],
                    p_w1msg->cmds[i].p_data, p_w1msg->cmds[i].len);
            }
            p_w1cmd_nl->len += sizeof(uint16_t);
        } else
#endif
        if ((p_w1msg->cmds[i].type==w1_cmd_write ||
            p_w1msg->cmds[i].type==w1_cmd_touch) && p_w1msg->cmds[i].len>0)
        {
            memcpy(p_w1cmd_nl->data,
                p_w1msg->cmds[i].p_data, p_w1msg->cmds[i].len);
        }

        p_w1cmd_nl=W1CMD_NEXT(p_w1cmd_nl, cmds_len);
    }

    memset(&cb_dta, 0, sizeof(cb_dta));
    cb_dta.p_w1msg = p_w1msg;

    EXEC_RG(send_recv_w1msg(p_hndl, p_w1msg_nl, w1msg_exec_cb, &cb_dta));

    if (cb_dta.status) {
        err_printf("[%s] w1 status error: 0x%02x\n", __func__, cb_dta.status);
        ret=LREC_PROTO_ERR;
        goto finish;
    }

finish:
    return ret;
}
