/*字符设备驱动代码模板，以后只要编写字符驱动程序，可以用下面的模板改，不需要从头开始编写*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include "btn.h"
#include <linux/io.h>

static unsigned char led_state=0;
static unsigned long gpm4con; 
#define GPM4BASE 0X110002E0 
#define GPM4CON *((volatile unsigned long *)(gpm4con + 0x0)) 
#define GPM4DAT *((volatile unsigned long *)(gpm4con + 0x4))

struct cdev *p_cdev;
dev_t dev;
struct class *led_class;

static unsigned char state=0;

ssize_t my_kernel_read(struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
	printk("read!\n");
	copy_to_user(buf,state,size);
	return 1;
}
ssize_t my_kernel_write(struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
	copy_from_user(&state,buf,size);
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

int key_value=0;


long my_kernel_ioctl(struct file *filp,unsigned int cmd,unsigned long arg){
	if(_IOC_NR(cmd)>TINY4412_BTN_MAX)
		return -EINVAL;
	if(_IOC_TYPE(cmd)!=TINY4412_BTN_TYPE)
		return -EINVAL;
	if(_IOC_DIR(cmd)==_IOC_READ)
	{
		if(!access_ok(VERIFY_WRITE,(void __user*)arg,_IOC_SIZE(cmd)))
			return -EINVAL;
	}
	else{
		if(!access_ok(VERIFY_READ,(void __user*)arg,_IOC_SIZE(cmd)))
			return -EINVAL;
	}
	switch(cmd){
	
	case TINY4412_BTN_IORESET:
		key_value=0;
		printk("TINY4412_BTN_IORESET\n");
		break;
		
	case TINY4412_BTN_IOGET:
		//copy_to_user((void __user*)arg,&key_value,sizeof(key_value));
		printk("TINY4412_BTN_IOGET\n");
		break;
		
	case TINY4412_BTN_IOSET:
		//copy_from_user(&key_value,(void __user*)arg,sizeof(key_value));
		printk("TINY4412_BTN_IOSET\n");
		break;
		
	case TINY4412_LED_ALL_OFF:
		copy_from_user(&led_state,(void __user*)arg,sizeof(led_state));
		GPM4DAT=led_state;
		printk("TINY4412_LED_ALL_OFF\n");
		break;
		
	case TINY4412_LED_ALL_ON:
		copy_from_user(&led_state,(void __user*)arg,sizeof(led_state));
		printk("TINY4412_LED_ALL_ON\n");
		GPM4DAT=led_state;
		break;
		
		case TINY4412_LED_STATE_GET:
		copy_to_user((void __user*)arg,&GPM4DAT,sizeof(GPM4DAT));
		printk("TINY4412_LED_STATE_GET\n");
		break;
		
	default:
		return -EINVAL;
	}
}

struct file_operations fops={
	.open=my_kernel_open,
	.release=my_kernel_release,
	.read=my_kernel_read,
	.write=my_kernel_write,
	.unlocked_ioctl=my_kernel_ioctl,
};

static int __init led_init(void)
{
	gpm4con = (unsigned int )ioremap((unsigned int)GPM4BASE,8);
	alloc_chrdev_region(&dev,0,1,"my_led_dev");
	p_cdev = cdev_alloc();
	cdev_init(p_cdev,&fops);
	cdev_add(p_cdev,dev,1);
	led_class=class_create(THIS_MODULE,"my_led_dev");
	device_create(led_class,NULL,dev,NULL,"my_led_dev");
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
