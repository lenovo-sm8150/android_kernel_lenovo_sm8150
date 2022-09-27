/* Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <cam_sensor_cmn_header.h>
#include "cam_ois_core.h"
#include "cam_ois_soc.h"
#include "cam_sensor_util.h"
#include "cam_debug_util.h"
#include "cam_res_mgr_api.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"

/* LENOVO_CUSTOM on 20190115 : start */
#ifdef CONFIG_PRODUCT_ZIPPO
//#define OIS_DEBUG_FUNC
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "OIS_head.h"
#include "OIS_prog.h"
#include "OIS_coef.h"
#include "OIS_calibration.h"
/* LENOVO_CUSTOM 20190507 start */
#define CAM_SENSOR_REGSETTING_MAXCOUNT 500 /* 500 * 16 bytes is about 8K bytes */
/* LENOVO_CUSTOM 20190507 end */
OIS_INT OIS_IIC_Check(struct camera_io_master* ois_io_master_info) {
	uint32_t	i2c_sts = 0, i = 0;
	OIS_INT rc = OIS_I2C_ERR;
	for (i=0; i<OIS_RETRY_NUMBER; i++) {
		msleep(1);
		/* Check I2C status */
		i2c_sts = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_SENSOR, CAMERA_SENSOR_I2C_TYPE_BYTE, _OIS_I2C_CHECK_STS);
		if (i2c_sts != OIS_I2C_CHECK_VALUE) {
			rc = OIS_I2C_ERR;
			continue;
		} else {
			rc = ADJ_OK;
			break;
		}
	}
	return ADJ_OK;
}
OIS_INT OIS_Reg_Check(struct camera_io_master* ois_io_master_info, uint32_t addr) {
	uint32_t	reg_sts = 0;
	reg_sts = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_SENSOR, CAMERA_SENSOR_I2C_TYPE_BYTE, addr);
	CAM_ERR(CAM_OIS,"###ois [INFO] [%s] reg_sts[0x%x]:0x%x",__func__,addr,reg_sts);
	return ADJ_OK;
}
OIS_INT OIS_Start_DL(struct camera_io_master* ois_io_master_info)
{
	OIS_INT rc = OIS_I2C_ERR;
	/* Check I2C status */
	rc = OIS_IIC_Check(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,"[ERR] OIS IIC Check rc=%d",rc);
		return rc;
	}

	/* Access to OIS Start DL */
	I2C_OIS_Single_write(ois_io_master_info, _OIS_START_DL, 0x00);
	msleep(1);

	return ADJ_OK;
}

OIS_INT OIS_SUM_Check(struct camera_io_master* ois_io_master_info)
{
	OIS_UINT sum_check = OIS_SUM_ERR;

	/* Check SUM Status */
	sum_check = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_SENSOR, CAMERA_SENSOR_I2C_TYPE_DWORD, _OIS_SUM_CHECK);
	if (sum_check != OIS_SUM_CHECK_VALUE) {
		CAM_ERR(CAM_OIS,"[ERR] [%s] SUM status:0x%04x",__func__,sum_check);
		return OIS_SUM_ERR;//Fail
	}
	return ADJ_OK;
}

OIS_INT OIS_Complete_DL(struct camera_io_master* ois_io_master_info)
{
	OIS_INT rc = OIS_I2C_ERR;
	/* Access to OIS Complete Download */
	I2C_OIS_Single_write(ois_io_master_info, _OIS_COMP_DL, 0x00);
	/* Check I2C status */
	rc = OIS_IIC_Check(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,"[ERR] OIS IIC Check rc=%d",rc);
		return rc;
	}
	return ADJ_OK;
}

void	I2C_OIS_Single_write(struct camera_io_master* ois_io_master_info, OIS_UINT addr, OIS_UINT dat){

	int32_t rc = 0;
	int32_t size = 1;

	struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
	struct cam_sensor_i2c_reg_array    *i2c_reg_array =
		(struct cam_sensor_i2c_reg_array *)
		kzalloc(sizeof(struct cam_sensor_i2c_reg_array)*size,
				GFP_KERNEL);

	memset(&i2c_reg_settings, 0, sizeof(i2c_reg_settings));
	memset(i2c_reg_array, 0, sizeof(*i2c_reg_array)*size);
	i2c_reg_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_settings.size = size;
	i2c_reg_array->reg_addr = addr;
	i2c_reg_array->reg_data = dat;
	i2c_reg_array->delay = 0x0;
	i2c_reg_settings.reg_setting = i2c_reg_array;
	i2c_reg_settings.delay = 1;

	rc = camera_io_dev_write(ois_io_master_info,&i2c_reg_settings);
	if (rc < 0){
		CAM_ERR(CAM_OIS,"[ERR] [%s] write error : %d",__func__,rc);
	}
	kfree(i2c_reg_array);
}

void I2C_OIS_Multi_write(struct camera_io_master* ois_io_master_info,
        OIS_UINT addr, OIS_UWORD size, OIS_UBYTE *dat )
{
	int32_t rc = 0, i = 0;

/* LENOVO_CUSTOM 20190507 start */
	struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
    struct cam_sensor_i2c_reg_array*   i2c_reg_array = NULL;
    OIS_UWORD left_count = size;
    OIS_UWORD write_count = 0;
    uint32_t reg_offset = 0;

    memset(&i2c_reg_settings, 0, sizeof(i2c_reg_settings));
    while (left_count > 0) {
        if (left_count > CAM_SENSOR_REGSETTING_MAXCOUNT) {
            write_count = CAM_SENSOR_REGSETTING_MAXCOUNT;
        } else {
            write_count = left_count;
        }

        CAM_DBG(CAM_OIS, "write count %u, size of reg arry %lu",
                write_count, (sizeof(struct cam_sensor_i2c_reg_array) * write_count));
        i2c_reg_array = (struct cam_sensor_i2c_reg_array *)
            kzalloc(sizeof(struct cam_sensor_i2c_reg_array) * write_count,
                    GFP_KERNEL);
        memset(i2c_reg_array, 0, sizeof(*i2c_reg_array) * write_count);

        for (i = 0; i < write_count; i++){
            i2c_reg_array[i].reg_addr = addr + i + reg_offset;
            i2c_reg_array[i].reg_data = dat[i + reg_offset];
            i2c_reg_array[i].delay = 0x0;
#if 0
            if ((left_count == size && i < 5) || (left_count <= CAM_SENSOR_REGSETTING_MAXCOUNT && i > write_count - 5))
                CAM_ERR(CAM_OIS,"###ois [INFO] [%s] size=%d i=%d addr=0x%04x data=0x%04x",__func__,size,i,
                        i2c_reg_array[i].reg_addr,i2c_reg_array[i].reg_data);
#endif
        }

        i2c_reg_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
        i2c_reg_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
        i2c_reg_settings.size = write_count;
        i2c_reg_settings.reg_setting = i2c_reg_array;
        i2c_reg_settings.delay = 1;

        rc = camera_io_dev_write_continuous(ois_io_master_info, &i2c_reg_settings, 0);
        if (rc < 0)
            CAM_ERR(CAM_OIS,"###ois [ERR] [%s] write error : %d",__func__,rc);

        kfree(i2c_reg_array);

        reg_offset = reg_offset + write_count;
        left_count = left_count - write_count;
        CAM_DBG(CAM_OIS, "left count %u", left_count);
    }
/* LENOVO_CUSTOM 20190507 end */
}

// *********************************************************
// Read Data from Slave device via I2C master device
// ---------------------------------------------------------
// <Function>
//		I2C master read data from the I2C slave device.
//		This function relate to your own circuit.
//
// <Input>
//		OIS_UBYTE	slvadr	I2C slave adr
//		OIS_UBYTE	size	Transfer Size
//		OIS_UBYTE	*dat	data matrix
//
// <Output>
//		OIS_UWORD	16bit data read from I2C Slave device
//
// <Description>
//	if size == 1
//		[S][SlaveAdr][W]+[dat[0]]+         [RS][SlaveAdr][R]+[RD_DAT0]+[RD_DAT1][P]
//	if size == 2
//		[S][SlaveAdr][W]+[dat[0]]+[dat[1]]+[RS][SlaveAdr][R]+[RD_DAT0]+[RD_DAT1][P]
//
// *********************************************************
uint32_t I2C_OIS_Single_read(struct camera_io_master* ois_io_master_info, OIS_UBYTE slvadr, OIS_UBYTE size, OIS_UINT dat )
{
	int32_t rc = 0;
	uint32_t   ret_data = 0;
	uint32_t   read_addr = 0;

	read_addr = dat;
	ois_io_master_info->cci_client->sid = slvadr;
	ois_io_master_info->cci_client->i2c_freq_mode = I2C_FAST_PLUS_MODE;
	rc = camera_io_dev_read(ois_io_master_info,read_addr,&ret_data,CAMERA_SENSOR_I2C_TYPE_WORD,size);
	if (rc < 0){
		CAM_ERR(CAM_OIS,"[ERR] [%s] read error : %d",__func__,rc);
	}

	return ret_data;
}

//  *****************************************************
//  **** Program Download Function
//  *****************************************************
OIS_INT OIS_FW_Download(struct camera_io_master* ois_io_master_info)
{
	OIS_INT rc = ADJ_ERR;

	//Start to Download
	rc = OIS_Start_DL(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] OIS_start_DL rc:%d",rc);
		return FW_DL_ERR;//Fail
	}

	/* Program data1 Download */
	func_download(ois_io_master_info, 0);
	/* Coeff data2 Download */
	func_download(ois_io_master_info, 1);

	//OIS SUM Check
	rc = OIS_SUM_Check(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] OIS_SUM_Check rc:%d",rc);
		return OIS_SUM_ERR;//Fail
	}
	return ADJ_OK;//Success
}

//  *****************************************************
//  **** Download the data
//  *****************************************************
void	func_download(struct camera_io_master* ois_io_master_info, OIS_UBYTE u16_type){
	/* 0:Prog download 1:Coef download */
	OIS_UBYTE flag_type = u16_type;
	OIS_UBYTE *dat;

	if ( flag_type == 0 ){
		/* Data Transfer for prog data1 download */
		dat = DOWNLOAD_PROG;
		I2C_OIS_Multi_write(ois_io_master_info, _OIS_FW_DL_PROG_ADDR, DOWNLOAD_PROG_LEN, dat);
	} else {
		/* Data Transfer for coef data2 download */
		dat = DOWNLOAD_COEF;
		I2C_OIS_Multi_write(ois_io_master_info, _OIS_FW_DL_COEF_ADDR, DOWNLOAD_COEF_LEN, dat);
	}
}

OIS_INT OIS_Calibration_Download( struct camera_io_master* ois_io_master_info )
{
	OIS_INT rc = ADJ_ERR;

	func_FADJ_Calibration_data_from_eeprom(ois_io_master_info);

	//Complete Download
	rc = OIS_Complete_DL(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] [%s] rc:%d",__func__,rc);
		return OIS_I2C_ERR;// FAIL
	}

	return ADJ_OK;// Success
}

// *********************************************************
// Read Factory Adjusted data from the non-volatile memory
// ---------------------------------------------------------
// <Function>
//		Factory adjusted data are sotred somewhere
//		non-volatile memory.  I2C master has to read these
//		data and store the data to the OIS controller.
//
// <Input>
//		none
//
// <Output>
//		_FACT_ADJ	Factory Adjusted data
//
// <Description>
//		You have to port your own system.
//
// *********************************************************
void func_FADJ_Calibration_data_from_eeprom( struct camera_io_master* ois_io_master_info)
{
	OIS_UBYTE dat[OIS_CAL_DATA_SIZE] = {0};
	OIS_UBYTE *dat_default;
	int32_t i = 0;
	ois_io_master_info->cci_client->sid = _SLV_OIS_EEPROM;
	ois_io_master_info->cci_client->i2c_freq_mode = I2C_FAST_PLUS_MODE;
	for (i=0;i<OIS_CAL_DATA_SIZE;i++) {
		/* Module vendor made mistake in downloading ois eeprom,we need exchange High Byte and Low Byte with OIS EEPROM data */
		if ((i>3&&i<20) || (i>21&&i<38)) {
			if (i%2 == 0)
				dat[i+1] = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_EEPROM, CAMERA_SENSOR_I2C_TYPE_BYTE, OIS_CAL_DATA_ADDR_START+i);
			else
				dat[i-1] = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_EEPROM, CAMERA_SENSOR_I2C_TYPE_BYTE, OIS_CAL_DATA_ADDR_START+i);
		}else
			dat[i] = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_EEPROM, CAMERA_SENSOR_I2C_TYPE_BYTE, OIS_CAL_DATA_ADDR_START+i);
	}
#ifdef OIS_DEBUG_FUNC
	for(i=0;i<OIS_CAL_DATA_SIZE;i++){
		CAM_ERR(CAM_OIS,"[INFO] dat[%d]=0x%x",i,dat[i]);
	}
#endif
	ois_io_master_info->cci_client->sid = _SLV_OIS_SENSOR;
	ois_io_master_info->cci_client->i2c_freq_mode = I2C_FAST_PLUS_MODE;
	if (dat[OSCILLATOR_OFFSET] != 0xff) {
		I2C_OIS_Multi_write(ois_io_master_info, _OIS_CAL_DATA_DL_ADDR, OIS_CAL_DATA_SIZE, dat);
	} else {
		/* OIS EEPROM Calibration data is NULL,instead of default data */
		dat_default = DOWNLOAD_CALIBRATION;
		I2C_OIS_Multi_write(ois_io_master_info, _OIS_CAL_DATA_DL_ADDR, DOWNLOAD_CALIBRATION_LEN, dat_default);
		CAM_ERR(CAM_OIS,"###ois [WARNING] oscillator=0x%x OIS EEPROM Calibration data is NULL,instead of default data",dat[OSCILLATOR_OFFSET]);
	}
}

OIS_INT OIS_init_data( struct camera_io_master* ois_io_master_info ) {
	OIS_INT rc = ADJ_ERR;
	ois_io_master_info->cci_client->i2c_freq_mode = I2C_FAST_PLUS_MODE;
	/* servo on */
	rc = OIS_init_servo_on(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] OIS_init_servo_on rc:%d",rc);
		return OIS_INIT_SERVO_ON_ERR;//Fail
	}
	/* gyro setting */
	rc = OIS_init_gyro_setting(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] OIS_init_gyro_setting rc:%d",rc);
		return OIS_INIT_GYRO_SET_ERR;//Fail
	}
	/* gyro on */
	rc = OIS_init_gyro_on(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] OIS_init_gyro_on rc:%d",rc);
		return OIS_INIT_GYRO_ON_ERR;//Fail
	}
	/* ois mode */
	rc = OIS_init_ois_mode(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] OIS_init_ois_mode rc:%d",rc);
		return OIS_INIT_OISMODE_ERR;//Fail
	}
	/* ois on */
	rc = OIS_init_ois_on(ois_io_master_info);
	if(rc) {
		CAM_ERR(CAM_OIS,"[ERR] OIS_init_ois_on rc:%d",rc);
		return OIS_INIT_OIS_ON_ERR;//Fail
	}
	return ADJ_OK;
}

OIS_INT OIS_init_servo_on(struct camera_io_master* ois_io_master_info) {
	OIS_INT rc = OIS_I2C_ERR;
	I2C_OIS_Single_write(ois_io_master_info, _OIS_SERVO_ON_6020H, 0x01);
	/* Check I2C status */
	rc = OIS_IIC_Check(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,"[ERR] OIS IIC Check rc=%d",rc);
		return rc;
	}
	return ADJ_OK;
}
OIS_INT OIS_init_gyro_setting(struct camera_io_master* ois_io_master_info) {
	OIS_INT rc = OIS_I2C_ERR;
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_6023H, 0x02);
	/* Check I2C status */
	rc = OIS_IIC_Check(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,"[ERR] OIS IIC Check rc=%d",rc);
		return rc;
	}
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602CH, 0x11);
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602DH, 0x01);
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602CH, 0x11);
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602DH, 0x00);
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602CH, 0x4E);
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602DH, 0x0C);
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602CH, 0x4F);
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_602DH, 0x63);
	msleep(20);
	return ADJ_OK;
}
OIS_INT OIS_init_gyro_on(struct camera_io_master* ois_io_master_info) {
	OIS_INT rc = OIS_I2C_ERR;
	I2C_OIS_Single_write(ois_io_master_info, _OIS_GYRO_SET_6023H, 0x00);
	/* Check I2C status */
	rc = OIS_IIC_Check(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,"[ERR] OIS IIC Check rc=%d",rc);
		return rc;
	}
	return ADJ_OK;
}
OIS_INT OIS_init_ois_mode(struct camera_io_master* ois_io_master_info) {
	I2C_OIS_Single_write(ois_io_master_info, _OIS_OIS_MODE_6021H, 0x7B);
	return ADJ_OK;
}
OIS_INT OIS_init_ois_on(struct camera_io_master* ois_io_master_info) {
	OIS_INT rc = OIS_I2C_ERR;
	/* Check I2C status */
	rc = OIS_IIC_Check(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,"[ERR] OIS IIC Check rc=%d",rc);
		return rc;
	}
	I2C_OIS_Single_write(ois_io_master_info, _OIS_OIS_ON_6020H, 0x02);

	return ADJ_OK;
}

void OIS_Gyro_Hall_Check_STS(struct camera_io_master* ois_io_master_info) {

	uint32_t	gyro_yout = 0,gyro_xout = 0,hall_x = 0,hall_y = 0;
	uint32_t	gyro_yout_n = 0,gyro_xout_n = 0,hall_x_n = 0,hall_y_n = 0;
	/* Check Gyro&Hall status */
	gyro_yout = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_SENSOR, CAMERA_SENSOR_I2C_TYPE_WORD, _OIS_GYRO_YOUT);
	gyro_xout = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_SENSOR, CAMERA_SENSOR_I2C_TYPE_WORD, _OIS_GYRO_XOUT);
	hall_x = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_SENSOR, CAMERA_SENSOR_I2C_TYPE_BYTE, _OIS_HALL_X);
	hall_y = I2C_OIS_Single_read(ois_io_master_info, _SLV_OIS_SENSOR, CAMERA_SENSOR_I2C_TYPE_BYTE, _OIS_HALL_Y);
	gyro_yout_n = ((gyro_yout&0x1000) | (~gyro_yout & 0xFFFF))+1;
	gyro_xout_n = ((gyro_xout&0x1000) | (~gyro_xout & 0xFFFF))+1;
	hall_x_n = ((hall_x&0x1000) | (~hall_x & 0xFFFF))+1;
	hall_y_n = ((hall_y&0x1000) | (~hall_y & 0xFFFF))+1;
	CAM_ERR(CAM_OIS,"###ois [INFO] Gyro_Xout:%d Gyro_Yout:%d Hall_X:%d Hall_Y:%d",
		gyro_xout,gyro_yout,hall_x,hall_y);
	CAM_ERR(CAM_OIS,"###ois [INFO] Gyro_Xout_n:%d Gyro_Yout_n:%d Hall_X_n:%d Hall_Y_n:%d",
		gyro_xout_n,gyro_yout_n,hall_x_n,hall_y_n);
}
OIS_INT cam_ois_fw_init(struct camera_io_master *ois_io_master_info) {

	OIS_INT rc = ADJ_ERR;

	/* FW prog&coeff data downloaded */
	rc  = OIS_FW_Download(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,
				"[ERR] Failed to apply OIS_FW_Download: %d",rc);
		return rc;
	}

	/* Read ois calibration data from eeprom and Download to ois sensor */
	rc = OIS_Calibration_Download(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,
				"[ERR] Failed to apply OIS_Calibration_Download: %d",rc);
		return rc;
	}

	/* Check I2C status */
	rc = OIS_IIC_Check(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,
				"[ERR] Failed to OIS IIC check status: %d",rc);
		return rc;
	}

	/* Init setting for OIS */
	rc = OIS_init_data(ois_io_master_info);
	if (rc < 0) {
		CAM_ERR(CAM_OIS,
				"[ERR] Failed to apply OIS_init_data: %d",rc);
		return rc;
	}
	return ADJ_OK;
}
#endif
/* LENOVO_CUSTOM on 20190115 : end */

int32_t cam_ois_construct_default_power_setting(
	struct cam_sensor_power_ctrl_t *power_info)
{
	int rc = 0;

	power_info->power_setting_size = 1;
	power_info->power_setting =
		(struct cam_sensor_power_setting *)
		kzalloc(sizeof(struct cam_sensor_power_setting),
			GFP_KERNEL);
	if (!power_info->power_setting)
		return -ENOMEM;

	power_info->power_setting[0].seq_type = SENSOR_VAF;
	power_info->power_setting[0].seq_val = CAM_VAF;
	power_info->power_setting[0].config_val = 1;
	power_info->power_setting[0].delay = 2;

	power_info->power_down_setting_size = 1;
	power_info->power_down_setting =
		(struct cam_sensor_power_setting *)
		kzalloc(sizeof(struct cam_sensor_power_setting),
			GFP_KERNEL);
	if (!power_info->power_down_setting) {
		rc = -ENOMEM;
		goto free_power_settings;
	}

	power_info->power_down_setting[0].seq_type = SENSOR_VAF;
	power_info->power_down_setting[0].seq_val = CAM_VAF;
	power_info->power_down_setting[0].config_val = 0;

	return rc;

free_power_settings:
	kfree(power_info->power_setting);
	power_info->power_setting = NULL;
	power_info->power_setting_size = 0;
	return rc;
}


/**
 * cam_ois_get_dev_handle - get device handle
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_get_dev_handle(struct cam_ois_ctrl_t *o_ctrl,
	void *arg)
{
	struct cam_sensor_acquire_dev    ois_acq_dev;
	struct cam_create_dev_hdl        bridge_params;
	struct cam_control              *cmd = (struct cam_control *)arg;

	if (o_ctrl->bridge_intf.device_hdl != -1) {
		CAM_ERR(CAM_OIS, "Device is already acquired");
		return -EFAULT;
	}
	if (copy_from_user(&ois_acq_dev, u64_to_user_ptr(cmd->handle),
		sizeof(ois_acq_dev)))
		return -EFAULT;

	bridge_params.session_hdl = ois_acq_dev.session_handle;
	bridge_params.ops = &o_ctrl->bridge_intf.ops;
	bridge_params.v4l2_sub_dev_flag = 0;
	bridge_params.media_entity_flag = 0;
	bridge_params.priv = o_ctrl;
	bridge_params.dev_id = CAM_OIS;
	ois_acq_dev.device_handle =
		cam_create_device_hdl(&bridge_params);
	if (ois_acq_dev.device_handle <= 0) {
		CAM_ERR(CAM_OIS, "Can not create device handle");
		return -EFAULT;
	}
	o_ctrl->bridge_intf.device_hdl = ois_acq_dev.device_handle;
	o_ctrl->bridge_intf.session_hdl = ois_acq_dev.session_handle;

	CAM_DBG(CAM_OIS, "Device Handle: %d", ois_acq_dev.device_handle);
	if (copy_to_user(u64_to_user_ptr(cmd->handle), &ois_acq_dev,
		sizeof(struct cam_sensor_acquire_dev))) {
		CAM_ERR(CAM_OIS, "ACQUIRE_DEV: copy to user failed");
		return -EFAULT;
	}
	return 0;
}

static int cam_ois_power_up(struct cam_ois_ctrl_t *o_ctrl)
{
	int                             rc = 0;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;
	struct cam_sensor_power_ctrl_t  *power_info;

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

	if ((power_info->power_setting == NULL) &&
		(power_info->power_down_setting == NULL)) {
		CAM_INFO(CAM_OIS,
			"Using default power settings");
		rc = cam_ois_construct_default_power_setting(power_info);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Construct default ois power setting failed.");
			return rc;
		}
	}

	/* Parse and fill vreg params for power up settings */
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_setting,
		power_info->power_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power up rc:%d", rc);
		return rc;
	}

	/* Parse and fill vreg params for power down settings*/
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_down_setting,
		power_info->power_down_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power down rc:%d", rc);
		return rc;
	}

	power_info->dev = soc_info->dev;

	rc = cam_sensor_core_power_up(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_OIS, "failed in ois power up rc %d", rc);
		return rc;
	}

	rc = camera_io_init(&o_ctrl->io_master_info);
	if (rc)
		CAM_ERR(CAM_OIS, "cci_init failed: rc: %d", rc);

	return rc;
}

/**
 * cam_ois_power_down - power down OIS device
 * @o_ctrl:     ctrl structure
 *
 * Returns success or failure
 */
static int cam_ois_power_down(struct cam_ois_ctrl_t *o_ctrl)
{
	int32_t                         rc = 0;
	struct cam_sensor_power_ctrl_t  *power_info;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "failed: o_ctrl %pK", o_ctrl);
		return -EINVAL;
	}

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;
	soc_info = &o_ctrl->soc_info;

	if (!power_info) {
		CAM_ERR(CAM_OIS, "failed: power_info %pK", power_info);
		return -EINVAL;
	}

	rc = cam_sensor_util_power_down(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_OIS, "power down the core is failed:%d", rc);
		return rc;
	}

	camera_io_release(&o_ctrl->io_master_info);

	return rc;
}

static int cam_ois_apply_settings(struct cam_ois_ctrl_t *o_ctrl,
	struct i2c_settings_array *i2c_set)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0;
	uint32_t i, size;

	if (o_ctrl == NULL || i2c_set == NULL) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	if (i2c_set->is_settings_valid != 1) {
		CAM_ERR(CAM_OIS, " Invalid settings");
		return -EINVAL;
	}

	list_for_each_entry(i2c_list,
		&(i2c_set->list_head), list) {
		if (i2c_list->op_code ==  CAM_SENSOR_I2C_WRITE_RANDOM) {
			//LENOVO_CUSTOM show ois enable mode from camx : start
			if (i2c_list->i2c_settings.reg_setting->reg_addr == 0x6020){
				CAM_ERR(CAM_OIS, "###ois [INFO] addr=0x%x data=0x%x",i2c_list->i2c_settings.reg_setting->reg_addr,
					i2c_list->i2c_settings.reg_setting->reg_data);
			}
			//LENOVO_CUSTOM show ois enable mode from camx : end
			rc = camera_io_dev_write(&(o_ctrl->io_master_info),
				&(i2c_list->i2c_settings));
			if (rc < 0) {
				CAM_ERR(CAM_OIS,
					"Failed in Applying i2c wrt settings");
				return rc;
			}
		} else if (i2c_list->op_code == CAM_SENSOR_I2C_POLL) {
			size = i2c_list->i2c_settings.size;
			for (i = 0; i < size; i++) {
				rc = camera_io_dev_poll(
				&(o_ctrl->io_master_info),
				i2c_list->i2c_settings.reg_setting[i].reg_addr,
				i2c_list->i2c_settings.reg_setting[i].reg_data,
				i2c_list->i2c_settings.reg_setting[i].data_mask,
				i2c_list->i2c_settings.addr_type,
				i2c_list->i2c_settings.data_type,
				i2c_list->i2c_settings.reg_setting[i].delay);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"i2c poll apply setting Fail");
					return rc;
				}
			}
		}
	}

	return rc;
}

static int cam_ois_slaveInfo_pkt_parser(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t *cmd_buf, size_t len)
{
	int32_t rc = 0;
	struct cam_cmd_ois_info *ois_info;

	if (!o_ctrl || !cmd_buf || len < sizeof(struct cam_cmd_ois_info)) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	ois_info = (struct cam_cmd_ois_info *)cmd_buf;
	if (o_ctrl->io_master_info.master_type == CCI_MASTER) {
		o_ctrl->io_master_info.cci_client->i2c_freq_mode =
			ois_info->i2c_freq_mode;
		o_ctrl->io_master_info.cci_client->sid =
			ois_info->slave_addr >> 1;
		o_ctrl->ois_fw_flag = ois_info->ois_fw_flag;
		o_ctrl->is_ois_calib = ois_info->is_ois_calib;
		memcpy(o_ctrl->ois_name, ois_info->ois_name, OIS_NAME_LEN);
		o_ctrl->ois_name[OIS_NAME_LEN - 1] = '\0';
		o_ctrl->io_master_info.cci_client->retries = 3;
		o_ctrl->io_master_info.cci_client->id_map = 0;
		memcpy(&(o_ctrl->opcode), &(ois_info->opcode),
			sizeof(struct cam_ois_opcode));
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x Freq Mode: %d",
			ois_info->slave_addr, ois_info->i2c_freq_mode);
	} else if (o_ctrl->io_master_info.master_type == I2C_MASTER) {
		o_ctrl->io_master_info.client->addr = ois_info->slave_addr;
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x", ois_info->slave_addr);
	} else {
		CAM_ERR(CAM_OIS, "Invalid Master type : %d",
			o_ctrl->io_master_info.master_type);
		rc = -EINVAL;
	}

	return rc;
}

static int cam_ois_fw_download(struct cam_ois_ctrl_t *o_ctrl)
{
#ifdef CONFIG_PRODUCT_ZIPPO
	OIS_INT rc = ADJ_ERR;
	rc = cam_ois_fw_init(&(o_ctrl->io_master_info));
	if (rc < 0) {
		CAM_ERR(CAM_OIS,
				"###ois [ERR] Failed to call cam_ois_fw_init: %d",rc);
	}else
		CAM_ERR(CAM_OIS,
				"###ois [INFO][OK] call cam_ois_fw_init Succeed: %d",rc);
#ifdef OIS_DEBUG_FUNC
	/* Check Gyro&Hall Status */
	OIS_Gyro_Hall_Check_STS(&(o_ctrl->io_master_info));
	msleep(1);
	OIS_Gyro_Hall_Check_STS(&(o_ctrl->io_master_info));
	msleep(1);
	OIS_Gyro_Hall_Check_STS(&(o_ctrl->io_master_info));
#endif
	return rc;
#else
	uint16_t                           total_bytes = 0;
	uint8_t                           *ptr = NULL;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	const struct firmware             *fw = NULL;
	const char                        *fw_name_prog = NULL;
	const char                        *fw_name_coeff = NULL;
	char                               name_prog[32] = {0};
	char                               name_coeff[32] = {0};
	struct device                     *dev = &(o_ctrl->pdev->dev);
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	struct page                       *page = NULL;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	snprintf(name_coeff, 32, "%s.coeff", o_ctrl->ois_name);

	snprintf(name_prog, 32, "%s.prog", o_ctrl->ois_name);

	/* cast pointer as const pointer*/
	fw_name_prog = name_prog;
	fw_name_coeff = name_coeff;

	/* Load FW */
	rc = request_firmware(&fw, fw_name_prog, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_prog);
		return rc;
	}

	total_bytes = fw->size;
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0, GFP_KERNEL);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		page_address(page));

	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.prog;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
		goto release_firmware;
	}
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	page = NULL;
	fw_size = 0;
	release_firmware(fw);

	rc = request_firmware(&fw, fw_name_coeff, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_coeff);
		return rc;
	}

	total_bytes = fw->size;
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0, GFP_KERNEL);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		page_address(page));

	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.coeff;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
	if (rc < 0)
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);

release_firmware:
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	release_firmware(fw);

	return rc;
#endif
}

/**
 * cam_ois_pkt_parse - Parse csl packet
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_pkt_parse(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int32_t                         rc = 0;
	int32_t                         i = 0;
	uint32_t                        total_cmd_buf_in_bytes = 0;
	struct common_header           *cmm_hdr = NULL;
	uintptr_t                       generic_ptr;
	struct cam_control             *ioctl_ctrl = NULL;
	struct cam_config_dev_cmd       dev_config;
	struct i2c_settings_array      *i2c_reg_settings = NULL;
	struct cam_cmd_buf_desc        *cmd_desc = NULL;
	uintptr_t                       generic_pkt_addr;
	size_t                          pkt_len;
	size_t                          remain_len = 0;
	struct cam_packet              *csl_packet = NULL;
	size_t                          len_of_buff = 0;
	uint32_t                       *offset = NULL, *cmd_buf;
	struct cam_ois_soc_private     *soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t  *power_info = &soc_private->power_info;

	ioctl_ctrl = (struct cam_control *)arg;
	if (copy_from_user(&dev_config,
		u64_to_user_ptr(ioctl_ctrl->handle),
		sizeof(dev_config)))
		return -EFAULT;
	rc = cam_mem_get_cpu_buf(dev_config.packet_handle,
		&generic_pkt_addr, &pkt_len);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"error in converting command Handle Error: %d", rc);
		return rc;
	}

	remain_len = pkt_len;
	if ((sizeof(struct cam_packet) > pkt_len) ||
		((size_t)dev_config.offset >= pkt_len -
		sizeof(struct cam_packet))) {
		CAM_ERR(CAM_OIS,
			"Inval cam_packet strut size: %zu, len_of_buff: %zu",
			 sizeof(struct cam_packet), pkt_len);
		rc = -EINVAL;
		goto rel_pkt;
	}

	remain_len -= (size_t)dev_config.offset;
	csl_packet = (struct cam_packet *)
		(generic_pkt_addr + (uint32_t)dev_config.offset);

	if (cam_packet_util_validate_packet(csl_packet,
		remain_len)) {
		CAM_ERR(CAM_OIS, "Invalid packet params");
		rc = -EINVAL;
		goto rel_pkt;
	}


	switch (csl_packet->header.op_code & 0xFFFFFF) {
	case CAM_OIS_PACKET_OPCODE_INIT:
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);

		/* Loop through multiple command buffers */
		for (i = 0; i < csl_packet->num_cmd_buf; i++) {
			total_cmd_buf_in_bytes = cmd_desc[i].length;
			if (!total_cmd_buf_in_bytes)
				continue;

			rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
				&generic_ptr, &len_of_buff);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "Failed to get cpu buf : 0x%x",
					cmd_desc[i].mem_handle);
				goto rel_pkt;
			}
			cmd_buf = (uint32_t *)generic_ptr;
			if (!cmd_buf) {
				CAM_ERR(CAM_OIS, "invalid cmd buf");
				rc = -EINVAL;
				goto rel_cmd_buf;
			}

			if ((len_of_buff < sizeof(struct common_header)) ||
				(cmd_desc[i].offset > (len_of_buff -
				sizeof(struct common_header)))) {
				CAM_ERR(CAM_OIS,
					"Invalid length for sensor cmd");
				rc = -EINVAL;
				goto rel_cmd_buf;
			}
			remain_len = len_of_buff - cmd_desc[i].offset;
			cmd_buf += cmd_desc[i].offset / sizeof(uint32_t);
			cmm_hdr = (struct common_header *)cmd_buf;

			switch (cmm_hdr->cmd_type) {
			case CAMERA_SENSOR_CMD_TYPE_I2C_INFO:
				rc = cam_ois_slaveInfo_pkt_parser(
					o_ctrl, cmd_buf, remain_len);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"Failed in parsing slave info");
					goto rel_cmd_buf;
				}
				break;
			case CAMERA_SENSOR_CMD_TYPE_PWR_UP:
			case CAMERA_SENSOR_CMD_TYPE_PWR_DOWN:
				CAM_DBG(CAM_OIS,
					"Received power settings buffer");
				rc = cam_sensor_update_power_settings(
					cmd_buf,
					total_cmd_buf_in_bytes,
					power_info, remain_len);
				if (rc) {
					CAM_ERR(CAM_OIS,
					"Failed: parse power settings");
					goto rel_cmd_buf;
				}
				break;
			default:
			if (o_ctrl->i2c_init_data.is_settings_valid == 0) {
				CAM_DBG(CAM_OIS,
				"Received init settings");
				i2c_reg_settings =
					&(o_ctrl->i2c_init_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"init parsing failed: %d", rc);
					goto rel_cmd_buf;
				}
			} else if ((o_ctrl->is_ois_calib != 0) &&
				(o_ctrl->i2c_calib_data.is_settings_valid ==
				0)) {
				CAM_DBG(CAM_OIS,
					"Received calib settings");
				i2c_reg_settings = &(o_ctrl->i2c_calib_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"Calib parsing failed: %d", rc);
					goto rel_cmd_buf;
				}
			}
			break;
			}
			if (cam_mem_put_cpu_buf(cmd_desc[i].mem_handle))
				CAM_WARN(CAM_OIS, "Failed to put cpu buf: 0x%x",
					cmd_desc[i].mem_handle);
		}

		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
			rc = cam_ois_power_up(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, " OIS Power up failed");
				goto rel_pkt;
			}
			o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
		}
/* LENOVO_CUSTOM on 20190115 : start */
		CAM_ERR(CAM_OIS, "###ois [INFO] o_ctrl->ois_fw_flag=%d,o_ctrl->is_ois_calib=%d",o_ctrl->ois_fw_flag,o_ctrl->is_ois_calib);
		//o_ctrl->ois_fw_flag = 1; //only kernel init ois
		if (o_ctrl->ois_fw_flag) {
			rc = cam_ois_fw_download(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, "Failed OIS FW Download");
				goto pwr_dwn;
			}
		} else {
			rc = -EINVAL;
			goto pwr_dwn;
			rc = cam_ois_apply_settings(o_ctrl, &o_ctrl->i2c_init_data);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "Cannot apply Init settings");
				goto pwr_dwn;
			}
		}

		if (o_ctrl->is_ois_calib) {
			rc = cam_ois_apply_settings(o_ctrl,
				&o_ctrl->i2c_calib_data);
			if (rc) {
				CAM_ERR(CAM_OIS, "Cannot apply calib data");
				goto pwr_dwn;
			}
		}
/* LENOVO_CUSTOM on 20190115 : end */
		rc = delete_request(&o_ctrl->i2c_init_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Init data: rc: %d", rc);
			rc = 0;
		}
		rc = delete_request(&o_ctrl->i2c_calib_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Calibration data: rc: %d", rc);
			rc = 0;
		}
		break;
	case CAM_OIS_PACKET_OPCODE_OIS_CONTROL:
		if (o_ctrl->cam_ois_state < CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Not in right state to control OIS: %d",
				o_ctrl->cam_ois_state);
			goto rel_pkt;
		}
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);
		i2c_reg_settings = &(o_ctrl->i2c_mode_data);
		i2c_reg_settings->is_settings_valid = 1;
		i2c_reg_settings->request_id = 0;
		rc = cam_sensor_i2c_command_parser(&o_ctrl->io_master_info,
			i2c_reg_settings,
			cmd_desc, 1);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "OIS pkt parsing failed: %d", rc);
			goto rel_pkt;
		}

		rc = cam_ois_apply_settings(o_ctrl, i2c_reg_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Cannot apply mode settings");
			goto rel_pkt;
		}

		rc = delete_request(i2c_reg_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Fail deleting Mode data: rc: %d", rc);
			goto rel_pkt;
		}
		break;
	default:
		CAM_ERR(CAM_OIS, "Invalid Opcode: %d",
			(csl_packet->header.op_code & 0xFFFFFF));
		rc = -EINVAL;
		goto rel_pkt;
	}

	if (!rc)
		goto rel_pkt;

rel_cmd_buf:
	if (cam_mem_put_cpu_buf(cmd_desc[i].mem_handle))
		CAM_WARN(CAM_OIS, "Failed to put cpu buf: 0x%x",
			cmd_desc[i].mem_handle);
pwr_dwn:
	cam_ois_power_down(o_ctrl);
rel_pkt:
	if (cam_mem_put_cpu_buf(dev_config.packet_handle))
		CAM_WARN(CAM_OIS, "Fail in put buffer: 0x%x",
			dev_config.packet_handle);

	return rc;
}

void cam_ois_shutdown(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	struct cam_ois_soc_private *soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;

	if (o_ctrl->cam_ois_state == CAM_OIS_INIT)
		return;

	if (o_ctrl->cam_ois_state >= CAM_OIS_CONFIG) {
		rc = cam_ois_power_down(o_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "OIS Power down failed");
		o_ctrl->cam_ois_state = CAM_OIS_ACQUIRE;
	}

	if (o_ctrl->cam_ois_state >= CAM_OIS_ACQUIRE) {
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
	}

	if (o_ctrl->i2c_mode_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_mode_data);

	if (o_ctrl->i2c_calib_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_calib_data);

	if (o_ctrl->i2c_init_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_init_data);

	kfree(power_info->power_setting);
	kfree(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	power_info->power_down_setting_size = 0;
	power_info->power_setting_size = 0;

	o_ctrl->cam_ois_state = CAM_OIS_INIT;
}

/**
 * cam_ois_driver_cmd - Handle ois cmds
 * @e_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
int cam_ois_driver_cmd(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int                              rc = 0;
	struct cam_ois_query_cap_t       ois_cap = {0};
	struct cam_control              *cmd = (struct cam_control *)arg;
	struct cam_ois_soc_private      *soc_private = NULL;
	struct cam_sensor_power_ctrl_t  *power_info = NULL;

	if (!o_ctrl || !cmd) {
		CAM_ERR(CAM_OIS, "Invalid arguments");
		return -EINVAL;
	}

	if (cmd->handle_type != CAM_HANDLE_USER_POINTER) {
		CAM_ERR(CAM_OIS, "Invalid handle type: %d",
			cmd->handle_type);
		return -EINVAL;
	}

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

	mutex_lock(&(o_ctrl->ois_mutex));
	switch (cmd->op_code) {
	case CAM_QUERY_CAP:
		ois_cap.slot_info = o_ctrl->soc_info.index;

		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&ois_cap,
			sizeof(struct cam_ois_query_cap_t))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		CAM_DBG(CAM_OIS, "ois_cap: ID: %d", ois_cap.slot_info);
		break;
	case CAM_ACQUIRE_DEV:
		rc = cam_ois_get_dev_handle(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed to acquire dev");
			goto release_mutex;
		}

		o_ctrl->cam_ois_state = CAM_OIS_ACQUIRE;
		break;
	case CAM_START_DEV:
		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
			"Not in right state for start : %d",
			o_ctrl->cam_ois_state);
			goto release_mutex;
		}
		o_ctrl->cam_ois_state = CAM_OIS_START;
		break;
	case CAM_CONFIG_DEV:
		rc = cam_ois_pkt_parse(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed in ois pkt Parsing");
			goto release_mutex;
		}
		break;
	case CAM_RELEASE_DEV:
		if (o_ctrl->cam_ois_state == CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Cant release ois: in start state");
			goto release_mutex;
		}

#ifdef OIS_DEBUG_FUNC
	/* Check Gyro&Hall Status */
	OIS_Gyro_Hall_Check_STS(&o_ctrl->io_master_info);
	msleep(1);
	OIS_Gyro_Hall_Check_STS(&o_ctrl->io_master_info);
	msleep(1);
	OIS_Gyro_Hall_Check_STS(&o_ctrl->io_master_info);
#endif

		if (o_ctrl->cam_ois_state == CAM_OIS_CONFIG) {
			rc = cam_ois_power_down(o_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "OIS Power down failed");
				goto release_mutex;
			}
		}

		if (o_ctrl->bridge_intf.device_hdl == -1) {
			CAM_ERR(CAM_OIS, "link hdl: %d device hdl: %d",
				o_ctrl->bridge_intf.device_hdl,
				o_ctrl->bridge_intf.link_hdl);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
		o_ctrl->cam_ois_state = CAM_OIS_INIT;

		kfree(power_info->power_setting);
		kfree(power_info->power_down_setting);
		power_info->power_setting = NULL;
		power_info->power_down_setting = NULL;
		power_info->power_down_setting_size = 0;
		power_info->power_setting_size = 0;

		if (o_ctrl->i2c_mode_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_mode_data);

		if (o_ctrl->i2c_calib_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_calib_data);

		if (o_ctrl->i2c_init_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_init_data);

		break;
	case CAM_STOP_DEV:
		if (o_ctrl->cam_ois_state != CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
			"Not in right state for stop : %d",
			o_ctrl->cam_ois_state);
		}
		o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
		break;
	default:
		CAM_ERR(CAM_OIS, "invalid opcode");
		goto release_mutex;
	}
release_mutex:
	mutex_unlock(&(o_ctrl->ois_mutex));
	return rc;
}
