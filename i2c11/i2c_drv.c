#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
static int major;
static struct class *class;
static struct i2c_client *at24_client;
static ssize_t at24_read(struct file *file, char __user *buf, size_t count, loff_t *off)
{
    unsigned char addr, data;
    unsigned long len;

    len = copy_from_user(&addr, buf, 1);
    data = i2c_smbus_read_byte_data(at24_client, addr);
    len = copy_to_user(buf, &data, 1);
    return 1;
}
static ssize_t at24_write(struct file *file, const char __user *buf, size_t count, loff_t *off)
{
    unsigned char ker_buf[2];
    unsigned char addr, data;
    unsigned long len;
    len = copy_from_user(ker_buf, buf, 2);
    addr = ker_buf[0];
    data = ker_buf[1];
    if (!i2c_smbus_write_byte_data(at24_client, addr, data))
        return 2;
    else
        return -EIO;
}
static struct file_operations at24_fops = {
    .owner = THIS_MODULE,
    .read = at24_read,
    .write = at24_write,
};
static int __devinit at24_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    at24_client = client;
    major = register_chrdev(0, "at24", &at24_fops);
    class = class_create(THIS_MODULE, "at24");
    device_create(class, NULL, MKDEV(major, 0), NULL, "at24");
    return 0;
}
static int __devexit at24_remove(struct i2c_client *client)
{
    device_destroy(class, MKDEV(major, 0));
    class_destroy(class);
    unregister_chrdev(major, "at24");
    return 0;
}
static const struct i2c_device_id at24_id_table[] = {
    {"at24", 0}, // 名字
    {},
};
static struct i2c_driver at24_driver = {
    .driver = {
        .name = "sice",
        .owner = THIS_MODULE,
    },
    .probe = at24_probe,
    .remove = __devexit_p(at24_remove),
    .id_table = at24_id_table, // id 表
};
static int at24_drv_init(void)
{
    i2c_add_driver(&at24_driver); // 注册
    return 0;
}
static void at24_drv_exit(void)
{
    i2c_del_driver(&at24_driver); // 注销
}
module_init(at24_drv_init);
module_exit(at24_drv_exit);
MODULE_LICENSE("GPL");
