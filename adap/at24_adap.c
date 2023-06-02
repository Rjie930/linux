#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_i2c.h>
#include <linux/of_gpio.h>
#include <asm/irq.h>
#include <plat/iic.h>
#include <plat/gpio-cfg.h>
#include <mach/map.h>

// 根据前面寄存器地址，定义各寄存器相对于基地址的偏移量
#define TINY4412_IICCON 0x00
#define TINY4412_IICSTAT 0x04
#define TINY4412_IICADD 0x08
#define TINY4412_IICDS 0x0C
// 根据各寄存器的位功能，定义位掩码
#define TINY4412_IICCON_ACKEN (1 << 7)
#define TINY4412_IICCON_TXDIV_16 (0 << 6)  // fpclk/16
#define TINY4412_IICCON_TXDIV_512 (1 << 6) // fpclk/512
#define TINY4412_IICCON_IRQEN (1 << 5)
#define TINY4412_IICCON_IRQPEND (1 << 4)
#define TINY4412_IICCON_SCALE(x) ((x)&0xf) // IICCON 的低 4 位用于对 I2C 时钟进行 2 次分频
#define TINY4412_IICCON_SCALEMASK (0xf)
#define TINY4412_IICSTAT_MASTER_RX (2 << 6)
#define TINY4412_IICSTAT_MASTER_TX (3 << 6)
#define TINY4412_IICSTAT_SLAVE_RX (0 << 6)
#define TINY4412_IICSTAT_SLAVE_TX (1 << 6)
#define TINY4412_IICSTAT_MODEMASK (3 << 6)
#define TINY4412_IICSTAT_START (1 << 5)
#define TINY4412_IICSTAT_BUSBUSY (1 << 5)
#define TINY4412_IICSTAT_TXRXEN (1 << 4)
#define TINY4412_IICSTAT_ARBITR (1 << 3)
#define TINY4412_IICSTAT_ASSLAVE (1 << 2)
#define TINY4412_IICSTAT_ADDR0 (1 << 1)
#define TINY4412_IICSTAT_LASTBIT (1 << 0)
// 定义枚举类型，用于指示 i2c 当前的状态
enum tiny4412_i2c_state
{

    STATE_IDLE, // 0
    STATE_START,
    STATE_READ,
    STATE_WRITE,
    STATE_STOP
};
// 封装 tiny4412_i2c 结构,并定义指针对象 i2c
struct tiny4412_i2c
{
    wait_queue_head_t wait;
    struct i2c_msg *msg;
    unsigned int msg_num;
    unsigned int msg_idx;
    unsigned int msg_ptr;
    enum tiny4412_i2c_state state;
    void __iomem *regs;
    struct clk *clk;
    struct device *dev;
    struct i2c_adapter adap;
} *i2c;
// 禁止应答
static inline void tiny4412_i2c_disable_ack(struct tiny4412_i2c *i2c)
{
    unsigned long tmp;
    tmp = readl(i2c->regs + TINY4412_IICCON);                          // 读 IICCON 寄存器的值
    writel(tmp & ~TINY4412_IICCON_ACKEN, i2c->regs + TINY4412_IICCON); // 取消应答功能
}
// 应答位置 1，使能应答
static inline void tiny4412_i2c_enable_ack(struct tiny4412_i2c *i2c)
{

    unsigned long tmp;
    tmp = readl(i2c->regs + TINY4412_IICCON);
    writel(tmp | TINY4412_IICCON_ACKEN, i2c->regs + TINY4412_IICCON);
}
// 禁止 i2c 传输中断
static inline void tiny4412_i2c_disable_irq(struct tiny4412_i2c *i2c)
{
    unsigned long tmp;
    tmp = readl(i2c->regs + TINY4412_IICCON);
    writel(tmp & ~TINY4412_IICCON_IRQEN, i2c->regs + TINY4412_IICCON);
}
// 使能中断
static inline void tiny4412_i2c_enable_irq(struct tiny4412_i2c *i2c)
{
    unsigned long tmp;
    tmp = readl(i2c->regs + TINY4412_IICCON);
    writel(tmp | TINY4412_IICCON_IRQEN, i2c->regs + TINY4412_IICCON);
}
static inline void tiny4412_i2c_master_complete(struct tiny4412_i2c *i2c, int ret)
{
    i2c->msg_ptr = 0; // 传输完成后，消息索引归 0
    i2c->msg = NULL;  // 消息指针置空
    i2c->msg_idx++;   // 用于记录完成的消息数
    i2c->msg_num = 0; // 总消息数清 0
    if (ret)
        i2c->msg_idx = ret; // 根据 ret，实现对 msg_idx 的重置，否则每完成一次传输，该值加 1

    wake_up(&i2c->wait); // 唤醒队列
}
// 产生起动条件，开始进行数据传输
static void tiny4412_i2c_message_start(struct tiny4412_i2c *i2c, struct i2c_msg *msg)
{
    unsigned int addr = (msg->addr & 0x7f) << 1; // 地址字节为：地址（高 7 位）+读写方向位（最低位）
    unsigned long stat;
    unsigned long iiccon;
    stat = 0;                        // 清空变量，用于拼凑控制字
    stat |= TINY4412_IICSTAT_TXRXEN; // 先拼凑传输使能位
    if (msg->flags & I2C_M_RD)
    {                                       // 读操作
        stat |= TINY4412_IICSTAT_MASTER_RX; // 控制字中设置为主机接收
        addr |= 1;                          // 将地址字节最低位置 1（读），以便通知从机发数据
    }
    else
        stat |= TINY4412_IICSTAT_MASTER_TX;
    if (msg->flags & I2C_M_REV_DIR_ADDR)         // 当 flag 中含该标志时，说明适配器读写功能取反
        addr ^= 1;                               // 翻转地址字节最低位，以便从机也执行相反的传输操作
    tiny4412_i2c_enable_ack(i2c);                // 适配器（主机）使能应答功能
    iiccon = readl(i2c->regs + TINY4412_IICCON); // 先读 IICCON，以清除中断标记
    writel(stat, i2c->regs + TINY4412_IICSTAT);  // 设置状态寄存器
    writeb(addr, i2c->regs + TINY4412_IICDS);    // 填充地址字节到移位寄存器

    ndelay(50);

    writel(iiccon, i2c->regs + TINY4412_IICCON); // 重新写回原配置

    stat |= TINY4412_IICSTAT_START;
    writel(stat, i2c->regs + TINY4412_IICSTAT); // 产生启动条件，开始传输
}
// 产生停止条件
static inline void tiny4412_i2c_stop(struct tiny4412_i2c *i2c, int ret)
{
    unsigned long iicstat = readl(i2c->regs + TINY4412_IICSTAT);
    iicstat &= ~TINY4412_IICSTAT_START;
    writel(iicstat, i2c->regs + TINY4412_IICSTAT); // 产生停止条件
    i2c->state = STATE_STOP;                       // 同时标记 i2c 总线当前状态
    tiny4412_i2c_master_complete(i2c, ret);        // 完成传输后，重置 i2c 结构
    tiny4412_i2c_disable_irq(i2c);                 // 完成传输后，关中断
}
// 判断是否是最后一次传输
static inline int is_lastmsg(struct tiny4412_i2c *i2c)
{
    return i2c->msg_idx >= (i2c->msg_num - 1); // 通过完成传输的消息数跟要发送的消息数比较来判断
}

//
static inline int is_msglast(struct tiny4412_i2c *i2c)
{
    if (i2c->msg->flags & I2C_M_RECV_LEN && i2c->msg->len == 1) // 只发一条时，本就是最后一条
        return 0;
    return i2c->msg_ptr == i2c->msg->len - 1; // 判断消息索引位置是否为 len-1，是则表示最后一条消息
}
// 消息是否全部传输完成
static inline int is_msgend(struct tiny4412_i2c *i2c)
{
    return i2c->msg_ptr >= i2c->msg->len; // 判断原理 is_lastmsg（）
}
// 中断处理的回调函数
static int i2c_irq_nextbyte(struct tiny4412_i2c *i2c, unsigned long iicstat)
{
    unsigned long tmp;
    unsigned char byte;
    int ret = 0;
    switch (i2c->state)
    {
    case STATE_IDLE: // 总线空闲，则直接退出
        goto out;
    case STATE_STOP: // 总线产生条件后，则禁止中断
        tiny4412_i2c_disable_irq(i2c);
        goto out_ack; // 同时，清中断标记

    case STATE_START:
        if (iicstat & TINY4412_IICSTAT_LASTBIT && // iicstat 为 1，忽略应答位
            !(i2c->msg->flags & I2C_M_IGNORE_NAK))
        {                                   // I2C_M_IGNORE_NAK 也表示要忽略，注意！号
            tiny4412_i2c_stop(i2c, -ENXIO); // 条件不一致，则停止总线
            goto out_ack;
        }
        if (i2c->msg->flags & I2C_M_RD) // 读操作，标记状态为 STATE_READ
            i2c->state = STATE_READ;
        else
            i2c->state = STATE_WRITE; // 写操作
        if (is_lastmsg(i2c) && i2c->msg->len == 0)
        {
            tiny4412_i2c_stop(i2c, 0); // 是最后一字节，且写完，则停止总线
            goto out_ack;
        }
        if (i2c->state == STATE_READ) // 根据状态标记，执行相应动作
            goto prepare_read;
    case STATE_WRITE:
        if (!(i2c->msg->flags & I2C_M_IGNORE_NAK))
        {
            if (iicstat & TINY4412_IICSTAT_LASTBIT)
            {
                tiny4412_i2c_stop(i2c, -ECONNREFUSED);
                goto out_ack;
            }
        }
    retry_write:

        if (!is_msgend(i2c))
        { // 主机通过 IICDS 发数据
            byte = i2c->msg->buf[i2c->msg_ptr++];
            writeb(byte, i2c->regs + TINY4412_IICDS);
            ndelay(50);
        }
        else if (!is_lastmsg(i2c))
        {
            i2c->msg_ptr = 0;
            i2c->msg_idx++;
            i2c->msg++;
            if (i2c->msg->flags & I2C_M_NOSTART)
            {
                if (i2c->msg->flags & I2C_M_RD)
                {
                    tiny4412_i2c_stop(i2c, -EINVAL);
                }
                goto retry_write;
            }
            else
            {
                tiny4412_i2c_message_start(i2c, i2c->msg);
                i2c->state = STATE_START;
            }
        }
        else
        {
            tiny4412_i2c_stop(i2c, 0);
        }
        break;
    case STATE_READ:
        byte = readb(i2c->regs + TINY4412_IICDS); // 读取数据，将存储到 byte 变量中
        i2c->msg->buf[i2c->msg_ptr++] = byte;     // 转存到 msg 结构中
        if (i2c->msg->flags & I2C_M_RECV_LEN && i2c->msg->len == 1)
            i2c->msg->len += byte;

    prepare_read:
        if (is_msglast(i2c))
        {
            if (is_lastmsg(i2c))
                tiny4412_i2c_disable_ack(i2c); // 主机读最后一字节，回应不应答信号
        }
        else if (is_msgend(i2c))
        {
            if (is_lastmsg(i2c))
            {
                tiny4412_i2c_stop(i2c, 0); // 若读完，则产生停止条件
            }
            else
            {
                i2c->msg_ptr = 0;
                i2c->msg_idx++;
                i2c->msg++;
            }
        }
        break;
    }
out_ack:
    tmp = readl(i2c->regs + TINY4412_IICCON);
    tmp &= ~TINY4412_IICCON_IRQPEND;
    writel(tmp, i2c->regs + TINY4412_IICCON);
out:
    return ret;
}
// 中断处理函数
static irqreturn_t tiny4412_i2c_irq(int irqno, void *dev_id)
{
    struct tiny4412_i2c *i2c = dev_id;
    unsigned long status;
    unsigned long tmp;

    status = readl(i2c->regs + TINY4412_IICSTAT);
    if (i2c->state == STATE_IDLE)
    {
        tmp = readl(i2c->regs + TINY4412_IICCON);
        tmp &= ~TINY4412_IICCON_IRQPEND;
        writel(tmp, i2c->regs + TINY4412_IICCON);
        goto out;
    }
    i2c_irq_nextbyte(i2c, status); // 调用回调函数，执行各种操作
out:
    return IRQ_HANDLED;
}
// 忙等
static int tiny4412_i2c_wait_busy(struct tiny4412_i2c *i2c)
{
    unsigned long iicstat;
    int timeout = 400;
    while (timeout-- > 0)
    {
        iicstat = readl(i2c->regs + TINY4412_IICSTAT);
        if (!(iicstat & TINY4412_IICSTAT_BUSBUSY))
            return 0;
        msleep(1);
    }
    return -ETIMEDOUT;
}
// 数据收、发的实现
static int tiny4412_i2c_doxfer(struct tiny4412_i2c *i2c, struct i2c_msg *msgs, int num)
{
    int ret;
    tiny4412_i2c_wait_busy(i2c); // 等待总线空闲
    i2c->msg = msgs;
    i2c->msg_num = num;
    i2c->msg_ptr = 0;
    i2c->msg_idx = 0;
    i2c->state = STATE_START;
    tiny4412_i2c_enable_irq(i2c);                             // 开中断
    tiny4412_i2c_message_start(i2c, msgs);                    // 产生起始条件，执行传输
    wait_event_timeout(i2c->wait, i2c->msg_num == 0, HZ * 5); // 挂起 5 秒
    ret = i2c->msg_idx;

    return ret;
}
// master_xfer()接口实现
static int tiny4412_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    // 可见，用 algo_data 保存了 i2c 结构的首地址

    struct tiny4412_i2c *i2c = (struct tiny4412_i2c *)adap->algo_data;
    int ret;
    clk_enable(i2c->clk);                      // 开时钟，使能 i2c 控制器
    ret = tiny4412_i2c_doxfer(i2c, msgs, num); // 数据传输
    clk_disable(i2c->clk);                     // 传输后关闭控制器时钟

    return ret;
}
// functionality()接口实现
static u32 tiny4412_i2c_func(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_NOSTART |
           I2C_FUNC_PROTOCOL_MANGLING;
}
// 定义 i2c_algorithm 对象并填充，主要是两个函数接口
static const struct i2c_algorithm tiny4412_i2c_algorithm = {
    .master_xfer = tiny4412_i2c_xfer,
    .functionality = tiny4412_i2c_func,
};
// i2c 初始化
static int tiny4412_i2c_init(struct tiny4412_i2c *i2c)
{
    unsigned long iicon = TINY4412_IICCON_IRQEN | TINY4412_IICCON_TXDIV_512 | TINY4412_IICCON_ACKEN;

    // 配置引脚，i2c 适配器 0 连接 GPD1_0 和 GPPD1_1 引脚,这里配置两个引脚(第 2 个参数)

    // 可见，将引脚设置为开漏、上拉。
    s3c_gpio_cfgall_range(EXYNOS4_GPD1(0), 2, S3C_GPIO_SFN(2), S3C_GPIO_PULL_UP);
    writeb(0x10, i2c->regs + TINY4412_IICADD);  // IICADD 寄存器中保存 4412 当做从机时的地址
    writel(iicon, i2c->regs + TINY4412_IICCON); // 设置配置寄存器
    return 0;
}
static int __init i2c_adap_init(void)
{
    int ret = 0;
    i2c = kzalloc(sizeof(struct tiny4412_i2c), GFP_KERNEL); // 申请 i2c 结构空间
    // 填充 i2c_adapter 结构，该结构是 i2c 的成员
    strlcpy(i2c->adap.name, "s3c2410-i2c", sizeof(i2c->adap.name));
    i2c->adap.owner = THIS_MODULE;
    i2c->adap.algo = &tiny4412_i2c_algorithm; // 关联底层算法
    i2c->adap.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
    i2c->adap.nr = 0; // 指定适配器编号（使用适配器 0）

    init_waitqueue_head(&i2c->wait);                // 初始化等待队列
    i2c->clk = clk_get_sys("s3c2440-i2c.0", "i2c"); // 获取 i2c 硬件设备的时钟
    clk_enable(i2c->clk);                           // 开时钟
    // 寄存器映射，这里 S3C_PA_IIC 的定义为：
    // #define S3C_PA_IIC EXYNOS4_PA_IIC(0)
    // #define EXYNOS4_PA_IIC(x) (0x13860000 + ((x) * 0x10000))

    // 由此可见，S3C_PA_IIC 就是适配器 0 的物理基地址 0x13860000
    // #define SZ_4K 0x00001000，实际这里仅用了 4 个寄存器，16 个字节
    i2c->regs = ioremap(S3C_PA_IIC, SZ_4K);
    i2c->adap.algo_data = i2c; // 填充后，保存地址
    tiny4412_i2c_init(i2c);    // 配置引脚和 IICCON，初始化适配器
    // 申请中断线，IIC 的中断号定义为：
    // #define IRQ_IIC EXYNOS4_IRQ_IIC
    // #define EXYNOS4_IRQ_IIC IRQ_SPI(58) //可见，是共享中断
    // #define IRQ_SPI(x) (x + 32)
    ret = request_irq(IRQ_IIC, tiny4412_i2c_irq, 0, "tiny4412", i2c);
    i2c_add_numbered_adapter(&i2c->adap); // 向内核注册 i2c_adapter

    return 0;
}
static void __exit i2c_adap_exit(void)
{
    i2c_del_adapter(&i2c->adap); // 注销 i2c_dapter
    free_irq(IRQ_IIC, i2c);      // 释放中断线
    clk_disable(i2c->clk);       // 关设备时钟
    clk_put(i2c->clk);           // 释放时钟源
    iounmap(i2c->regs);          // 取消 io 映射
}
module_init(i2c_adap_init);
module_exit(i2c_adap_exit);
MODULE_LICENSE("GPL");