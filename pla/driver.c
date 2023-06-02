#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h> //request_irq/free_irq
#include <linux/gpio.h>      //gpio_to_irq
#include <mach/gpio.h>       //宏定义

#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/platform_device.h>

// 模块部分
static dev_t dev = 0; // 用于存储申请到的设备号
static struct class *led_class;
static struct cdev *pcdev;

volatile unsigned long *GPM4BAS, *GPM4CON, *GPM4DAT;

// 底层函数
static int led_open(struct inode *inode, struct file *file)
{
    printk("led_drv open!\n");
    return 0;
}
static int led_close(struct inode *inode, struct file *file)
{
    printk("led_drv close!\n");
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t len, loff_t *offp)
{
    long size = 0;
    if (filp->f_flags & O_NONBLOCK)
        return -EAGAIN;
    size = copy_to_user(buf, GPM4DAT, len);
    return size;
}

ssize_t led_write(struct inode *inop,const char __user *buf,size_t size,loff_t f_pos)
{
 copy_from_user(&led_state,buf,size); //将用户数据送给内核变量
 *GPM4DAT = led_state; //用内核变量控制 LED
 printk("write led state!\n");
 return size;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_close,
    .read = led_read,
    .write=led_write,
};

// 将模块初始化中的内容写到 probe 方法中
static int __devinit led_platform_probe(struct platform_device *pdev)
{
    unsigned int i = 0;
    struct resource *led_resource;
    unsigned int keynumber = 0;
    led_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0); // 获取 IO 资源
    GPM4BASE = (unsigned int)key_resource->start;                  // 从 IO 资源中提取引脚编号
    led_resource = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    GPM4CON = GPM4BASE + (unsigned int)key_resource->start;
    led_resource = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    GPM4DAT = GPM4BASE + (unsigned int)key_resource->start;

    *GPM4CON = 0X1111;
    *GPM4DAT &= ~0XF;
    alloc_chrdev_region(&dev, 0, 1, "led_drv"); // 动态注册设备号
    pcdev = cdev_alloc();                       // 申请 cdev 结构空间
    cdev_init(pcdev, &fops);                    // 填充结构，建立与底层操作联系
    pcdev->owner = THIS_MODULE;
    cdev_add(pcdev, dev, 1); // 注册设备
    // 自动创建设备文件
    led_class = class_create(THIS_MODULE, "led");
    device_create(led_class, NULL, dev, NULL, led_DRV_NAME);
    return 0;
}
// 将模块退出函数中的内容写在 remove 方法中
static int __devexit led_platform_remove(struct platform_device *pdev)
{
    unsigned int i = 0;
    device_destroy(led_class, dev);
    class_destroy(led_class);
    cdev_del(pcdev);
    unregister_chrdev_region(dev, 1);
    return 0;
}
// 定义平台设备驱动
static struct platform_driver led_platform_driver = {
    .probe = led_platform_probe,
    .remove = led_platform_remove,
    .driver = {
        .name = "led",
        .owner = THIS_MODULE,
    },
};
// 模块入口
static int __init led_init(void)
{
    int ret = 0;
    ret = platform_driver_register(&led_platform_driver);
    return ret;
}
// 模块出口
static void __exit led_exit(void)
{
    platform_driver_unregister(&led_platform_driver);
}
// 模块声明
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("seig");