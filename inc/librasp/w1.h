/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_W1_H__
#define __LR_W1_H__

#include "librasp/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t w1_master_id_t;
typedef uint64_t w1_slave_id_t;

typedef struct _w1_hndl_t
{
    int sock_nl;
    uint32_t seq;
} w1_hndl_t;

typedef enum _w1_msg_type_t
{
    w1_master_msg=0,
    w1_slave_msg
} w1_msg_type_t;

typedef union _w1_cmd_extra_dta_t
{
    unsigned int pullup;
} w1_cmd_extra_dta_t;

/* types of w1 commands */
typedef enum _w1_cmd_type
{
    w1_cmd_read=0,
    w1_cmd_write,
    w1_cmd_touch,
    w1_cmd_bus_reset,       /* master only */
    w1_cmd_write_pullup     /* the library must be configured to handle
                               this command type (CONFIG_WRITE_PULLUP) */
} w1_cmd_type;

typedef struct _w1_cmd_t {
    w1_cmd_type type;
    size_t len;
    uint8_t *p_data;
    /* reserved for extra data (depending on the command type) */
    w1_cmd_extra_dta_t extra;
} w1_cmd_t;

/* w1 message consists of a set of commands designated to be sent on the 1-wire
   bus controlled by a particular bus master (the master message) OR to a
   specific slave device (the slave message). The master message consists of
   lowest level link layer (L2) commands, where the sender must provide full
   1-wire frame (RESET + ROM addressing command + FUNC command followed by any
   required data exchange); whereas the slave message consists of merely FUNC
   command (plus required data exchange) and all the L2 boilerplate (RESET, ROM
   addressing) is performed by the w1 bus driver (therefore the slave message
   may be seen as a transport layer (L4) message).
   NOTE: Content of this struct should be opaque for a library user and need not
   to be accessed directly.
 */
typedef struct _w1_msg_t
{
    w1_msg_type_t type;

    union {
        w1_master_id_t mst;
        w1_slave_id_t slv;
    } id;

    unsigned int n_cmds;
    unsigned int max_cmds;

    w1_cmd_t cmds[0];
} w1_msg_t;

/* Get buffer size necessary to store list of 'n' number of w1 commands at max */
#define get_w1_cmds_bufsz(n) (sizeof(w1_msg_t)+(n)*sizeof(w1_cmd_t))

/* Initialize an empty w1 master/slave message to store 'max_cmds' w1 commands
   at max. The caller must allocate the w1_msg_t struct before the call; use
   get_w1_cmds_bufsz() macro to acquire required buffer size to store the struct.
 */
void w1_master_msg_init(
    w1_msg_t *p_w1msg, w1_master_id_t master, unsigned int max_cmds);
void w1_slave_msg_init(
    w1_msg_t *p_w1msg, w1_slave_id_t slave, unsigned int max_cmds);

/* Set master/slave id for a w1 message.
   'p_id' must point to a w1_master_id_t or w1_slave_id_t type of id, depending
   on a message type (specified during w1 message initialization). The w1 message
   type can not be changed once the message has been initialized, so id's type
   must be always the same for the same w1 message.
 */
void w1_msg_set_id(w1_msg_t *p_w1msg, const void *p_id);

/* Flush all command(s) from w1 message. The w1 message may be re-filled anew by
   add_XXX_w1_cmd() family of functions after this call.
 */
void w1_msg_flush_cmds(w1_msg_t *p_w1msg);

/* Add a w1 command to the w1 message for execution by w1_msg_exec() in order
   determined by calling order of these functions.

   NOTE: The caller must not use/modify/free passed data until w1_msg_exec()
   which uses the data for writing/reading/touching. After the w1_msg_exec()
   call, the caller may inspect the result by examining passed data buffer(s).
 */
lr_errc_t add_read_w1_cmd(w1_msg_t *p_w1msg, uint8_t *p_data, size_t len);
lr_errc_t add_write_w1_cmd(w1_msg_t *p_w1msg, uint8_t *p_data, size_t len);
lr_errc_t add_touch_w1_cmd(w1_msg_t *p_w1msg, uint8_t *p_data, size_t len);
/* for master message only */
lr_errc_t add_bus_reset_w1_cmd(w1_msg_t *p_w1msg);

/* The library must be configured to support this function (CONFIG_WRITE_PULLUP)
 */
lr_errc_t add_write_pullup_w1_cmd(
    w1_msg_t *p_w1msg, uint8_t *p_data, size_t len, unsigned int pullup);

/* Execute a w1 message. Data to be sent/retrieved are taken/written into the
   buffers passed during w1 message command(s) provisioning by add_XXX_w1_cmd()
   family of functions. The w1 bus is guaranteed to be locked during the
   execution time of all commands contained within a single w1 message.

   NOTE: The executed message pointed by 'p_w1msg' may be re-executed, if the
   "touch" command buffer(s) are re-initialized to their initial states. If
   other recipient is desired use w1_msg_set_id() to set it's id.
 */
lr_errc_t w1_msg_exec(w1_hndl_t *p_hndl, w1_msg_t *p_w1msg);

/* Free w1 handle */
void w1_free(w1_hndl_t *p_hndl);

/* Initialize w1 handle

   NOTE: The initiated handle is associated with a netlink socket, therefore
   it's feasible to have multiple w1 handlers for various logically separate
   operations.
 */
lr_errc_t w1_init(w1_hndl_t *p_hndl);

typedef struct _masters_t
{
    size_t sz;          /* num of elements in ids[] */
    size_t max_sz;      /* capacity of ids[] table */
    w1_master_id_t ids[0];
} w1_masters_t;

/* Get buffer size necessary to store list of 'n' number of master ids at max */
#define get_w1_masters_bufsz(n) (sizeof(w1_masters_t)+(n)*sizeof(w1_master_id_t))

/* Get id list of w1 bus masters and write it under 'p_masts'. The caller must
   allocate the w1_masters_t struct (use get_w1_masters_bufsz() for this case)
   and set its 'max_sz' appropriately. If not NULL 'p_n_masts' will get overall
   number of masters in the system. NOTE: this number may be larger than 'max_sz'.
 */
lr_errc_t list_w1_masters(
    w1_hndl_t *p_hndl, w1_masters_t *p_masts, size_t *p_n_masts);

typedef struct _slaves_t
{
    size_t sz;          /* num of elements in ids[] */
    size_t max_sz;      /* capacity of ids[] table */
    w1_slave_id_t ids[0];
} w1_slaves_t;

/* Get buffer size necessary to store list of 'n' number of slave ids at max */
#define get_w1_slaves_bufsz(n) (sizeof(w1_slaves_t)+(n)*sizeof(w1_slave_id_t))

/* Search for slave devices connected to a bus controlled by a 'master'.
   'p_slavs' and 'p_n_slavs' have similar meaning as 'p_masts' and 'p_n_masts'
   for list_w1_masters().
 */
lr_errc_t search_w1_slaves(w1_hndl_t *p_hndl,
    w1_master_id_t master, w1_slaves_t *p_slavs, size_t *p_n_slavs);

/* Search for slaves with alarms being set. Meaning of params is the same as
   for list_w1_slaves().
 */
lr_errc_t search_w1_alarms(w1_hndl_t *p_hndl,
    w1_master_id_t master, w1_slaves_t *p_slavs, size_t *p_n_slavs);

#ifdef __cplusplus
}
#endif

#endif /* __LR_W1_H__ */
