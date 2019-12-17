# Release Version 0.1.17
# Add config define "CONFIG_SND_SOC_DEBUSSY" to "y"
# Add to Makefile of prented folder: obj-$(CONFIG_SND_SOC_DEBUSSY) += debussy/

snd-soc-debussy-objs += debussy.o debussy_snd_ctrl.o debussy_kws.o
snd-soc-debussy-objs += debussy_intf.o debussy_intf_i2c.o
snd-soc-debussy-objs += debussy_flash.o debussy_psram.o
snd-soc-debussy-objs += debussy_spidev.o
snd-soc-debussy-objs += debussy_cdev.o

obj-$(CONFIG_SND_SOC_DEBUSSY) += snd-soc-debussy.o
#obj-y += snd-soc-debussy.o
