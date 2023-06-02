#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include <linux/io.h>
#include <linux/input.h>

#include <linux/irq.h> 
#include <linux/interrupt.h> //request_irq/free_irq 

#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/wait.h> 
#include <linux/sched.h>

//#include <linux/timer.h>

struct input_dev *btn_dev;

struct key_t{
	char *name;
	unsigned int gpio;
	unsigned int irqnum;
	unsigned int keyvalue;
};

struct key_t keys[] = {
	{"K1",EXYNOS4_GPX3(2),0,KEY_L},
	{"K2",EXYNOS4_GPX3(3),0,KEY_S},
	{"K3",EXYNOS4_GPX3(4),0,KEY_BACK},
	{"K4",EXYNOS4_GPX3(5),0,KEY_ENTER},
};

static void btn_timer_func(unsigned long data)
{
	struct key_t *p=(struct key_t*)data;
	unsigned int pinval=0;
	pinval=gpio_get_value(p->gpio);
	if(pinval==1)
	{
		input_event(btn_dev,EV_KEY,p->keyvalue,0);
		input_sync(btn_dev);
	}
	else{
		input_event(btn_dev,EV_KEY,p->keyvalue,1);
		input_sync(btn_dev);
	}
}

DEFINE_TIMER(btn_timer,btn_timer_func,0,0);

static irqreturn_t handler(int irq,void *dev)
{
	del_timer(&btn_timer);
	btn_timer.function=btn_timer_func;
	btn_timer.data=(unsigned long) dev;
	btn_timer.expires=jiffies+HZ/100;
	add_timer(&btn_timer);
	return IRQ_HANDLED;
}

static int __init led_init(void)
{
	btn_dev=input_allocate_device();
	btn_dev->name="btn_input";
	set_bit(EV_KEY,btn_dev->evbit);
	set_bit(EV_REP,btn_dev->evbit);

	int i;
	for (i=0;i<4;i++)
		set_bit(keys[i].keyvalue,btn_dev->keybit);
	input_register_device(btn_dev);
	
	for (i=0;i<4;i++)
		request_irq(gpio_to_irq(keys[i].gpio),handler,IRQ_TYPE_EDGE_BOTH,keys[i].name,&keys[i]);
	return 0;
}

static void __exit led_exit(void)
{
	int i;
	for (i=0;i<4;i++)
		free_irq(gpio_to_irq(keys[i].gpio),&keys[i]);
	input_unregister_device(btn_dev);
	input_free_device(btn_dev);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
