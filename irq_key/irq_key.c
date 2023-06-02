#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include <linux/io.h>

#include <linux/irq.h> 
#include <linux/interrupt.h> //request_irq/free_irq 

#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/wait.h> 
#include <linux/sched.h>

static unsigned long gpm4con; 
#define GPM4BASE 0X110002E0 
#define GPM4CON *((volatile unsigned long *)(gpm4con + 0x0)) 
#define GPM4DAT *((volatile unsigned long *)(gpm4con + 0x4))

static void btn_softirq_func(struct softirq_action *a);
static void btn_tasklet_func(unsigned long date);
static void btn_workq_func(struct work_struct *work);
struct cdev *p_cdev;
dev_t dev;
struct class *led_class;

static unsigned char state=0;

struct btn_t{
	int gpio;
	int keyvalue;
	char *name;

};

struct btn_t btns[] = {
	{EXYNOS4_GPX3(2),1,"K1"},
	{EXYNOS4_GPX3(3),2,"K2"},
	{EXYNOS4_GPX3(4),3,"K3"},
	{EXYNOS4_GPX3(5),4,"K4"},
};

DECLARE_WAIT_QUEUE_HEAD(btn_waitq);

int key_value = 0;
int ev_press = 0;

DECLARE_TASKLET(my_tasklet,btn_tasklet_func,0);
DECLARE_WORK(btn_work,btn_workq_func);

static void btn_softirq_func(struct softirq_action *a)
{
printk("K1 softirq!\n");
ev_press=1;
wake_up_interruptible(&btn_waitq);
}

static void btn_tasklet_func(unsigned long date)
{
printk("K2 tasklet!\n");
ev_press=1;
wake_up_interruptible(&btn_waitq);
}

static void btn_workq_func(struct work_struct *work)
{
printk("K3 workq!\n");
ev_press=1;
wake_up_interruptible(&btn_waitq);
}

ssize_t my_kernel_read(struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
	printk("read!\n");
	wait_event_interruptible(btn_waitq, ev_press);
	ev_press = 0;
	size = copy_to_user(buf,&key_value,sizeof(key_value));  
	return size;  //<-------return
}

ssize_t my_kernel_write(struct file *filp, const char __user *buf, 

size_t size, loff_t *offset)
{
	copy_from_user(&state,buf,size);
	printk("write!\n");
	GPM4DAT=state;
	return 1;
}

static void dotimer_func(unsigned long data)
{
struct btn_t *p=(struct btn_t*)data;
key_value=p->keyvalue;
ev_press=1;
//GPM4DAT^=0x1<<key_value-1;

wake_up_interruptible(&btn_waitq);
}

DEFINE_TIMER(mytimer,dotimer_func,0,0);

static irqreturn_t btn_irq_handler(int irq,void *dev)
{
del_timer(&mytimer);
mytimer.expires=jiffies+HZ/20;
mytimer.data=(unsigned long) dev;
add_timer(&mytimer);
	return IRQ_HANDLED;

}

int my_kernel_open(struct inode *_inode, struct file *filp)
{
	
	printk("open!\n");
	int irq;
	int i;
	for (i=0;i<4;i++)
	{
		irq = gpio_to_irq(btns[i].gpio);
		request_irq(irq,btn_irq_handler,IRQ_TYPE_EDGE_FALLING,btns[i].name,&btns[i]);
		
	}
	open_softirq(TINY4412_BTN_SOFTIRQ,btn_softirq_func);
	return 0;
}
int my_kernel_release(struct inode *_inode, struct file *filp)
{
	printk("release!\n");
	int irq;
	int i;
	for (i=0;i<4;i++)
	{
		irq = gpio_to_irq(btns[i].gpio);
		free_irq(irq,&btns[i]);
	}
	return 0;
}

struct file_operations fops={
	.open=my_kernel_open,
	.release=my_kernel_release,
	.read=my_kernel_read,
	.write=my_kernel_write,
};

static int __init led_init(void)
{

gpm4con = (unsigned int )ioremap((unsigned int)GPM4BASE,8);

	alloc_chrdev_region(&dev,0,1,"my_key_drv");
	p_cdev = cdev_alloc();
	cdev_init(p_cdev,&fops);
	cdev_add(p_cdev,dev,1);
	led_class=class_create(THIS_MODULE,"my_key_drv");
	device_create(led_class,NULL,dev,NULL,"my_key_drv");
	return 0;
}

static void __exit led_exit(void)
{
	device_destroy(led_class,dev);
	class_destroy(led_class);
	cdev_del(p_cdev);
	unregister_chrdev_region(dev,1);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
