#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/slab.h>
static struct i2c_board_info at24_info = {
 I2C_BOARD_INFO("at24", 0x50 ),//at24 地址
};
static struct i2c_client *at24_client;
static int at24_dev_init(void)
{
 struct i2c_adapter *i2c_adap;
 i2c_adap = i2c_get_adapter(0);

 at24_client = i2c_new_device (i2c_adap, &at24_info);//注册
 i2c_put_adapter(i2c_adap);

 return 0;
}
static void at24_dev_exit(void)
{
 i2c_unregister_device (at24_client);//注销
}
module_init(at24_dev_init);
module_exit(at24_dev_exit);
MODULE_LICENSE("GPL");
