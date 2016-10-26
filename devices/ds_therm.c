/*
   Copyright (c) 2015,2016 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <string.h>
#include <unistd.h>
#include "common.h"
#include "librasp/devices/ds_therm.h"

/* CRC8 calculation */
static uint8_t crc8(const void *p_in, size_t len)
{
    size_t i;
    uint8_t b, crc=0;

    while (len--) {
        b = *((uint8_t*)p_in++);
        for (i=8; i; i--, b>>=1)
            crc=((crc^b)&1 ? 0x8c : 0)^(crc>>1);
    }
    return crc;
}

/* Execute a command for a given slave 'slv'. Bus probing may be performed after
   the execution.
 */
static lr_errc_t exec_cmd(w1_hndl_t *p_w1_h, w1_slave_id_t slv,
    const void *p_cmd, size_t cmd_len, void *p_out, size_t out_len)
{
    lr_errc_t ret=LREC_SUCCESS;
    uint8_t cmd_buf[32];

    uint8_t msg_buf[get_w1_cmds_bufsz(1)];
    w1_msg_t *p_msg = (w1_msg_t*)msg_buf;

    w1_slave_msg_init(p_msg, slv, 1);

    memcpy(&cmd_buf[0], p_cmd, cmd_len);
    if (out_len>0) memset(&cmd_buf[cmd_len], 0xff, out_len);

    add_touch_w1_cmd(p_msg, cmd_buf, cmd_len+out_len);

    EXEC_RG(w1_msg_exec(p_w1_h, p_msg));
    if (out_len>0) memcpy(p_out, &cmd_buf[cmd_len], out_len);
finish:
    return ret;
}

#ifdef CONFIG_WRITE_PULLUP
/* Execute 1-byte length command 'cmd' (w/o following args) for a given slave
   'slv' with strong pull-up time following the execution. Due to the command
   character no bus probing is performed.
 */
static lr_errc_t exec_cmd_pullup(
    w1_hndl_t *p_w1_h, w1_slave_id_t slv, uint8_t cmd, unsigned int pullup)
{
    uint8_t cmd_buf[8];
    uint8_t msg_buf[get_w1_cmds_bufsz(1)];
    w1_msg_t *p_msg = (w1_msg_t*)msg_buf;

    w1_slave_msg_init(p_msg, slv, 1);

    cmd_buf[0] = cmd;
    add_write_pullup_w1_cmd(p_msg, cmd_buf, 1, pullup);

    return w1_msg_exec(p_w1_h, p_msg);
}
#endif

/* Execute a command for all slaves connected to a 'master' bus. Due to the
   command character no bus probing is performed. Optional strong pull-up time
   may be requested for the command execution.
 */
static lr_errc_t exec_cmd_all(w1_hndl_t *p_w1_h, w1_master_id_t master,
    const void *p_cmd, size_t cmd_len, bool_t has_pullup, unsigned int pullup)
{
    lr_errc_t ret=LREC_SUCCESS;
    uint8_t cmd_buf[32];

    uint8_t msg_buf[get_w1_cmds_bufsz(3)];
    w1_msg_t *p_msg = (w1_msg_t*)msg_buf;

    w1_master_msg_init(p_msg, master, 3);

    add_bus_reset_w1_cmd(p_msg);

    /* SKIP_ROM addresses - all slaves */
    cmd_buf[0] = SKIP_ROM;
    add_write_w1_cmd(p_msg, &cmd_buf[0], 1);

    memcpy(&cmd_buf[1], p_cmd, cmd_len);

#ifdef CONFIG_WRITE_PULLUP
    if (has_pullup) {
        add_write_pullup_w1_cmd(p_msg, &cmd_buf[1], cmd_len+1, pullup);
    } else
#endif
        add_write_w1_cmd(p_msg, &cmd_buf[1], cmd_len+1);

    EXEC_RG(w1_msg_exec(p_w1_h, p_msg));
finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t dsth_read_pow_supply(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, bool_t *p_is_paras)
{
    lr_errc_t ret=LREC_SUCCESS;
    uint8_t cmd=READ_POW_SUPPLY, resp;

    EXEC_RG(exec_cmd(p_w1_h, therm, &cmd, 1, &resp, 1));
    *p_is_paras = !(resp&1);
finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t dsth_convert_t(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, void *p_status, size_t stat_len)
{
    uint8_t cmd=CONVERT_T;
    return exec_cmd(
        p_w1_h, therm, &cmd, 1, (stat_len ? p_status : NULL), stat_len);
}

/* exported; see header for details */
lr_errc_t dsth_convert_t_with_pullup(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, unsigned int pullup)
{
#ifdef CONFIG_WRITE_PULLUP
    return exec_cmd_pullup(p_w1_h, therm, CONVERT_T, pullup);
#else
    return LREC_NOT_SUPP;
#endif
}

/* exported; see header for details */
lr_errc_t dsth_read_scratchpad(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_scratchpad_t *p_scpd)
{
    lr_errc_t ret=LREC_SUCCESS;
    uint8_t cmd=READ_SCRATCHPAD;

    EXEC_RG(exec_cmd(p_w1_h, therm, &cmd, 1, p_scpd, sizeof(*p_scpd)));
    if (crc8(p_scpd, (size_t)(uint8_t*)&(((dsth_scratchpad_t*)0)->crc)) !=
        p_scpd->crc)
    {
        err_printf("[%s] Scratchpad CRC doesn't match\n", __func__);
        ret=LREC_DTA_CRPT;
    }
finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t dsth_write_scratchpad(w1_hndl_t *p_w1_h,
    w1_slave_id_t therm, uint8_t th, uint8_t tl, uint8_t cfg_reg)
{
    uint8_t cmd[4];
    size_t cmd_len = sizeof(cmd);

    cmd[0]=WRITE_SCRATCHPAD;
    cmd[1]=th;
    cmd[2]=tl;

    if (dsth_get_family(therm) != DS18S20)
        cmd[3]=cfg_reg;
    else
        cmd_len--;

    return exec_cmd(p_w1_h, therm, cmd, cmd_len, NULL, 0);
}

/* exported; see header for details */
lr_errc_t dsth_copy_scratchpad(w1_hndl_t *p_w1_h, w1_slave_id_t therm)
{
    uint8_t cmd=COPY_SCRATCHPAD;
    return exec_cmd(p_w1_h, therm, &cmd, 1, NULL, 0);
}

/* exported; see header for details */
lr_errc_t dsth_copy_scratchpad_with_pullup(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, unsigned int pullup)
{
#ifdef CONFIG_WRITE_PULLUP
    return exec_cmd_pullup(p_w1_h, therm, COPY_SCRATCHPAD, pullup);
#else
    return LREC_NOT_SUPP;
#endif
}

/* exported; see header for details */
lr_errc_t dsth_recall_eeprom(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, uint8_t *p_status)
{
    uint8_t cmd=RECALL_EEPROM;
    return exec_cmd(p_w1_h, therm, &cmd, 1, p_status, (p_status ? 1 : 0));
}

/* exported; see header for details */
lr_errc_t dsth_convert_t_all(w1_hndl_t *p_w1_h, w1_master_id_t master)
{
    uint8_t cmd=CONVERT_T;
    return exec_cmd_all(p_w1_h, master, &cmd, 1, FALSE, 0);
}

/* exported; see header for details */
lr_errc_t dsth_convert_t_with_pullup_all(
    w1_hndl_t *p_w1_h, w1_master_id_t master, unsigned int pullup)
{
#ifdef CONFIG_WRITE_PULLUP
    uint8_t cmd=CONVERT_T;
    return exec_cmd_all(p_w1_h, master, &cmd, 1, TRUE, pullup);
#else
    return LREC_NOT_SUPP;
#endif
}

/* exported; see header for details */
lr_errc_t dsth_write_scratchpad_all(w1_hndl_t *p_w1_h,
    w1_master_id_t master, uint8_t th, uint8_t tl, uint8_t cfg_reg)
{
    uint8_t cmd[4];
    cmd[0]=WRITE_SCRATCHPAD;
    cmd[1]=th;
    cmd[2]=tl;
    cmd[3]=cfg_reg;
    return exec_cmd_all(p_w1_h, master, cmd, sizeof(cmd), FALSE, 0);
}

/* exported; see header for details */
lr_errc_t dsth_write_scratchpad_all_ds18s20(
    w1_hndl_t *p_w1_h, w1_master_id_t master, uint8_t th, uint8_t tl)
{
    uint8_t cmd[3];
    cmd[0]=WRITE_SCRATCHPAD;
    cmd[1]=th;
    cmd[2]=tl;
    return exec_cmd_all(p_w1_h, master, cmd, sizeof(cmd), FALSE, 0);
}

/* exported; see header for details */
lr_errc_t dsth_copy_scratchpad_all(w1_hndl_t *p_w1_h, w1_master_id_t master)
{
    uint8_t cmd=COPY_SCRATCHPAD;
    return exec_cmd_all(p_w1_h, master, &cmd, 1, FALSE, 0);
}

/* exported; see header for details */
lr_errc_t dsth_copy_scratchpad_with_pullup_all(
    w1_hndl_t *p_w1_h, w1_master_id_t master, unsigned int pullup)
{
#ifdef CONFIG_WRITE_PULLUP
    uint8_t cmd=COPY_SCRATCHPAD;
    return exec_cmd_all(p_w1_h, master, &cmd, 1, TRUE, pullup);
#else
    return LREC_NOT_SUPP;
#endif
}

/* exported; see header for details */
lr_errc_t dsth_recall_eeprom_all(w1_hndl_t *p_w1_h, w1_master_id_t master)
{
    uint8_t cmd=RECALL_EEPROM;
    return exec_cmd_all(p_w1_h, master, &cmd, 1, FALSE, 0);
}

/* exported; see header for details */
lr_errc_t dsth_get_res(w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_res_t *p_res)
{
    lr_errc_t ret=LREC_SUCCESS;
    dsth_scratchpad_t scpd;

    if (dsth_get_family(therm) != DS18S20) {
        EXEC_RG(dsth_read_scratchpad(p_w1_h, therm, &scpd));
        *p_res = (dsth_res_t)((scpd.cfg_reg>>5)&3);
    } else
        *p_res = DSTH_RES_12BIT;
finish:
    return ret;
}

/* exported; see header for details */
lr_errc_t dsth_set_res(w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_res_t res)
{
    lr_errc_t ret=LREC_SUCCESS;
    dsth_scratchpad_t scpd;

    if (dsth_get_family(therm) != DS18S20) {
        EXEC_RG(dsth_read_scratchpad(p_w1_h, therm, &scpd));
        ret = dsth_write_scratchpad(p_w1_h, therm, scpd.th, scpd.tl,
            SET_BITFLD(scpd.cfg_reg, ((uint8_t)res&3)<<5, (uint8_t)3<<5));
    } else
    if (res!=DSTH_RES_12BIT) ret=LREC_DEV_ERR;

finish:
    return ret;
}

/* exported; see header for details */
int dsth_get_temp_scratchpad(
    w1_slave_id_t therm, const dsth_scratchpad_t *p_scpd)
{
    int temp = ((int)(int8_t)p_scpd->temp_hsb << 8) | p_scpd->temp_lsb;

    if (dsth_get_family(therm) != DS18S20) {
        unsigned int res = (p_scpd->cfg_reg>>5) & 3;
        temp = (temp>>(3-res))<<(3-res);  /* zeroed undefined bits */
        temp = (temp*1000)/16;
    } else
    if (p_scpd->cnt_per_c) {
        temp = 1000*(temp>>1) - 250;
        temp += 1000*(p_scpd->cnt_per_c - p_scpd->cnt_rem) / p_scpd->cnt_per_c;
    } else {
        /* shall never happen */
        temp = (temp*1000)/2;
        warn_printf("[%s] Zeroed COUNT_PER_C detected!\n", __func__);
    }

    return temp;
}

/* exported; see header for details */
lr_errc_t dsth_probe(w1_hndl_t *p_w1_h, w1_slave_id_t therm, int *p_temp)
{
    lr_errc_t ret=LREC_SUCCESS;
    dsth_res_t res;
    unsigned int conv_time;
    dsth_scratchpad_t scpd;
#ifdef CONFIG_WRITE_PULLUP
    bool_t is_paras;

    EXEC_RG(dsth_read_pow_supply(p_w1_h, therm, &is_paras));
    dbg_printf("[%s] Is parasitic power: %d\n", __func__, is_paras);
#endif

    EXEC_RG(dsth_get_res(p_w1_h, therm, &res));
    dbg_printf("[%s] DS therm resolution: %d-bit\n", __func__, (int)res+9);
    conv_time = dsth_get_conv_time(res);

#ifndef CONFIG_WRITE_PULLUP
    EXEC_RG(dsth_convert_t(p_w1_h, therm, NULL, 0));

    /* Since there is no way to lock the w1 bus and safety probe the completion
       status in the non-parasitic mode without the danger of interfering with
       other bus activities during the waiting state, always wait specified
       amount of time (depending on the temperature resolution) before reading
       the result.
     */
    usleep(conv_time*1000);
#else
    if (is_paras) {
        EXEC_RG(dsth_convert_t_with_pullup(p_w1_h, therm, conv_time));
    } else {
        EXEC_RG(dsth_convert_t(p_w1_h, therm, NULL, 0));
        usleep(conv_time*1000);
    }
#endif

    EXEC_RG(dsth_read_scratchpad(p_w1_h, therm, &scpd));
    dbg_printf("[%s] temp-lsb:0x%02x, temp-hsb:0x%02x\n",
        __func__, scpd.temp_lsb, scpd.temp_hsb);

    *p_temp = dsth_get_temp_scratchpad(therm, &scpd);
finish:
    return ret;
}
