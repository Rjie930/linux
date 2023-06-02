/*字符设备驱动代码模板，以后只要编写字符驱动程序，可以用下面的模板改，不需要从头开始编写*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include <linux/poll.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/irq.h> 
#include <linux/interrupt.h>
#include <linux/wait.h> 
#include <linux/sched.h>

struct cdev *p_cdev;
dev_t dev;
struct class *led_class;

static unsigned char key_value=0;
static unsigned char key_pressed=0;
DECLARE_WAIT_QUEUE_HEAD(btn_wq);

static irqreturn_t btn_interrupt(int irq,void *dev)
{
key_value=1;
key_pressed=1;
wake_up_interruptible(&btn_wq);
return IRQ_HANDLED;
}

unsigned int my_kernel_poll(struct file *filp,struct poll_table_struct *table)
{
poll_wait(filp,&btn_wq,table);
if(key_pressed)
	return POLLIN;
else 
	return 0;
}

ssize_t my_kernel_read(struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
	printk("read!\n");
	wait_event_interruptible(btn_wq,key_pressed);
	key_pressed=0;
	copy_to_user(buf,&key_value,sizeof(key_value));
	return 1;
}
ssize_t my_kernel_write(struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
	copy_from_user(&key_value,buf,sizeof(key_value));
	printk("write!\n");
	return 1;
}
int my_kernel_open(struct inode *_inode, struct file *filp)
{
	printk("open!\n");
	return 0;
}
int my_kernel_release(struct inode *_inode, struct file *filp)
{
	printk("release!\n");
	return 0;
}

struct file_operations fops={
	.open=my_kernel_open,
	.release=my_kernel_release,
	.read=my_kernel_read,
	.write=my_kernel_write,
	.poll=my_kernel_poll,
};

static int __init led_init(void)
{
	int irq,err;
	irq=gpio_to_irq(EXYNOS4_GPX3(2));
	err=request_irq(irq,btn_interrupt,IRQF_TRIGGER_FALLING,"K1",NULL);
	
	alloc_chrdev_region(&dev,0,1,"key1_dev");
	p_cdev = cdev_alloc();
	cdev_init(p_cdev,&fops);
	cdev_add(p_cdev,dev,1);
	led_class=class_create(THIS_MODULE,"key1_dev");
	device_create(led_class,NULL,dev,NULL,"key1_dev");
	return 0;
}

static void __exit led_exit(void)
{
	int irq;
	irq=gpio_to_irq(EXYNOS4_GPX3(2));
	free_irq(irq,NULL);
	
	device_destroy(led_class,dev);
	class_destroy(led_class);
	cdev_del(p_cdev);
	unregister_chrdev_region(dev,1);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
