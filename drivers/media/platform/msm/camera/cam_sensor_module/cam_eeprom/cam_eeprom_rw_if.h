#ifndef __CAM_EEPROM_RW_IF_H__
#define __CAM_EEPROM_RW_IF_H__
#include <linux/fs.h>
#include "cam_eeprom_dev.h"
#include "cam_sensor_cmn_header.h"

typedef enum {
	CAM_ID_0,
	CAM_ID_1,
	CAM_ID_2,
	CAM_ID_3,
	MAX_CAM_ID
}max_cam_id_t;

typedef enum {
	MEM_OK,
	MEM_FAILED
}eeprom_if_state_t;

typedef struct {
	struct cam_sensor_power_ctrl_t power_info;
	struct cam_hw_soc_info soc_info;
	int state;
}cam_eeprom_rw_t;

#if __EEPROM_RW_POWER_BY_KERNEL__
extern int eeprom_rw_if_init(struct cam_eeprom_ctrl_t *e_ctrl);
extern void eeprom_rw_if_release(int cam_id);
extern int eeprom_rw_if_power_up(int cam_id);
extern int eeprom_rw_if_power_down(int cam_id);
extern void set_eeprom_handle(struct cam_eeprom_ctrl_t *e_ctrl);
extern cam_eeprom_rw_t* get_eeprom_handle(int cam_id);
#endif
extern int eeprom_rw_if_sysfs_create(struct kobject *kobj);
#endif
