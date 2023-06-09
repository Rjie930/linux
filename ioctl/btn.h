#ifndef BTN_H
#define BT_H
#include <linux/ioctl.h>

#define TINY4412_BTN_TYPE 'f'

#define TINY4412_BTN_IORESET _IO(TINY4412_BTN_TYPE,1)
#define TINY4412_BTN_IOGET _IOR(TINY4412_BTN_TYPE,2,char)
#define TINY4412_BTN_IOSET _IOW(TINY4412_BTN_TYPE,3,char)

#define TINY4412_LED_ALL_OFF _IOW(TINY4412_BTN_TYPE,4,unsigned int)
#define TINY4412_LED_ALL_ON _IOW(TINY4412_BTN_TYPE,5,unsigned int)
#define TINY4412_LED_STATE_GET _IOR(TINY4412_BTN_TYPE,6,unsigned int)

#define TINY4412_BTN_MAX 6

#endif




