#include "cam_eeprom_dev.h"
#include "cam_req_mgr_dev.h"
#include "cam_eeprom_soc.h"
#include "cam_eeprom_core.h"
#include "cam_debug_util.h"
#include "cam_eeprom_dev.h"
#ifdef CONFIG_PRODUCT_HEART
#ifdef __EEPROM_RW_IF__
#include <linux/fs.h>
#include "cam_eeprom_data.h"
#include "cam_eeprom_rw_if.h"
#include "cam_sensor_cmn_header.h"

#if __EEPROM_RW_POWER_BY_KERNEL__
static cam_eeprom_rw_t* g_e_ctrl[MAX_CAM_ID];

int eeprom_rw_if_init(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int rc = 0;
	int i = 0;
	int cam_id = e_ctrl->soc_info.index;
	int len = sizeof(cam_eeprom_rw_t);
	int clk_len = sizeof(e_ctrl->soc_info.clk);
	int reg_len = sizeof(e_ctrl->soc_info.rgltr);

	if ((EEPROM_CALIBRATION_STORE_CAM_MAX_ID > MAX_CAM_ID) ||
		(cam_id > EEPROM_CALIBRATION_STORE_CAM_MAX_ID))
		return -EINVAL;

	CAM_DBG(CAM_EEPROM, "eeprom_rw_if_init");

	g_e_ctrl[cam_id] = vmalloc(len);
	if (g_e_ctrl[cam_id] != NULL) {
		g_e_ctrl[cam_id]->power_info.dev = vmalloc(sizeof(struct device));
		g_e_ctrl[cam_id]->power_info.power_setting = vmalloc(sizeof(struct cam_sensor_power_setting) *
			EEPROM_POWER_MAX_SIZE);
		g_e_ctrl[cam_id]->power_info.power_down_setting = vmalloc(sizeof(struct cam_sensor_power_setting) *
			EEPROM_POWER_MAX_SIZE);
		g_e_ctrl[cam_id]->power_info.gpio_num_info = vmalloc(sizeof(struct msm_camera_gpio_num_info) *
			EEPROM_POWER_MAX_SIZE);

		if (g_e_ctrl[cam_id]->power_info.dev == NULL || g_e_ctrl[cam_id]->power_info.power_setting == NULL ||
			g_e_ctrl[cam_id]->power_info.power_down_setting == NULL || g_e_ctrl[cam_id]->power_info.gpio_num_info == NULL) {
			rc = -EINVAL;
			goto FAILED;
		}

		g_e_ctrl[cam_id]->soc_info.pdev = vmalloc(sizeof(struct platform_device));
		g_e_ctrl[cam_id]->soc_info.dev = vmalloc(sizeof(struct device));
		g_e_ctrl[cam_id]->soc_info.dev_name = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
		g_e_ctrl[cam_id]->soc_info.irq_name = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
		g_e_ctrl[cam_id]->soc_info.irq_line = vmalloc(sizeof(struct resource));
		g_e_ctrl[cam_id]->soc_info.gpio_data = vmalloc(sizeof(struct cam_soc_gpio_data));
		g_e_ctrl[cam_id]->soc_info.dentry = vmalloc(sizeof(struct dentry));

		for (i = 0; i < CAM_SOC_MAX_CLK; i++) {
			g_e_ctrl[cam_id]->soc_info.clk_name[i] = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
			g_e_ctrl[cam_id]->soc_info.clk[i] = vmalloc(sizeof(clk_len));
			if (g_e_ctrl[cam_id]->soc_info.clk_name[i] == NULL ||
				g_e_ctrl[cam_id]->soc_info.clk[i] == NULL) {
				rc = -EINVAL;
				goto FAILED;
			}
		}

		for (i = 0; i < CAM_SOC_MAX_REGULATOR; i++) {
			g_e_ctrl[cam_id]->soc_info.rgltr_name[i] = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
			g_e_ctrl[cam_id]->soc_info.rgltr[i] = vmalloc(sizeof(reg_len));
			if (g_e_ctrl[cam_id]->soc_info.rgltr[i] == NULL ||
				g_e_ctrl[cam_id]->soc_info.rgltr_name[i] == NULL) {
				rc = -EINVAL;
				goto FAILED;
			}
		}

		if (g_e_ctrl[cam_id]->soc_info.pdev == NULL || g_e_ctrl[cam_id]->soc_info.dev == NULL ||
			g_e_ctrl[cam_id]->soc_info.dev_name == NULL || g_e_ctrl[cam_id]->soc_info.irq_name == NULL ||
			g_e_ctrl[cam_id]->soc_info.irq_line == NULL || g_e_ctrl[cam_id]->soc_info.gpio_data == NULL ||
			g_e_ctrl[cam_id]->soc_info.dentry == NULL) {
			rc = -EINVAL;
			goto FAILED;
		}
	} else {
		rc = -EINVAL;
		goto FAILED;
	}
	g_e_ctrl[cam_id]->state = MEM_OK;

	return rc;
FAILED:
	if (g_e_ctrl[cam_id]->power_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.dev);
	if (g_e_ctrl[cam_id]->power_info.power_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_setting);
	if (g_e_ctrl[cam_id]->power_info.power_down_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_down_setting);
	if (g_e_ctrl[cam_id]->power_info.gpio_num_info != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.gpio_num_info);
	if (g_e_ctrl[cam_id]->soc_info.pdev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.pdev);
	if (g_e_ctrl[cam_id]->soc_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev);
	if (g_e_ctrl[cam_id]->soc_info.dev_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_line != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_line);
	if (g_e_ctrl[cam_id]->soc_info.gpio_data)
		vfree(g_e_ctrl[cam_id]->soc_info.gpio_data);
	if (g_e_ctrl[cam_id]->soc_info.dentry != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dentry);

	for (i = 0; i < CAM_SOC_MAX_CLK; i++) {
		if (g_e_ctrl[cam_id]->soc_info.clk_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.clk[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk[i]);
	}

	for (i = 0; i < CAM_SOC_MAX_REGULATOR; i++) {
		if (g_e_ctrl[cam_id]->soc_info.rgltr_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.rgltr[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr[i]);
	}

	if (g_e_ctrl[cam_id] != NULL)
		vfree(g_e_ctrl[cam_id]);

	g_e_ctrl[cam_id]->state = MEM_FAILED;

	CAM_ERR(CAM_EEPROM, "eeprom_rw_if_init failed rc:%d", rc);

	return rc;
}

void eeprom_rw_if_release(int cam_id)
{
	int i = 0;

	if ((EEPROM_CALIBRATION_STORE_CAM_MAX_ID > MAX_CAM_ID) ||
		(cam_id > EEPROM_CALIBRATION_STORE_CAM_MAX_ID))
		return;

	if (g_e_ctrl[cam_id]->power_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.dev);
	if (g_e_ctrl[cam_id]->power_info.power_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_setting);
	if (g_e_ctrl[cam_id]->power_info.power_down_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_down_setting);
	if (g_e_ctrl[cam_id]->power_info.gpio_num_info != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.gpio_num_info);
	if (g_e_ctrl[cam_id]->soc_info.pdev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.pdev);
	if (g_e_ctrl[cam_id]->soc_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev);
	if (g_e_ctrl[cam_id]->soc_info.dev_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_line != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_line);
	if (g_e_ctrl[cam_id]->soc_info.clk_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.clk_name);
	if (g_e_ctrl[cam_id]->soc_info.clk != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.clk);
	if (g_e_ctrl[cam_id]->soc_info.gpio_data)
		vfree(g_e_ctrl[cam_id]->soc_info.gpio_data);
	if (g_e_ctrl[cam_id]->soc_info.dentry != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dentry);

	for (i = 0; i < CAM_SOC_MAX_CLK; i++) {
		if (g_e_ctrl[cam_id]->soc_info.clk_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.clk[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk[i]);
	}

	for (i = 0; i < CAM_SOC_MAX_REGULATOR; i++) {
		if (g_e_ctrl[cam_id]->soc_info.rgltr_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.rgltr[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr[i]);
	}
	if (g_e_ctrl[cam_id] != NULL)
		vfree(g_e_ctrl[cam_id]);
}

void set_eeprom_handle(struct cam_eeprom_ctrl_t *e_ctrl)
{
	struct cam_eeprom_soc_private  *soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;
	int cam_id = e_ctrl->soc_info.index;

	CAM_DBG(CAM_EEPROM, "set_eeprom_handle cam_id:%d %p power_info->power_setting->seq_type:%d",
		e_ctrl, cam_id, (power_info != NULL) ? power_info->power_setting->seq_type : -1);

	if ((EEPROM_CALIBRATION_STORE_CAM_MAX_ID > MAX_CAM_ID) ||
		(cam_id > EEPROM_CALIBRATION_STORE_CAM_MAX_ID) ||
		(g_e_ctrl[cam_id]->state == MEM_FAILED))
		return;

	memcpy(g_e_ctrl[cam_id]->power_info.dev, power_info->dev, sizeof(struct device));
	memcpy(g_e_ctrl[cam_id]->power_info.power_setting, power_info->power_setting,
		sizeof(struct cam_sensor_power_setting)*(power_info->power_setting_size));
	memcpy(g_e_ctrl[cam_id]->power_info.power_down_setting, power_info->power_down_setting,
		sizeof(struct cam_sensor_power_setting)*(power_info->power_down_setting_size));
	memcpy(g_e_ctrl[cam_id]->power_info.gpio_num_info, power_info->gpio_num_info,
		sizeof(struct msm_camera_gpio_num_info));

	g_e_ctrl[cam_id]->power_info.power_setting_size = power_info->power_setting_size;
	g_e_ctrl[cam_id]->power_info.power_down_setting_size = power_info->power_down_setting_size;
	g_e_ctrl[cam_id]->power_info.pinctrl_info = power_info->pinctrl_info;
	g_e_ctrl[cam_id]->soc_info = e_ctrl->soc_info;

}

cam_eeprom_rw_t* get_eeprom_handle(int cam_id)
{
	CAM_DBG(CAM_EEPROM, "get_eeprom_handle g_e_ctrl = %p cam_id: %d seq_type:%d", g_e_ctrl[cam_id], cam_id,
		 (g_e_ctrl[cam_id] != NULL) ? g_e_ctrl[cam_id]->power_info.power_setting->seq_type : SENSOR_SEQ_TYPE_MAX);

	return g_e_ctrl[cam_id];
}
#endif

#ifdef CHECK_SUM_CRC
char *data_count = NULL;

/*check_sum_CRC*/
static char *make_crc(void)
{
	return NULL;
}

static int get_crc_checksum(int dataCount, uint8_t *buffer)
{
	return 0;
}

static int write_crc_to_eeprom(cam_eeprom_ctrl_t *eeprom, uint8_t *buffer, uint32_t *cal_addr)
{
	int rc = 0;
	uint32_t crc_check_sum;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;

	if (cal_addr == NULL || eeprom == NULL || buffer == NULL)
		return -EINVAL;

	uint32_t cal_addr_h = *(cal_addr + 0);
	uint32_t cal_addr_l = *(cal_addr + 1);

	crc_check_sum = get_crc_checksum(CALIBRATION_DATA_SIZE, buffer);
	if (crc_check_sum <= 0) {
		CAM_DBG(CAM_EEPROM, "get_crc_checksum  Failed :%d", crc_check_sum);
	}

	i2c_reg_setting.reg_setting[0].reg_addr = cal_addr_h;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)((crc_check_sum & 0xFF00) >> 8);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;

	rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__,rc);
	}
	usleep_range(5000,5000);
	i2c_reg_setting.reg_setting[0].reg_addr = cal_addr_l;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)(crc_check_sum & 0x00FF);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__,rc);
	}
	usleep_range(5000,5000);
	CAM_DBG("crc_check_sum =%d", crc_check_sum);

	return rc;
}
#endif

#ifdef EEPROM_WRITE_PROTECT
static int eeprom_wp_control(struct cam_eeprom_ctrl_t *eeprom, bool flag)
{
	int rc = 0;
	uint32_t addr = 0;
	uint32_t value = 0;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array reg_write_setting[EEPROM_ONE_BURST_SIZE];

	if (flag) {
		addr = EEPROM_WRITE_PROTECT_ADDR;
		value = EEPROM_WRITE_PROTECT_ENABLE;
	} else {
		addr = EEPROM_WRITE_PROTECT_ADDR;
		value = EEPROM_WRITE_PROTECT_DISABLE;
	}

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	i2c_reg_setting.reg_setting[0].reg_addr = addr;
	i2c_reg_setting.reg_setting[0].reg_data =  value;
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;

	rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
	usleep_range(5000,5000);

	return rc;
}
#endif

int recalibration_file_rw(const char *fname, void *buf, size_t len, loff_t *offset)
{
	struct file *fp;
	int ret = 0;

	fp = filp_open(fname, O_RDWR | O_CREAT, 0666);
	if(IS_ERR(fp)) {
		CAM_ERR(CAM_EEPROM, "RST: %s error\n", __func__);
		return -1;
	}
	ret = kernel_read(fp, buf, len, offset);
	if (ret < 0) {
		CAM_ERR(CAM_EEPROM, "kernel read: %s error\n", __func__);
		return ret;
	}

	filp_close(fp, NULL);

	return ret;
}

static ssize_t read_eeprom_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int rc = 0;
	uint32_t i = 0;
	int read_eeprom_data = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);

	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif
	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_store cci_init failed");
		return -EINVAL;
	}

	for (i = 0; i < 2; i++) {
		rc = cam_cci_i2c_read(eeprom->io_master_info.cci_client,
			EEPROM_MASTER_CRC_ADDR_H + i, &read_eeprom_data,
			CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
			CAM_DBG(CAM_EEPROM, "check_sum_crc_master=0x%x", read_eeprom_data);
	}

	if (rc) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_store failed rc:%d", rc);
	}

	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_store cci_release failed");
		return -EINVAL;
	}

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif

	return 0;
}

static DEVICE_ATTR(read_eeprom, 0664, read_eeprom_show, NULL);

static ssize_t write_eeprom_store_master(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int rc = 0;
	uint32_t i = 0;
	loff_t offset = 0;
	uint32_t reg_addr = START_REGADDR_M;
	uint8_t *buffer_master;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);
#ifdef CHECK_SUM_CRC
	uint32_t cal_addr[] = {EEPROM_MASTER_CRC_ADDR_H, EEPROM_MASTER_CRC_ADDR_L};
#endif
	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master");

	buffer_master = vmalloc(CALIBRATION_DATA_SIZE);
	if (buffer_master == NULL) {
		rc = -ENOMEM;
		CAM_ERR(CAM_EEPROM, "vmalloc failed rc:%d", rc);
		return rc;
	}

	rc = recalibration_file_rw(RECALIBRATION_FILE_NAME_MASTER, buffer_master, CALIBRATION_DATA_SIZE, &offset);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "recalibration file failed rc:%d", rc);
	}
	usleep_range(5000,5000);
	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif
	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_init failed");
		return -EINVAL;
	}

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

#ifdef EEPROM_WRITE_PROTECT
	/*close eeprom write protect*/
	rc = eeprom_wp_control(eeprom, false);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif
	for (i = 0 ; i < CALIBRATION_DATA_SIZE; i++) {
		i2c_reg_setting.reg_setting[0].reg_addr = reg_addr++;
		i2c_reg_setting.reg_setting[0].reg_data = buffer_master[i];
		i2c_reg_setting.reg_setting[0].delay  = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;

		rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
		}

		usleep_range(5000,5000);
	}

#ifdef CHECK_SUM_CRC
	rc = write_crc_to_eeprom(eeprom, buffer_master, cal_addr);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed rc:%d",__func__, rc);
	}
#endif

#ifdef EEPROM_WRITE_PROTECT
	/*open eeprom write protect*/
	rc = eeprom_wp_control(eeprom, true);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif
	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store_master cci_init failed");
		return -EINVAL;
	}
#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif
	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master success");

	if (buffer_master)
		vfree(buffer_master);

	return 0;
}

static DEVICE_ATTR(write_eeprom_master, 0664, write_eeprom_store_master, NULL);//use 'cat' to trigger for windows env cmd

static ssize_t write_eeprom_store_slave(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int rc = 0;
	uint32_t i = 0;
	loff_t offset = 0;
	uint32_t reg_addr = START_REGADDR_S;
	uint8_t *buffer_slave;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);
#ifdef CHECK_SUM_CRC
	uint32_t cal_addr[] = {EEPROM_SLAVE_CRC_ADDR_H, EEPROM_SLAVE_CRC_ADDR_L};
#endif

	CAM_DBG(CAM_EEPROM, "write_eeprom_store_slave");
	buffer_slave = vmalloc(CALIBRATION_DATA_SIZE);
	if (buffer_slave == NULL) {
		rc = -ENOMEM;
		CAM_ERR(CAM_EEPROM, "vmalloc failed");
	}

	recalibration_file_rw(RECALIBRATION_FILE_NAME_SLAVE, buffer_slave, CALIBRATION_DATA_SIZE, &offset);
	usleep_range(5000, 5000);
	eeprom->io_master_info.cci_client->cci_i2c_master = 1;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_S;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_1);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif
	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_init failed");
		return -EINVAL;
	}
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size =1 ;

#ifdef EEPROM_WRITE_PROTECT
	/*close eeprom write protect*/
	rc = eeprom_wp_control(eeprom, false);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif

	for (i = 0 ; i < CALIBRATION_DATA_SIZE; i++){
		i2c_reg_setting.reg_setting[0].reg_addr = reg_addr++;
		i2c_reg_setting.reg_setting[0].reg_data = buffer_slave[i];
		i2c_reg_setting.reg_setting[0].delay  = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;

		rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if(rc < 0){
			CAM_ERR(CAM_EEPROM, " %s: write failed :%d",__func__,rc);
		}
		usleep_range(5000,5000);
	}

#ifdef CHECK_SUM_CRC
	rc = write_crc_to_eeprom(eeprom, buffer_slave, cal_addr);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed rc:%d",__func__, rc);
	}
#endif

#ifdef EEPROM_WRITE_PROTECT
	/*open eeprom write protect*/
	rc = eeprom_wp_control(eeprom, true);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif

	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store_slave cci_init failed");
		return -EINVAL;
	}

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_1);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif
	if (buffer_slave)
		vfree(buffer_slave);

	return count;
}

static DEVICE_ATTR(write_eeprom_slave, 0664, NULL, write_eeprom_store_slave);

static struct attribute *eeprom_attrs[] = {
	&dev_attr_read_eeprom.attr,
	&dev_attr_write_eeprom_master.attr,
	&dev_attr_write_eeprom_slave.attr,
	NULL
};

static const struct attribute_group eeprom_attr_group = {
	.name = "eeprom_rw_if",
	.attrs = eeprom_attrs,
};

int eeprom_rw_if_sysfs_create(struct kobject *kobj)
{
	int rc = 0;

	rc = sysfs_create_group(kobj, &eeprom_attr_group);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "Create eeprom rw if sysfs group failed, error: %d\n",
			rc);
		return rc;
	}
	return rc;
}
#endif

#elif defined (CONFIG_PRODUCT_ZIPPO)

#ifdef __EEPROM_RW_IF__
#include <linux/fs.h>
#include "cam_eeprom_data.h"
#include "cam_eeprom_rw_if.h"
#include "cam_sensor_cmn_header.h"

#define EEPROM_WRITE_PROTECT

#if __EEPROM_RW_POWER_BY_KERNEL__
static cam_eeprom_rw_t* g_e_ctrl[MAX_CAM_ID];

int eeprom_rw_if_init(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int rc = 0;
	int i = 0;
	int cam_id = e_ctrl->soc_info.index;
	int len = sizeof(cam_eeprom_rw_t);
	int clk_len = sizeof(e_ctrl->soc_info.clk);
	int reg_len = sizeof(e_ctrl->soc_info.rgltr);

	if ((EEPROM_CALIBRATION_STORE_CAM_MAX_ID > MAX_CAM_ID) ||
		(cam_id > EEPROM_CALIBRATION_STORE_CAM_MAX_ID))
		return -EINVAL;

	CAM_DBG(CAM_EEPROM, "eeprom_rw_if_init");

	g_e_ctrl[cam_id] = vmalloc(len);
	if (g_e_ctrl[cam_id] != NULL) {
		g_e_ctrl[cam_id]->power_info.dev = vmalloc(sizeof(struct device));
		g_e_ctrl[cam_id]->power_info.power_setting = vmalloc(sizeof(struct cam_sensor_power_setting) *
			EEPROM_POWER_MAX_SIZE);
		g_e_ctrl[cam_id]->power_info.power_down_setting = vmalloc(sizeof(struct cam_sensor_power_setting) *
			EEPROM_POWER_MAX_SIZE);
		g_e_ctrl[cam_id]->power_info.gpio_num_info = vmalloc(sizeof(struct msm_camera_gpio_num_info) *
			EEPROM_POWER_MAX_SIZE);

		if (g_e_ctrl[cam_id]->power_info.dev == NULL || g_e_ctrl[cam_id]->power_info.power_setting == NULL ||
			g_e_ctrl[cam_id]->power_info.power_down_setting == NULL || g_e_ctrl[cam_id]->power_info.gpio_num_info == NULL) {
			rc = -EINVAL;
			goto FAILED;
		}

		g_e_ctrl[cam_id]->soc_info.pdev = vmalloc(sizeof(struct platform_device));
		g_e_ctrl[cam_id]->soc_info.dev = vmalloc(sizeof(struct device));
		g_e_ctrl[cam_id]->soc_info.dev_name = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
		g_e_ctrl[cam_id]->soc_info.irq_name = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
		g_e_ctrl[cam_id]->soc_info.irq_line = vmalloc(sizeof(struct resource));
		g_e_ctrl[cam_id]->soc_info.gpio_data = vmalloc(sizeof(struct cam_soc_gpio_data));
		g_e_ctrl[cam_id]->soc_info.dentry = vmalloc(sizeof(struct dentry));

		for (i = 0; i < CAM_SOC_MAX_CLK; i++) {
			g_e_ctrl[cam_id]->soc_info.clk_name[i] = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
			g_e_ctrl[cam_id]->soc_info.clk[i] = vmalloc(sizeof(clk_len));
			if (g_e_ctrl[cam_id]->soc_info.clk_name[i] == NULL ||
				g_e_ctrl[cam_id]->soc_info.clk[i] == NULL) {
				rc = -EINVAL;
				goto FAILED;
			}
		}

		for (i = 0; i < CAM_SOC_MAX_REGULATOR; i++) {
			g_e_ctrl[cam_id]->soc_info.rgltr_name[i] = vmalloc(sizeof(char) * EEPROM_DEV_NAME_MAX_SIZE);
			g_e_ctrl[cam_id]->soc_info.rgltr[i] = vmalloc(sizeof(reg_len));
			if (g_e_ctrl[cam_id]->soc_info.rgltr[i] == NULL ||
				g_e_ctrl[cam_id]->soc_info.rgltr_name[i] == NULL) {
				rc = -EINVAL;
				goto FAILED;
			}
		}

		if (g_e_ctrl[cam_id]->soc_info.pdev == NULL || g_e_ctrl[cam_id]->soc_info.dev == NULL ||
			g_e_ctrl[cam_id]->soc_info.dev_name == NULL || g_e_ctrl[cam_id]->soc_info.irq_name == NULL ||
			g_e_ctrl[cam_id]->soc_info.irq_line == NULL || g_e_ctrl[cam_id]->soc_info.gpio_data == NULL ||
			g_e_ctrl[cam_id]->soc_info.dentry == NULL) {
			rc = -EINVAL;
			goto FAILED;
		}
	} else {
		rc = -EINVAL;
		goto FAILED;
	}
	g_e_ctrl[cam_id]->state = MEM_OK;

	return rc;
FAILED:
	if (g_e_ctrl[cam_id]->power_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.dev);
	if (g_e_ctrl[cam_id]->power_info.power_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_setting);
	if (g_e_ctrl[cam_id]->power_info.power_down_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_down_setting);
	if (g_e_ctrl[cam_id]->power_info.gpio_num_info != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.gpio_num_info);
	if (g_e_ctrl[cam_id]->soc_info.pdev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.pdev);
	if (g_e_ctrl[cam_id]->soc_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev);
	if (g_e_ctrl[cam_id]->soc_info.dev_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_line != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_line);
	if (g_e_ctrl[cam_id]->soc_info.gpio_data)
		vfree(g_e_ctrl[cam_id]->soc_info.gpio_data);
	if (g_e_ctrl[cam_id]->soc_info.dentry != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dentry);

	for (i = 0; i < CAM_SOC_MAX_CLK; i++) {
		if (g_e_ctrl[cam_id]->soc_info.clk_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.clk[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk[i]);
	}

	for (i = 0; i < CAM_SOC_MAX_REGULATOR; i++) {
		if (g_e_ctrl[cam_id]->soc_info.rgltr_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.rgltr[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr[i]);
	}

	if (g_e_ctrl[cam_id] != NULL)
		vfree(g_e_ctrl[cam_id]);

	g_e_ctrl[cam_id]->state = MEM_FAILED;

	CAM_ERR(CAM_EEPROM, "eeprom_rw_if_init failed rc:%d", rc);

	return rc;
}

void eeprom_rw_if_release(int cam_id)
{
	int i = 0;

	if ((EEPROM_CALIBRATION_STORE_CAM_MAX_ID > MAX_CAM_ID) ||
		(cam_id > EEPROM_CALIBRATION_STORE_CAM_MAX_ID))
		return;

	if (g_e_ctrl[cam_id]->power_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.dev);
	if (g_e_ctrl[cam_id]->power_info.power_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_setting);
	if (g_e_ctrl[cam_id]->power_info.power_down_setting != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.power_down_setting);
	if (g_e_ctrl[cam_id]->power_info.gpio_num_info != NULL)
		vfree(g_e_ctrl[cam_id]->power_info.gpio_num_info);
	if (g_e_ctrl[cam_id]->soc_info.pdev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.pdev);
	if (g_e_ctrl[cam_id]->soc_info.dev != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev);
	if (g_e_ctrl[cam_id]->soc_info.dev_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dev_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_name);
	if (g_e_ctrl[cam_id]->soc_info.irq_line != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.irq_line);
	if (g_e_ctrl[cam_id]->soc_info.clk_name != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.clk_name);
	if (g_e_ctrl[cam_id]->soc_info.clk != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.clk);
	if (g_e_ctrl[cam_id]->soc_info.gpio_data)
		vfree(g_e_ctrl[cam_id]->soc_info.gpio_data);
	if (g_e_ctrl[cam_id]->soc_info.dentry != NULL)
		vfree(g_e_ctrl[cam_id]->soc_info.dentry);

	for (i = 0; i < CAM_SOC_MAX_CLK; i++) {
		if (g_e_ctrl[cam_id]->soc_info.clk_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.clk[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.clk[i]);
	}

	for (i = 0; i < CAM_SOC_MAX_REGULATOR; i++) {
		if (g_e_ctrl[cam_id]->soc_info.rgltr_name[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr_name[i]);
		if (g_e_ctrl[cam_id]->soc_info.rgltr[i] != NULL)
			vfree(g_e_ctrl[cam_id]->soc_info.rgltr[i]);
	}
	if (g_e_ctrl[cam_id] != NULL)
		vfree(g_e_ctrl[cam_id]);
}

void set_eeprom_handle(struct cam_eeprom_ctrl_t *e_ctrl)
{
	struct cam_eeprom_soc_private  *soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;
	int cam_id = e_ctrl->soc_info.index;

	CAM_DBG(CAM_EEPROM, "set_eeprom_handle cam_id:%d %p power_info->power_setting->seq_type:%d",
		e_ctrl, cam_id, (power_info != NULL) ? power_info->power_setting->seq_type : -1);

	if ((EEPROM_CALIBRATION_STORE_CAM_MAX_ID > MAX_CAM_ID) ||
		(cam_id > EEPROM_CALIBRATION_STORE_CAM_MAX_ID) ||
		(g_e_ctrl[cam_id]->state == MEM_FAILED))
		return;

	memcpy(g_e_ctrl[cam_id]->power_info.dev, power_info->dev, sizeof(struct device));
	memcpy(g_e_ctrl[cam_id]->power_info.power_setting, power_info->power_setting,
		sizeof(struct cam_sensor_power_setting)*(power_info->power_setting_size));
	memcpy(g_e_ctrl[cam_id]->power_info.power_down_setting, power_info->power_down_setting,
		sizeof(struct cam_sensor_power_setting)*(power_info->power_down_setting_size));
	memcpy(g_e_ctrl[cam_id]->power_info.gpio_num_info, power_info->gpio_num_info,
		sizeof(struct msm_camera_gpio_num_info));

	g_e_ctrl[cam_id]->power_info.power_setting_size = power_info->power_setting_size;
	g_e_ctrl[cam_id]->power_info.power_down_setting_size = power_info->power_down_setting_size;
	g_e_ctrl[cam_id]->power_info.pinctrl_info = power_info->pinctrl_info;
	g_e_ctrl[cam_id]->soc_info = e_ctrl->soc_info;

}

cam_eeprom_rw_t* get_eeprom_handle(int cam_id)
{
	CAM_DBG(CAM_EEPROM, "get_eeprom_handle g_e_ctrl = %p cam_id: %d seq_type:%d", g_e_ctrl[cam_id], cam_id,
		 (g_e_ctrl[cam_id] != NULL) ? g_e_ctrl[cam_id]->power_info.power_setting->seq_type : SENSOR_SEQ_TYPE_MAX);

	return g_e_ctrl[cam_id];
}
#endif

#ifdef CHECK_SUM_CRC
char *data_count = NULL;

/*check_sum_CRC*/
static char *make_crc(void)
{
	return NULL;
}

static int get_crc_checksum(int dataCount, uint8_t *buffer)
{
	return 0;
}

static int write_crc_to_eeprom(cam_eeprom_ctrl_t *eeprom, uint8_t *buffer, uint32_t *cal_addr)
{
	int rc = 0;
	uint32_t crc_check_sum;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;

	if (cal_addr == NULL || eeprom == NULL || buffer == NULL)
		return -EINVAL;

	uint32_t cal_addr_h = *(cal_addr + 0);
	uint32_t cal_addr_l = *(cal_addr + 1);

	crc_check_sum = get_crc_checksum(CALIBRATION_DATA_SIZE, buffer);
	if (crc_check_sum <= 0) {
		CAM_DBG(CAM_EEPROM, "get_crc_checksum  Failed :%d", crc_check_sum);
	}

	i2c_reg_setting.reg_setting[0].reg_addr = cal_addr_h;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)((crc_check_sum & 0xFF00) >> 8);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;

	rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__,rc);
	}
	usleep_range(5000,5000);
	i2c_reg_setting.reg_setting[0].reg_addr = cal_addr_l;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)(crc_check_sum & 0x00FF);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__,rc);
	}
	usleep_range(5000,5000);
	CAM_DBG("crc_check_sum =%d", crc_check_sum);

	return rc;
}
#endif

int recalibration_file_rw(const char *fname, void *buf, size_t len, loff_t *offset)
{
	struct file *fp;
	int ret = 0;

	fp = filp_open(fname, O_RDWR | O_CREAT, 0666);
	if(IS_ERR(fp)) {
		CAM_ERR(CAM_EEPROM, "RST: %s error\n", __func__);
		return -1;
	}
	ret = kernel_read(fp, buf, len, offset);
	if (ret < 0) {
		CAM_ERR(CAM_EEPROM, "kernel read: %s error\n", __func__);
		return ret;
	}

	filp_close(fp, NULL);

	return ret;
}

static int read_rewrite_eeprom_data_from_file(uint8_t *buffer_master, uint32_t len)
{
	int rc = 0;
	loff_t offset = 0;

	CAM_DBG(CAM_EEPROM, "read_eeprom_data_from_file");

	rc = recalibration_file_rw(EEPROM_DATA_REWRITE_FILE_NAME_MASTER1, buffer_master, len, &offset);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "recalibration file failed rc:%d", rc);
	}
	usleep_range(5000,5000);

	CAM_DBG(CAM_EEPROM, "read_eeprom_data success");

	return rc;
}

#ifdef EEPROM_WRITE_PROTECT
static int read_eeprom_data_from_file(uint8_t *buffer_master, uint32_t len)
{
	int rc = 0;
	loff_t offset = 0;

	CAM_DBG(CAM_EEPROM, "read_eeprom_data_from_file");

	rc = recalibration_file_rw(EEPROM_DATA_FILE_NAME_MASTER1, buffer_master, len, &offset);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "recalibration file failed rc:%d", rc);
	}
	usleep_range(5000,5000);

	CAM_DBG(CAM_EEPROM, "read_eeprom_data success");

	return rc;
}

static int eeprom_wp_control(struct cam_eeprom_ctrl_t *eeprom, bool flag)
{
	int rc = 0;
	uint32_t addr = 0;
	uint32_t value = 0;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];

	if (flag) {
		value = EEPROM_WRITE_PROTECT_ENABLE;
	} else {
		value = EEPROM_WRITE_PROTECT_DISABLE;
	}
	addr = EEPROM_WRITE_PROTECT_ADDR;

	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_VCM;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 1;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	i2c_reg_setting.reg_setting[0].reg_addr = addr;
	i2c_reg_setting.reg_setting[0].reg_data =  value;
	i2c_reg_setting.reg_setting[0].delay  = 1;
	i2c_reg_setting.reg_setting[0].data_mask =0;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "eeprom_wp_control cci_init failed");
		return rc;
	}

	rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
	usleep_range(5000,5000);

	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "eeprom_wp_control cci_release failed");
		return rc;
	}

	return rc;
}

static int eeprom_erase_control(struct cam_eeprom_ctrl_t *eeprom)
{
	int rc = 0;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];

	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	//erase operation
	i2c_reg_setting.reg_setting[0].reg_addr = 0x81;
	i2c_reg_setting.reg_setting[0].reg_data = 0xEE;
	i2c_reg_setting.reg_setting[0].delay = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "eeprom_wp_control cci_init failed");
		return rc;
	}

	rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
	usleep_range(15000,15000);

	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "eeprom_wp_control cci_release failed");
		return rc;
	}

	return rc;
}

#endif

static ssize_t read_eeprom_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int rc = 0;
	uint32_t i = 0;
	int read_eeprom_data = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);

	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif
	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_store cci_init failed");
		return -EINVAL;
	}

	for (i = 0; i < 2; i++) {
		rc = cam_cci_i2c_read(eeprom->io_master_info.cci_client,
			EEPROM_MASTER1_CRC_ADDR_H + i, &read_eeprom_data,
			CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
			CAM_DBG(CAM_EEPROM, "check_sum_crc_master_48_16 =0x%x", read_eeprom_data);
	}

	if (rc) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_store failed rc:%d", rc);
	}

	for (i = 0; i < 2; i++) {
		rc = cam_cci_i2c_read(eeprom->io_master_info.cci_client,
			EEPROM_MASTER2_CRC_ADDR_H + i, &read_eeprom_data,
			CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
			CAM_DBG(CAM_EEPROM, "check_sum_crc_master_48_8 =0x%x", read_eeprom_data);
	}

	if (rc) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_store failed rc:%d", rc);
	}

	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "read_eeprom_store cci_release failed");
		return -EINVAL;
	}

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif

	return 0;
}

static DEVICE_ATTR(read_eeprom, 0664, read_eeprom_show, NULL);

static ssize_t write_eeprom_store_master1(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int rc = 0;
	uint32_t i = 0;
	loff_t offset = 0;
	uint8_t *buffer_master = NULL;
#ifdef EEPROM_WRITE_PROTECT
	uint32_t j = 0;
	uint32_t k = 0;
	uint8_t *buffer_all = NULL;
#else
	uint32_t reg_addr = START_REGADDR_M1;
#endif
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);
#ifdef CHECK_SUM_CRC
	uint32_t cal_addr[] = {EEPROM_MASTER1_CRC_ADDR_H, EEPROM_MASTER1_CRC_ADDR_L};
#endif
	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master");

	buffer_master = vmalloc(CALIBRATION_DATA_SIZE);
	if (buffer_master == NULL) {
		rc = -ENOMEM;
		CAM_ERR(CAM_EEPROM, "vmalloc failed rc:%d", rc);
		return rc;
	}

#ifdef EEPROM_WRITE_PROTECT
	buffer_all = vmalloc(EEPROM_DATA_SIZE);
	if (buffer_all == NULL) {
		rc = -ENOMEM;
		CAM_ERR(CAM_EEPROM, "vmalloc failed rc:%d", rc);
		return rc;
	}
#endif

	rc = recalibration_file_rw(RECALIBRATION_FILE_NAME_MASTER1, buffer_master, CALIBRATION_DATA_SIZE, &offset);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "recalibration file failed rc:%d", rc);
	}

	usleep_range(5000, 5000);

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif

#ifdef EEPROM_WRITE_PROTECT
	rc = read_eeprom_data_from_file(buffer_all, EEPROM_DATA_SIZE);
    if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: read failed :%d", __func__, rc);
		return rc;
    }

	/*close eeprom write protect*/
	rc = eeprom_wp_control(eeprom, true);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}

	for (i = 0; i < CALIBRATION_DATA_SIZE; i++) {
		buffer_all[START_REGADDR_M1_OFFSET + i] = buffer_master[i];
	}

	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_init failed");
		return -EINVAL;
	}

	for (j = 0; j < 3; j++) {
		//calculate ERASE HEADER
		k = (((((j & 0xFF) << 1) | 0x01) << 1) | ((0x1 << 7) & 0xFF)) & 0xFF;
		CAM_DBG(CAM_EEPROM, "%s: write reg_addr :0x%x",__func__, k);
		//erase operation
		i2c_reg_setting.reg_setting[0].reg_addr = k;
		i2c_reg_setting.reg_setting[0].reg_data = 0xEE;
		i2c_reg_setting.reg_setting[0].delay = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;
		i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
		i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;

		rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
		}
		usleep_range(15000, 15000);
		//erase operation end

		i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;

		for (i = 0; i < EEPROM_REG_PAGE_SIZE; i++) {
			i2c_reg_setting.reg_setting[0].reg_addr = j * EEPROM_REG_PAGE_SIZE + i;
			i2c_reg_setting.reg_setting[0].reg_data = buffer_all[j * EEPROM_REG_PAGE_SIZE + i];
			i2c_reg_setting.reg_setting[0].delay = 0;
			i2c_reg_setting.reg_setting[0].data_mask =0;

			rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
			}
			usleep_range(3000,3000);
		}
	}

	CAM_DBG(CAM_EEPROM, "debug: pos: %d ", j * EEPROM_REG_PAGE_SIZE + i);
#else
	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_init failed");
		return -EINVAL;
	}

	for (i = 0 ; i < CALIBRATION_DATA_SIZE; i++) {
		i2c_reg_setting.reg_setting[0].reg_addr = reg_addr++;
		i2c_reg_setting.reg_setting[0].reg_data = buffer_master[i];
		i2c_reg_setting.reg_setting[0].delay  = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;

		rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
		}
		usleep_range(25, 25);
	}
#endif

#ifdef CHECK_SUM_CRC
	rc = write_crc_to_eeprom(eeprom, buffer_master, cal_addr);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed rc:%d",__func__, rc);
	}
#endif

	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_release failed");
		return -EINVAL;
	}

#ifdef EEPROM_WRITE_PROTECT
	/*open eeprom write protect*/
	rc = eeprom_wp_control(eeprom, false);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif
	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master success");

	if (buffer_master)
		vfree(buffer_master);
#ifdef EEPROM_WRITE_PROTECT
	if (buffer_all)
		vfree(buffer_all);
#endif
	return 0;
}

static DEVICE_ATTR(write_eeprom_w48_u16, 0664, write_eeprom_store_master1, NULL);//use 'cat' to trigger for windows env cmd

static ssize_t write_eeprom_store_master2(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int rc = 0;
	uint32_t i = 0;
	loff_t offset = 0;
	uint8_t *buffer_master = NULL;
#ifdef EEPROM_WRITE_PROTECT
    uint32_t j = 0;
	int32_t k = 0;
    uint8_t *buffer_all = NULL;
#else
	uint32_t reg_addr = START_REGADDR_M2;
#endif
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);
#ifdef CHECK_SUM_CRC
	uint32_t cal_addr[] = {EEPROM_MASTER2_CRC_ADDR_H, EEPROM_MASTER2_CRC_ADDR_L};
#endif
	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master");

	buffer_master = vmalloc(CALIBRATION_DATA_SIZE);
	if (buffer_master == NULL) {
		rc = -ENOMEM;
		CAM_ERR(CAM_EEPROM, "vmalloc failed rc:%d", rc);
		return rc;
	}

#ifdef EEPROM_WRITE_PROTECT
		buffer_all = vmalloc(EEPROM_DATA_SIZE);
		if (buffer_all == NULL) {
			rc = -ENOMEM;
			CAM_ERR(CAM_EEPROM, "vmalloc failed rc:%d", rc);
			return rc;
		}
#endif
	rc = recalibration_file_rw(RECALIBRATION_FILE_NAME_MASTER2, buffer_master, CALIBRATION_DATA_SIZE, &offset);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "recalibration file failed rc:%d", rc);
	}
	usleep_range(5000, 5000);

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif

#ifdef EEPROM_WRITE_PROTECT
	rc = read_eeprom_data_from_file(buffer_all, EEPROM_DATA_SIZE);
    if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: read failed :%d", __func__, rc);
		return rc;
    }

	/*close eeprom write protect*/
	rc = eeprom_wp_control(eeprom, true);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}

	for (i = 0; i < CALIBRATION_DATA_SIZE; i++) {
		buffer_all[START_REGADDR_M2_OFFSET + i] = buffer_master[i];
	}

	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_init failed");
		return -EINVAL;
	}

	for (j = 4; j < 7; j++) {
		//calculate ERASE HEADER
		k = (((((j & 0xFF) << 1) | 0x01) << 1) | ((0x1 << 7) & 0xFF)) & 0xFF;
		CAM_DBG(CAM_EEPROM, "%s: write reg_addr :0x%x", __func__, k);
		//erase operation
		i2c_reg_setting.reg_setting[0].reg_addr = k;
		i2c_reg_setting.reg_setting[0].reg_data = 0xEE;
		i2c_reg_setting.reg_setting[0].delay = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;
		i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
		i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;

		rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
		}
		usleep_range(15000, 15000);
		//erase operation end

		i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
		i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;

		for (i = 0; i < EEPROM_REG_PAGE_SIZE; i++) {
			//if (i > 0xFF) {
				i2c_reg_setting.reg_setting[0].reg_addr = j * EEPROM_REG_PAGE_SIZE + i;//(((((i & 0x0300) >> 8)|((j & 0x07) << 2)) & 0xFF) << 8) | (i & 0xFF);
			/*
			} else {
				i2c_reg_setting.reg_setting[0].reg_addr = ((((j & 0x07) << 2) & 0xFF)<< 8) | (0xFF & i);
			}
			*/
			i2c_reg_setting.reg_setting[0].reg_data = buffer_all[j * EEPROM_REG_PAGE_SIZE + i];
			i2c_reg_setting.reg_setting[0].delay = 0;
			i2c_reg_setting.reg_setting[0].data_mask =0;

			rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
			}
			usleep_range(25, 25);
		}
	}
#else
	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_init failed");
		return -EINVAL;
	}

	for (i = 0 ; i < CALIBRATION_DATA_SIZE; i++) {
		i2c_reg_setting.reg_setting[0].reg_addr = reg_addr++;
		i2c_reg_setting.reg_setting[0].reg_data = buffer_master[i];
		i2c_reg_setting.reg_setting[0].delay  = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;

		rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
		}
		usleep_range(3000,3000);
	}
#endif

#ifdef CHECK_SUM_CRC
	rc = write_crc_to_eeprom(eeprom, buffer_master, cal_addr);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed rc:%d",__func__, rc);
	}
#endif

	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store cci_release failed");
		return -EINVAL;
	}

#ifdef EEPROM_WRITE_PROTECT
	/*open eeprom write protect*/
	rc = eeprom_wp_control(eeprom, false);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif
	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master success");

	if (buffer_master)
		vfree(buffer_master);
#ifdef EEPROM_WRITE_PROTECT
	if (buffer_all)
		vfree(buffer_all);
#endif
	return 0;
}
static DEVICE_ATTR(write_eeprom_w48_t8, 0664, write_eeprom_store_master2, NULL);//use 'cat' to trigger for windows env cmd

static ssize_t write_eeprom_store_all(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int rc = 0;
	uint32_t i = 0;
#ifdef EEPROM_WRITE_PROTECT
	uint32_t j = 0;
	uint8_t *buffer_all = NULL;
#endif
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);

	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master");

#ifdef EEPROM_WRITE_PROTECT
	buffer_all = vmalloc(EEPROM_DATA_SIZE);
	if (buffer_all == NULL) {
		rc = -ENOMEM;
		CAM_ERR(CAM_EEPROM, "vmalloc failed rc:%d", rc);
		return rc;
	}
#endif

	usleep_range(5000, 5000);

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif
	rc = read_rewrite_eeprom_data_from_file(buffer_all, EEPROM_DATA_SIZE);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: read failed :%d", __func__, rc);
		return rc;
	}

#ifdef EEPROM_WRITE_PROTECT
	/*close eeprom write protect*/
	rc = eeprom_wp_control(eeprom, true);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s:write failed :%d", __func__, rc);
		return rc;
	}

	rc = eeprom_erase_control(eeprom);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s:erase failed:%d", __func__, rc);
		return rc;
	}

	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 1;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "eeprom_wp_control cci_init failed");
		return -EINVAL;
	}

	for (j = 0; j < 8; j++) {
		for (i = 0; i < EEPROM_REG_PAGE_SIZE; i++) {
			i2c_reg_setting.reg_setting[0].reg_addr = j * EEPROM_REG_PAGE_SIZE + i;
			if ((j * EEPROM_REG_PAGE_SIZE + i) != i2c_reg_setting.reg_setting[0].reg_addr)
				CAM_ERR(CAM_EEPROM, "error value more than 0xFF :reg_addr:0x%x index:0x%x\n",
					i2c_reg_setting.reg_setting[0].reg_addr, (j * EEPROM_REG_PAGE_SIZE + i));
			i2c_reg_setting.reg_setting[0].reg_data = buffer_all[j * EEPROM_REG_PAGE_SIZE + i];
			i2c_reg_setting.reg_setting[0].delay = 0;
			i2c_reg_setting.reg_setting[0].data_mask =0;

			rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
			}
			usleep_range(3000, 3000);
		}
	}
#else
	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->cci_device = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = I2C_FAST_MODE;
	eeprom->io_master_info.master_type = CCI_MASTER;

	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;

	//erase operation
	i2c_reg_setting.reg_setting[0].reg_addr = 0x81;
	i2c_reg_setting.reg_setting[0].reg_data = 0xEE;
	i2c_reg_setting.reg_setting[0].delay = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;

	rc = camera_io_init(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "eeprom_wp_control cci_init failed");
		return -EINVAL;
	}

	for (j = 0; j < 8; j++)
	{
		for (i = 0; i < EEPROM_REG_PAGE_SIZE; i++) {
			if (i > 0xFF) {
				i2c_reg_setting.reg_setting[0].reg_addr = (((((i & 0x0300) >> 8)|((j & 0x07) << 2)) & 0xFF) << 8) | (i & 0xFF);
			} else {
				i2c_reg_setting.reg_setting[0].reg_addr = ((((j & 0x07) << 2) & 0xFF)<< 8) | (0xFF & i);
			}
			i2c_reg_setting.reg_setting[0].reg_data = buffer_all[j * EEPROM_REG_PAGE_SIZE + i];
			i2c_reg_setting.reg_setting[0].delay = 0;
			i2c_reg_setting.reg_setting[0].data_mask =0;

			rc = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
			if (rc < 0) {
				CAM_ERR(CAM_EEPROM, "%s: write failed :%d",__func__, rc);
			}
			usleep_range(3000,3000);
		}
	}
#endif
	rc = camera_io_release(&(eeprom->io_master_info));
	if (rc) {
		CAM_ERR(CAM_EEPROM, "eeprom_wp_control cci_release failed");
		return -EINVAL;
	}

#ifdef EEPROM_WRITE_PROTECT
	/*open eeprom write protect*/
	rc = eeprom_wp_control(eeprom, false);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif

	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master success");

	if (buffer_all)
		vfree(buffer_all);

	return 0;
}

static DEVICE_ATTR(write_eeprom_all, 0664, write_eeprom_store_all, NULL);//use 'cat' to trigger for windows env cmd

#ifdef ERASE_EEPROM_FOR_DEBUG
static ssize_t write_eeprom_erase_all(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int rc = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);

	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master");

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_up(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power up failed");
		return -EINVAL;
	}
#endif

#ifdef EEPROM_WRITE_PROTECT
	/*close eeprom write protect*/
	rc = eeprom_wp_control(eeprom, true);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}
#endif

	rc = eeprom_erase_control(eeprom);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "%s:erase failed:%d", __func__, rc);
		return rc;
	}

	/*open eeprom write protect*/
	rc = eeprom_wp_control(eeprom, false);
	if (rc < 0)
	{
		CAM_ERR(CAM_EEPROM, "%s: write failed :%d", __func__, rc);
		return rc;
	}

#if __EEPROM_RW_POWER_BY_KERNEL__
	rc = eeprom_rw_if_power_down(CAM_ID_0);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "write_eeprom_store power down failed");
		return -EINVAL;
	}
#endif

	CAM_DBG(CAM_EEPROM, "write_eeprom_store_master success");

	return 0;
}

static DEVICE_ATTR(erase_eeprom_all, 0664, write_eeprom_erase_all, NULL);//use 'cat' to trigger for windows env cmd
#endif

static struct attribute *eeprom_attrs[] = {
	&dev_attr_read_eeprom.attr,
	&dev_attr_write_eeprom_w48_u16.attr,
	&dev_attr_write_eeprom_w48_t8.attr,
	&dev_attr_write_eeprom_all.attr,
#ifdef ERASE_EEPROM_FOR_DEBUG
	&dev_attr_erase_eeprom_all.attr,
#endif
	NULL
};

static const struct attribute_group eeprom_attr_group = {
	.name = "eeprom_rw_if",
	.attrs = eeprom_attrs,
};

int eeprom_rw_if_sysfs_create(struct kobject *kobj)
{
	int rc = 0;

	rc = sysfs_create_group(kobj, &eeprom_attr_group);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "Create eeprom rw if sysfs group failed, error: %d\n",
			rc);
		return rc;
	}
	return rc;
}
#endif
#endif

