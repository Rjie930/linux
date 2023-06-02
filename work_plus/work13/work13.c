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

#define TINY4412_IICCON 0x00
#define TINY4412_IICSTAT ___(1) ___ // 状态寄存器的偏移量
#define TINY4412_IICADD 0x08
#define TINY4412_IICDS 0x0C

#define TINY4412_IICCON_ACKEN (1 << 7)
#define TINY4412_IICCON_TXDIV_16 (0 << 6)
#define TINY4412_IICCON_TXDIV_512 (1 << 6)
#define TINY4412_IICCON_IRQEN _____(2) _____ // 中断使能位
#define TINY4412_IICCON_IRQPEND (1 << 4)
#define TINY4412_IICCON_SCALE(x) ((x)&0xf)
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

enum tiny4412_i2c_state
{
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_STOP
};

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

static inline void tiny4412_i2c_disable_ack(struct tiny4412_i2c *i2c)
{
	unsigned long tmp;
	tmp = readl(i2c->regs + TINY4412_IICCON);
	writel(tmp & ~TINY4412_IICCON_ACKEN, i2c->regs + TINY4412_IICCON);
}

static inline void tiny4412_i2c_enable_ack(struct tiny4412_i2c *i2c)
{
	unsigned long tmp;
	tmp = readl(i2c->regs + TINY4412_IICCON);
	writel(tmp | TINY4412_IICCON_ACKEN, i2c->regs + TINY4412_IICCON);
}

static inline void tiny4412_i2c_disable_irq(struct tiny4412_i2c *i2c)
{
	unsigned long tmp;
	// 控制寄存器中断使能标志位置1
	_____________(3)
	_______________
		_____________(4) _______________
}

static inline void tiny4412_i2c_enable_irq(struct tiny4412_i2c *i2c)
{
	unsigned long tmp;
	// 控制寄存器中断使能标志位置0
	_____________(5)
	_______________
		_____________(6) _______________
}

static inline void tiny4412_i2c_master_complete(struct tiny4412_i2c *i2c, int ret)
{
	i2c->msg_ptr = 0;
	i2c->msg = NULL;
	i2c->msg_idx++;
	i2c->msg_num = 0;
	if (ret)
		i2c->msg_idx = ret;
	__________(7)
	___________ // 唤醒
}

static void tiny4412_i2c_message_start(struct tiny4412_i2c *i2c, struct i2c_msg *msg)
{
	unsigned int addr = (msg->addr & 0x7f) << 1;
	unsigned long stat;
	unsigned long iiccon;

	stat = 0;
	stat |= TINY4412_IICSTAT_TXRXEN;

	if (msg->flags & I2C_M_RD)
	{
		stat |= TINY4412_IICSTAT_MASTER_RX;
		_______(8)
		_______ // 地址addr第0为置1
	}
	else
		stat |= TINY4412_IICSTAT_MASTER_TX;

	if (msg->flags & I2C_M_REV_DIR_ADDR)
		_______(9)
		_______ // 地址addr第0为置0

			tiny4412_i2c_enable_ack(i2c);

	iiccon = readl(i2c->regs + TINY4412_IICCON);
	writel(stat, i2c->regs + TINY4412_IICSTAT);
	__________________(10)
	________________________ // 把地址写到数据寄存器
		ndelay(50);

	writel(iiccon, i2c->regs + TINY4412_IICCON);

	stat |= TINY4412_IICSTAT_START;
	___________________(11)
	________________________ // 把stat写到状态寄存器
}

static inline void tiny4412_i2c_stop(struct tiny4412_i2c *i2c, int ret)
{
	unsigned long iicstat = readl(i2c->regs + TINY4412_IICSTAT);

	iicstat &= ~TINY4412_IICSTAT_START;

	writel(iicstat, i2c->regs + TINY4412_IICSTAT);

	i2c->state = STATE_STOP;

	tiny4412_i2c_master_complete(i2c, ret);
	tiny4412_i2c_disable_irq(i2c);
}

static inline int is_lastmsg(struct tiny4412_i2c *i2c)
{
	return i2c->msg_idx >= (i2c->msg_num - 1);
}

static inline int is_msglast(struct tiny4412_i2c *i2c)
{
	if (i2c->msg->flags & I2C_M_RECV_LEN && i2c->msg->len == 1)
		return 0;

	return i2c->msg_ptr == i2c->msg->len - 1;
}

static inline int is_msgend(struct tiny4412_i2c *i2c)
{
	return i2c->msg_ptr >= i2c->msg->len;
}

static int i2c_irq_nextbyte(struct tiny4412_i2c *i2c, unsigned long iicstat)
{
	unsigned long tmp;
	unsigned char byte;
	int ret = 0;

	switch (i2c->state)
	{

	case STATE_IDLE:
		goto out;

	case STATE_STOP:
		tiny4412_i2c_disable_irq(i2c);
		goto out_ack;

	case STATE_START:
		if (iicstat & TINY4412_IICSTAT_LASTBIT &&
			!(i2c->msg->flags & I2C_M_IGNORE_NAK))
		{
			tiny4412_i2c_stop(i2c, -ENXIO);
			goto out_ack;
		}

		if (i2c->msg->flags & I2C_M_RD)
			i2c->state = STATE_READ;
		else
			i2c->state = STATE_WRITE;
		if (is_lastmsg(i2c) && i2c->msg->len == 0)
		{
			tiny4412_i2c_stop(i2c, 0);
			goto out_ack;
		}

		if (i2c->state == STATE_READ)
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
		{
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
		byte = readb(i2c->regs + TINY4412_IICDS);
		i2c->msg->buf[i2c->msg_ptr++] = byte;
		if (i2c->msg->flags & I2C_M_RECV_LEN && i2c->msg->len == 1)
			i2c->msg->len += byte;
	prepare_read:
		if (is_msglast(i2c))
		{
			if (is_lastmsg(i2c))
				tiny4412_i2c_disable_ack(i2c);
		}
		else if (is_msgend(i2c))
		{
			if (is_lastmsg(i2c))
			{
				tiny4412_i2c_stop(i2c, 0);
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

static irqreturn_t tiny4412_i2c_irq(int irqno, void *dev_id)
{
	struct tiny4412_i2c *i2c = dev_id;
	unsigned long status;
	unsigned long tmp;

	status = readl(i2c->regs + TINY4412_IICSTAT);

	if (i2c->state == STATE_IDLE)
	{
		// 控制寄存器TINY4412_IICCON_IRQPEND标志位置0
		_______________(12)
		____________________
			_______________(13) ____________________
				_______________(14) ____________________ goto out;
	}

	i2c_irq_nextbyte(i2c, status);

out:
	return IRQ_HANDLED;
}

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

static int tiny4412_i2c_doxfer(struct tiny4412_i2c *i2c, struct i2c_msg *msgs, int num)
{
	int ret;

	tiny4412_i2c_wait_busy(i2c);

	i2c->msg = msgs;
	i2c->msg_num = num;
	i2c->msg_ptr = 0;
	i2c->msg_idx = 0;
	__________(15)
	__________ // 状态为开始状态

		tiny4412_i2c_enable_irq(i2c);
	tiny4412_i2c_message_start(i2c, msgs);

	wait_event_timeout(i2c->wait, i2c->msg_num == 0, HZ * 5);

	ret = i2c->msg_idx;

	return ret;
}

static int tiny4412_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct tiny4412_i2c *i2c = (struct tiny4412_i2c *)adap->algo_data;
	int ret;

	clk_enable(i2c->clk);
	ret = tiny4412_i2c_doxfer(i2c, msgs, num);
	clk_disable(i2c->clk);

	return ret;
}

static u32 tiny4412_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_NOSTART |
		   I2C_FUNC_PROTOCOL_MANGLING;
}

static const struct i2c_algorithm tiny4412_i2c_algorithm = {
	.master_xfer = _________(16) ___________,
	.functionality = tiny4412_i2c_func,
};

static int tiny4412_i2c_init(struct tiny4412_i2c *i2c)
{
	unsigned long iicon = TINY4412_IICCON_IRQEN | TINY4412_IICCON_TXDIV_512 | TINY4412_IICCON_ACKEN;
	s3c_gpio_cfgall_range(EXYNOS4_GPD1(0), 2, S3C_GPIO_SFN(2), S3C_GPIO_PULL_UP);
	writeb(0x10, i2c->regs + TINY4412_IICADD);
	_____________(17)
	________________ // 写入控制寄存器
		return 0;
}

static int __init i2c_adap_init(void)
{
	i2c = kzalloc(sizeof(struct tiny4412_i2c), GFP_KERNEL);

	/*初始化adapter*/
	strlcpy(i2c->adap.name, "s3c2410-i2c", sizeof(i2c->adap.name));
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = _____________(18) ______________
						 i2c->adap.class
		= I2C_CLASS_HWMON | I2C_CLASS_SPD;
	i2c->adap.nr = 0;

	init_waitqueue_head(&i2c->wait);

	i2c->clk = clk_get_sys("s3c2440-i2c.0", "i2c");
	clk_enable(i2c->clk);

	i2c->regs = _________(19) ___________ // 建立基地址映射

					i2c->adap.algo_data = i2c;

	tiny4412_i2c_init(i2c);

	request_irq(IRQ_IIC, _____(20) ________, 0, "tiny4412", i2c);

	i2c_add_numbered_adapter(&i2c->adap); // 注册函数

	return 0;
}

static void __exit i2c_adap_exit(void)
{
	i2c_del_adapter(&i2c->adap);
	free_irq(IRQ_IIC, i2c);
	clk_disable(i2c->clk);
	clk_put(i2c->clk);
	iounmap(i2c->regs);
}

module_init(i2c_adap_init);
module_exit(i2c_adap_exit);
MODULE_LICENSE("GPL");
