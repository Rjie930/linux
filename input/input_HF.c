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
#include <linux/input.h> //增加两个头文件
#include <linux/delay.h>
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
static irqreturn_t btn_handler(int irq,void *dev); //声明中断上半部
//定义数组
static char keyNameArray[4][20] = {"key1","key2","key3","key4"};
static unsigned short keyValueArray[4] = {KEY_A,KEY_B,KEY_C,KEY_D};
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
		key[i].keyValue = (unsigned char)keyValueArray[i]; //初始化按键值
		key[i].handler=btn_handler; //同一个中断入口
}
}
//模块部分
static unsigned int err[4] = {0};
struct input_dev *btn_dev = NULL;
//模块入口
static int __init btn_init(void)
{
	unsigned int i = 0;
	int ret = 0;
	key_array_init();
//申请空间
btn_dev = input_allocate_device();
if(btn_dev == NULL)
{
	printk("alloc error!\n");
	return -1;
}
btn_dev->name = "exynos4412 key"; //这里不同于设备文件名
set_bit(EV_KEY,btn_dev->evbit); //输入设备支持按键类型事件
for(i=0;i<4;i++)
	set_bit(key[i].keyValue,btn_dev->keybit); //输入设备支持的按键值
//设置按键对应的引脚为输入
for(i=0;i<4;i++)
{
	gpio_free(key[i].gpioNum);
	ret = gpio_request(key[i].gpioNum,key[i].keyName);
	if(ret)
	{
		printk("gpio %d request failed!\n",i);
	}
	else
		gpio_direction_input(key[i].gpioNum);
}
//注册中断处理函数(不共享)
for(i=0;i<4;i++)
{
//注册中断线，修改出发方式和参数5(用i 替换)
	err[i] = request_irq(key[i].irqNum,btn_handler,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
	key[i].keyName,(void *)i);
	if(err[i] != 0)
		printk("k%d_interrupt register error!\n",i);
	else
		printk("k%d_interrupt registed!\n",i);
	}
	input_register_device(btn_dev);
	return 0;
}
//模块出口
static void __exit btn_exit(void)
{
	unsigned int i = 0;
	input_unregister_device(btn_dev); //注销btn_dev
	input_free_device(btn_dev); //释放btn_dev 对应的内存空间
	for(i=0;i<4;i++)
	{
		if(err[i] == 0)
		{
			free_irq(key[i].irqNum,(void *)i);
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
static irqreturn_t btn_handler(int irq,void *dev)
{
int value = 0;
unsigned char temp = (unsigned char )dev;
if(temp > 3)
	return IRQ_NONE;
mdelay(10); //延时10ms,实际中应参考实验5 内核定时器推迟10ms
value = gpio_get_value(key[temp].gpioNum); //由引脚编号获取引脚电平
if(value == 1) //高电平（松手）
{
	input_report_key(btn_dev,key[temp].keyValue,0); //上报松手事件
	input_sync(btn_dev); //上报同步事件
}
else if(value == 0) //按下
{
	input_report_key(btn_dev,key[temp].keyValue,1); //上报按下事件
	input_sync(btn_dev);
}
return IRQ_HANDLED;
}
