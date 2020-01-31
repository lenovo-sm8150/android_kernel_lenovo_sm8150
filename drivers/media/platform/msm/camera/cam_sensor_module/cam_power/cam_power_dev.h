#ifndef _CAM_POWER_DEV_H_
#define _CAM_POWER_DEV_H_

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <media/cam_sensor.h>
#include <cam_sensor_i2c.h>
#include <cam_sensor_spi.h>
#include <cam_sensor_io.h>
#include <cam_cci_dev.h>
#include <cam_req_mgr_util.h>
#include <cam_req_mgr_interface.h>
#include <cam_mem_mgr.h>
#include <cam_subdev.h>
#include "cam_soc_util.h"

enum cam_power_state {
	CAM_POWER_INIT,
	CAM_POWER_ACQUIRE,
	CAM_POWER_CONFIG,
};

struct cam_power_i2c_info_t {
	uint16_t slave_addr;
	uint8_t i2c_freq_mode;
};

struct cam_power_soc_private {
	const char *power_name;
	struct cam_power_i2c_info_t i2c_info;
	struct cam_sensor_power_ctrl_t power_info;
};

struct cam_power_intf_params {
	int32_t device_hdl;
	int32_t session_hdl;
	int32_t link_hdl;
	struct cam_req_mgr_kmd_ops ops;
	struct cam_req_mgr_crm_cb *crm_cb;
};

typedef struct cam_power_ctrl {
	struct platform_device *pdev;
	struct device *dev;
	struct mutex power_mutex;
	struct cam_hw_soc_info soc_info;
	struct msm_camera_gpio_num_info *gpio_num_info;
	struct camera_io_master io_master_info;
	struct cam_subdev v4l2_dev_str;
	struct cam_power_intf_params bridge_intf;
	struct i2c_data_settings i2c_data;
	enum cci_i2c_master_t cci_i2c_master;
	enum cci_device_num cci_num;
	enum msm_camera_device_type_t power_device_type;
	enum cam_power_state cam_power_state;
	bool userspace_probe;
	char device_name[20];
	uint16_t cell_id;
}cam_power_ctrl_t;

typedef enum cam_cell_id {
	CAM_CELL_ID_0,
	CAM_CELL_ID_1,
	CAM_CELL_ID_2,
	CAM_CELL_ID_3,
	CAM_CELL_ID_4,
}cam_cell_id_t;
extern cam_power_ctrl_t* get_cam_power_ctrl(void);
extern int cam_power_ldo_control(uint16_t cam_cell_id, bool enable);
extern uint16_t get_cell_id(void);

#endif
