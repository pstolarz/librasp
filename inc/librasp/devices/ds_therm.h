/*
   Copyright (c) 2015 Piotr Stolarz
   librasp: RPi HW interface library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __LR_DEVS_DS_THERM_H__
#define __LR_DEVS_DS_THERM_H__

#include "librasp/w1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define READ_ROM            0x33
#define MATCH_ROM           0x55
#define SKIP_ROM            0xcc
#define ALARM_SEARCH        0xec
#define SEARCH_ROM          0xf0

#define CONVERT_T           0x44
#define COPY_SCRATCHPAD     0x48
#define WRITE_SCRATCHPAD    0x4e
#define RECALL_EEPROM       0xb8
#define READ_POW_SUPPLY     0xb4
#define READ_SCRATCHPAD     0xbe

/* supported DS therms (more may be added in the future) */
#define DS1822              0x22
#define DS18B20             0x28
#define DS1825              0x3b
#define DS28EA00            0x42

/* returns NULL if not supported */
#define dsth_name(id) \
    (((id)&0xff)==DS1822 ? "DS1822" : \
    (((id)&0xff)==DS18B20 ? "DS18B20" : \
    (((id)&0xff)==DS1825 ? "DS1825" : \
    (((id)&0xff)==DS28EA00 ? "DS28EA00" : \
    (NULL)))))

typedef struct _dsth_sratchpad_t
{
    uint8_t temp_lsb;
    uint8_t temp_hsb;
    uint8_t th;
    uint8_t tl;
    uint8_t cfg_reg;
    uint8_t res1;
    uint8_t res2;
    uint8_t res3;
    uint8_t crc;
} dsth_sratchpad_t;

/* Low level therm functions
 */
lr_errc_t dsth_read_pow_supply(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, bool_t *p_is_paras);
lr_errc_t dsth_convert_t(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, void *p_status, size_t stat_len);
#ifdef CONFIG_WRITE_PULLUP
lr_errc_t dsth_convert_t_with_pullup(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, unsigned int pullup);
#endif
lr_errc_t dsth_read_scratchpad(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_sratchpad_t *p_scpd);
lr_errc_t dsth_write_scratchpad(w1_hndl_t *p_w1_h,
    w1_slave_id_t therm, uint8_t th, uint8_t tl, uint8_t cfg_reg);
lr_errc_t dsth_copy_scratchpad(w1_hndl_t *p_w1_h, w1_slave_id_t therm);
lr_errc_t dsth_recall_eeprom(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, uint8_t *p_status);

typedef enum _dsth_res_t {
    DSTH_RES_9BIT =0,
    DSTH_RES_10BIT,
    DSTH_RES_11BIT,
    DSTH_RES_12BIT
} dsth_res_t;

#define DSTH_CONV_T_12BIT   750U
#define dsth_get_conv_time(res) \
    (DSTH_CONV_T_12BIT >> (3-(((unsigned int)(res))%4)))

/* Get/set therm resolution */
lr_errc_t dsth_get_res(w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_res_t *p_res);
lr_errc_t dsth_set_res(w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_res_t res);

/* Probe the temperature and write it under 'p_temp' (x1000).
 */
lr_errc_t dsth_probe(w1_hndl_t *p_w1_h, w1_slave_id_t therm, int *p_temp);

#ifdef __cplusplus
}
#endif

#endif /* __LR_DEVS_DS_THERM_H__ */