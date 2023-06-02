obj-m += i2c_dev.o i2c_drv.o
flags=-C /home/sice/linux-3.5 M=`pwd`
modules:
	make ${flags} modules
	arm-linux-gcc ./i2c_test.c -o i2c_test
install:
	cp i2c_dev.ko i2c_drv.ko i2c_test /opt/rootfs
clean:
	make ${flags} clean
	rm ./i2c_app /opt/rootfs/i2c_app
	rm /opt/rootfs/lib/modules/3.5.0-sice/extra/i2c_*.ko
