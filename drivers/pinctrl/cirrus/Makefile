# Cirrus Logic pinctrl drivers
pinctrl-madera-objs		:= pinctrl-madera-core.o
ifeq ($(CONFIG_PINCTRL_CS47L35),y)
pinctrl-madera-objs		+= pinctrl-cs47l35.o
endif

obj-$(CONFIG_PINCTRL_MADERA)	+= pinctrl-madera.o