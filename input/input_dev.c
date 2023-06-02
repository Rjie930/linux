/*字符设备驱动代码模板，以后只要编写字符驱动程序，可以用下面的模板改，不需要从头开始编写*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include <linux/irq.h>
#include <linux/interrupt.h> //request_irq/free_irq
#include <linux/gpio.h> //gpio_to_irq
#include <mach/gpio.h> //宏定义
#include <linux/wait.h>
#include <linux/sched.h>

#include <linux/input.h>

#include <linux/timer.h>


struct btn_t{
	int gpio;
	int irqnum;
	int keyvalue;
	char *name;
};

static struct btn_t btns[]={
	{EXYNOS4_GPX3(2),0,KEY_L,"k1"},
	{EXYNOS4_GPX3(3),0,KEY_S,"k2"},
	{EXYNOS4_GPX3(4),0,KEY_BACK,"k3"},
	{EXYNOS4_GPX3(5),0,KEY_ENTER,"k4"},
};

struct input_dev *btn_dev;

static void dotimer_func(unsigned long data)
{
	struct btn_t *p = (struct btn_t*)data;
	unsigned int pinval = 0;
	pinval = gpio_get_value(p->gpio);
	if(pinval==1)
	{
		input_event(btn_dev,EV_KEY,p->keyvalue,0);
		input_sync(btn_dev);
	}else{
		input_event(btn_dev,EV_KEY,p->keyvalue,1);
		input_sync(btn_dev);
	}
}

DEFINE_TIMER(mytimer,dotimer_func,0,0);

static irqreturn_t btn_irq_handler(int irq,void *dev)
{
	del_timer(&mytimer);
	mytimer.function=dotimer_func;
	mytimer.expires=jiffies+HZ/200;
	mytimer.data = (unsigned long)dev;
	add_timer(&mytimer);
	return IRQ_HANDLED;
}

static int __init led_init(void)
{
	btn_dev=input_allocate_device();
	btn_dev->name="btn_input";
	set_bit(EV_KEY,btn_dev->evbit);
	set_bit(EV_REP,btn_dev->evbit);
	int i;
	for(i=0;i<4;i++)
		set_bit(btns[i].keyvalue,btn_dev->keybit);
	input_register_device(btn_dev);
	for(i=0;i<4;i++)
		request_irq(gpio_to_irq(btns[i].gpio),btn_irq_handler,IRQ_TYPE_EDGE_BOTH,btns[i].name,&btns[i]);
	return 0;
}

static void __exit led_exit(void)
{
	int i;
	for(i=0;i<4;i++)
		free_irq(gpio_to_irq(btns[i].gpio),&btns[i]);
	input_unregister_device(btn_dev);
	input_free_device(btn_dev);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
