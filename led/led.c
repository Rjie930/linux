/*字符设备驱动代码模板，以后只要编写字符驱动程序，可以用下面的模板改，不需要从头开始编写*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

struct cdev *p_cdev;
dev_t dev;
struct class *led_class;

unsigned long *gpm4con;
unsigned char *gpm4dat;

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

struct file_operations fops={
	.open=my_kernel_open,
	.release=my_kernel_release,
	.read=my_kernel_read,
	.write=my_kernel_write,
};

static int __init led_init(void)
{
    gpm4con=(unsigned long*) ioremap(0x110002e0,4);
    gpm4dat=(unsigned char*) ioremap(0x110002e4,1);
	alloc_chrdev_region(&dev,0,1,"led");
	p_cdev = cdev_alloc();
	cdev_init(p_cdev,&fops);
	cdev_add(p_cdev,dev,1);
	led_class=class_create(THIS_MODULE,"led");
	device_create(led_class,NULL,dev,NULL,"led");
	return 0;
}

static void __exit led_exit(void)
{
	iounmap(gpm4con);
	iounmap(gpm4dat);
	device_destroy(led_class,dev);
	class_destroy(led_class);
	cdev_del(p_cdev);
	unregister_chrdev_region(dev,1);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
