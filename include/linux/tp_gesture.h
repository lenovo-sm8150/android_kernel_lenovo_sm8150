#ifndef _LINUX_TP_GESTURE_H
#define _LINUX_TP_GESTURE_H

#include <linux/device.h>

struct tp_dev{
	void (*update_status)(int);
};
extern void tp_gesture_register(struct tp_dev *opt);

#endif
