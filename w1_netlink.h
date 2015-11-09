/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __W1_NETLINK_H__
#define __W1_NETLINK_H__

#include <linux/netlink.h>

#define MAX_W1_NETLINK_MSG_SZ   (0x1000+NLMSG_HDRLEN)

enum w1_netlink_message_types {
    /* notifications */
    W1_SLAVE_ADD = 0,
    W1_SLAVE_REMOVE,
    W1_MASTER_ADD,
    W1_MASTER_REMOVE,
    /* commands */
    W1_MASTER_CMD,
    W1_SLAVE_CMD,
    /* root command */
    W1_LIST_MASTERS
};

enum w1_commands {
    W1_CMD_READ = 0,
    W1_CMD_WRITE,
    W1_CMD_SEARCH,
    W1_CMD_ALARM_SEARCH,
    W1_CMD_TOUCH,
    W1_CMD_RESET
#ifdef CONFIG_WRITE_PULLUP
    ,W1_CMD_WRITE_PULLUP=20
#endif
};

struct w1_netlink_msg
{
    uint8_t type;
    uint8_t status;
    uint16_t len;
    union {
        uint8_t id[8];
        struct w1_mst {
            uint32_t id;
            uint32_t res;
        } mst;
    } id;
    uint8_t data[0];
};

struct w1_netlink_cmd
{
    uint8_t cmd;
    uint8_t res;
    uint16_t len;
    uint8_t data[0];
};

#endif /* __W1_NETLINK_H__ */
