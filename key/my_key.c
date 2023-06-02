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
#include <linux/gpio.h> //gpio_to_irq
#include <mach/gpio.h> //宏定义
#include <linux/wait.h>
#include <linux/sched.h>
/***********************************
四个按键定义：
1. K1 -> GPX3_2/XINT26
2. K2 -> GPX3_3/XINT27
3. K3 -> GPX3_4/XINT28
4. K4 -> GPX3_5/XINT29
说明：已外置上拉，按下为低电平
**/
//定义数据结构
typedef struct{
unsigned int gpioNum; //引脚编号
int irqNum; //中断编号
char *keyName; //按键名称
unsigned char keyValue; //按键值
irq_handler_t handler; //中断处理
}key_int_t;
//中断函数声明
static irqreturn_t k1_handler(int irq,void *dev);
static irqreturn_t k2_handler(int irq,void *dev);
static irqreturn_t k3_handler(int irq,void *dev);
static irqreturn_t k4_handler(int irq,void *dev);
//定义数组
static char keyNameArray[4][20] = {"key1","key2","key3","key4"};
static irq_handler_t handlerArray[4]={k1_handler,k2_handler,k3_handler,k4_handler};
static key_int_t key[4] = {{0}}; //定义结构数组，并初始化为0
//结构数组初始化
static void key_array_init(void)
{
unsigned int i = 0;
for(i=0;i<4;i++)
{
key[i].gpioNum = EXYNOS4_GPX3(i+2);
key[i].irqNum=gpio_to_irq(key[i].gpioNum);
key[i].keyName=keyNameArray[i];
key[i].keyValue = i+1;
key[i].handler=handlerArray[i];
}
}
//底层函数
static int btn_open(struct inode *inode, struct file *file)
{
printk("btn_drv open!\n");
return 0;
}
static int btn_close(struct inode *inode, struct file *file)
{
printk("btn_drv close!\n");
return 0;
}
static unsigned char btn_status = 0; //用于保存按键的状态
static wait_queue_head_t wq;
static unsigned char condition = 0;
static ssize_t btn_read (struct file *filp, char __user *buf, size_t len, loff_t *offp)
{
long size = 0;
if(filp->f_flags & O_NONBLOCK)
return -EAGAIN;
wait_event_interruptible(wq,condition ); //条件为假，则休眠；为真，且产生中断信号，则唤醒
size = copy_to_user(buf,&btn_status,len);
condition = 0; //数据传递后，重新进入睡眠
return size;
}
static struct file_operations fops = {
.owner = THIS_MODULE,
.open = btn_open,
.release = btn_close,
.read = btn_read,
};
//模块部分
static dev_t dev = 0; //用于存储申请到的设备号
static struct class * btn_class;
static struct cdev * pcdev;
#define BTN_DRV_NAME "btn_drv"
static unsigned int err[4] = {0};
//模块入口
static int __init btn_init(void)
{
unsigned int i = 0;
key_array_init();
alloc_chrdev_region(&dev,0,1,BTN_DRV_NAME);//动态注册设备号
pcdev = cdev_alloc(); //申请cdev 结构空间
cdev_init(pcdev,&fops); //填充结构，建立与底层操作联系
pcdev->owner= THIS_MODULE;
cdev_add(pcdev,dev,1); //注册设备
//自动创建设备文件
btn_class = class_create(THIS_MODULE,"btn");
device_create(btn_class,NULL,dev,NULL,BTN_DRV_NAME);
//初始化等待队列
init_waitqueue_head(&wq);
//注册中断处理函数(不共享)
for(i = 0;i<4;i++)
{
err[i] = request_irq(key[i].irqNum,key[i].handler,IRQF_TRIGGER_FALLING,key[i].keyName,NULL);
if(err[i] != 0)
printk("k%d_interrupt register error!\n",i);
else
printk("k%d_interrupt registed!\n",i);
}
return 0;
}
//模块出口
static void __exit btn_exit(void)
{
unsigned int i = 0;
device_destroy(btn_class,dev);
class_destroy(btn_class);
cdev_del(pcdev);
unregister_chrdev_region(dev,1);
for(i=0;i<4;i++)
{
if(err[i] == 0)
{
free_irq(key[i].irqNum,NULL);
printk("k%d_interrupt unregisted!\n",i);
}
}
}
//模块声明
module_init(btn_init);
module_exit(btn_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("seig");
//中断处理函数实现
static irqreturn_t k1_handler(int irq,void *dev)
{
condition = 1; //表明中断产生
wake_up_interruptible(&wq); //唤醒等待的进程
btn_status = key[0].keyValue;
printk("K1 pressed!\n");
return IRQ_HANDLED;
}
static irqreturn_t k2_handler(int irq,void *dev)
{
condition = 1;
wake_up_interruptible(&wq);
btn_status = key[1].keyValue;
printk("K2 pressed!\n");
return IRQ_HANDLED;
}
static irqreturn_t k3_handler(int irq,void *dev)
{
condition = 1;
wake_up_interruptible(&wq);
btn_status = key[2].keyValue;
printk("K3 pressed!\n");
return IRQ_HANDLED;
}
static irqreturn_t k4_handler(int irq,void *dev)
{
condition = 1;
wake_up_interruptible(&wq);
btn_status = key[3].keyValue;
printk("K4 pressed!\n");
return IRQ_HANDLED;
}

