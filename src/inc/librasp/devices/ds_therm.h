/*
   Copyright (c) 2015,2016 Piotr Stolarz
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

#define CONVERT_T           0x44
#define COPY_SCRATCHPAD     0x48
#define WRITE_SCRATCHPAD    0x4e
#define RECALL_EEPROM       0xb8
#define READ_POW_SUPPLY     0xb4
#define READ_SCRATCHPAD     0xbe

/* supported DS therms families */
#define DS18S20             0x10
#define DS1822              0x22
#define DS18B20             0x28
#define DS1825              0x3b
#define DS28EA00            0x42

#define dsth_get_family(id) ((id)&0xff)

/* returns NULL if not supported */
#define dsth_name(id) \
    (((id)&0xff)==DS18S20 ? "DS18S20" : \
    (((id)&0xff)==DS1822 ? "DS1822" : \
    (((id)&0xff)==DS18B20 ? "DS18B20" : \
    (((id)&0xff)==DS1825 ? "DS1825" : \
    (((id)&0xff)==DS28EA00 ? "DS28EA00" : \
    (NULL))))))

#define dsth_is_supported(id) (dsth_name(id)!=NULL)

typedef struct _dsth_scratchpad_t
{
    uint8_t temp_lsb;
    uint8_t temp_hsb;
    uint8_t th;
    uint8_t tl;
    uint8_t cfg_reg;    /* DS18S20: reserved */
    uint8_t reserved;
    uint8_t cnt_rem;    /* DS18S20 only */
    uint8_t cnt_per_c;  /* DS18S20 only */
    uint8_t crc;
} dsth_scratchpad_t;

/* Low level therm functions

   NOTE: All functions containing "pullup" in their names, requires
   the library to be configured with CONFIG_WRITE_PULLUP.
 */

lr_errc_t dsth_read_pow_supply(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, bool_t *p_is_paras);

lr_errc_t dsth_convert_t(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, void *p_status, size_t stat_len);

lr_errc_t dsth_convert_t_with_pullup(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, unsigned int pullup);

lr_errc_t dsth_read_scratchpad(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_scratchpad_t *p_scpd);

/* 'cfg_reg' is ignored if writing the scratchpad for DS18S20 */
lr_errc_t dsth_write_scratchpad(w1_hndl_t *p_w1_h,
    w1_slave_id_t therm, uint8_t th, uint8_t tl, uint8_t cfg_reg);

lr_errc_t dsth_copy_scratchpad(w1_hndl_t *p_w1_h, w1_slave_id_t therm);

lr_errc_t dsth_copy_scratchpad_with_pullup(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, unsigned int pullup);

lr_errc_t dsth_recall_eeprom(
    w1_hndl_t *p_w1_h, w1_slave_id_t therm, uint8_t *p_status);

/* Functions below perform the same actions as their counterparts above but
   for all devices connected to the bus controlled by a master.
 */

lr_errc_t dsth_convert_t_all(w1_hndl_t *p_w1_h, w1_master_id_t master);

lr_errc_t dsth_convert_t_with_pullup_all(
    w1_hndl_t *p_w1_h, w1_master_id_t master, unsigned int pullup);

/* use with care: DS18S20 should not be present on the bus */
lr_errc_t dsth_write_scratchpad_all(w1_hndl_t *p_w1_h,
    w1_master_id_t master, uint8_t th, uint8_t tl, uint8_t cfg_reg);

/* use with care: bus with DS18S20 only */
lr_errc_t dsth_write_scratchpad_all_ds18s20(
    w1_hndl_t *p_w1_h, w1_master_id_t master, uint8_t th, uint8_t tl);

lr_errc_t dsth_copy_scratchpad_all(w1_hndl_t *p_w1_h, w1_master_id_t master);

lr_errc_t dsth_copy_scratchpad_with_pullup_all(
    w1_hndl_t *p_w1_h, w1_master_id_t master, unsigned int pullup);

lr_errc_t dsth_recall_eeprom_all(w1_hndl_t *p_w1_h, w1_master_id_t master);

typedef enum _dsth_res_t {
    DSTH_RES_9BIT =0,
    DSTH_RES_10BIT,
    DSTH_RES_11BIT,
    DSTH_RES_12BIT
} dsth_res_t;

#define DSTH_CONV_T_12BIT       750U

/* Returns conversion time (msec) for a given resolution */
#define dsth_get_conv_time(res) \
    (DSTH_CONV_T_12BIT >> (3-(((unsigned int)(res))%4)))

/* min. strong pull-up time for COPY_SCRATCHPAD command (msec) */
#define COPY_SCRATCHPAD_PULLUP  10U

/* Get/set therm resolution for a single therm.
   The resolution is set by fetching the current configuration, modifying it
   and writing back to the device.

   NOTE: For DS18S20 the resolution is always 12-bit and can not be changed,
   therefore dsth_set_res() returns LREC_DEV_ERR if called with resolution
   other than 12-bit.
 */
lr_errc_t dsth_get_res(w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_res_t *p_res);
lr_errc_t dsth_set_res(w1_hndl_t *p_w1_h, w1_slave_id_t therm, dsth_res_t res);

/* The function fetches temperature value from the scratchpad (previously
   obtained by dsth_read_scratchpad()). The value is scaled by 1000.
 */
int dsth_get_temp_scratchpad(
    w1_slave_id_t therm, const dsth_scratchpad_t *p_scpd);

/* Probe a single therm for the temperature and write it under 'p_temp' (x1000).
 */
lr_errc_t dsth_probe(w1_hndl_t *p_w1_h, w1_slave_id_t therm, int *p_temp);

#ifdef __cplusplus
}
#endif

#endif /* __LR_DEVS_DS_THERM_H__ */
