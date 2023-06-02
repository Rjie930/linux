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
#include <linux/platform_device.h> //平台设备头文件


struct resource led_resource[] = {
    [0] = {
        .start = GPM4BASE,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = GPM4CON,
        .flags = IORESOURCE_MEM,
    }, 
    [2] = {
        .start = GPM4DAT,
        .flags = IORESOURCE_MEM,
    }
};
// release()函数
static void led_platform_release(struct device *dev)
{
    printk("led devices release!\n"); 
}

// 定义平台设备
static struct platform_device led_platform_device = {
    .name = "tiny4412 leds",
    .id = -1,
    .num_resources = ARRAY_SIZE(led_resource),
    .resource = led_resource,
    .dev = {
    .release = led_platform_release,
    },
};
// 模块入口
static int __init led_init(void)
{
    int ret;
    ret = platform_device_register(&led_platform_device);
    if (ret < 0) // 注册失败
    {
        // 销毁一个 platform_device，并释放所有与这个 platform_device 相关的内存
        platform_device_put(&led_platform_device);
        return ret;
    }
    return 0;
}
// 模块出口
static void __exit led_exit(void)
{
    platform_device_unregister(&led_platform_device);
}
// 模块声明
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("seig");