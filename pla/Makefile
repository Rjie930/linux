obj-m += btn_drv.o btn_dev.o
flags=-C /home/sice/linux-3.5 M=`pwd`
modules:
	make ${flags} modules
	arm-linux-gcc ./btn_app.c -o btn_app
install:
	make ${flags} INSTALL_MOD_PATH=/opt/rootfs modules_install
	cp ./btn_app /opt/rootfs/
clean:
	make ${flags} clean
	rm -rf btn_app /opt/rootfs/btn_app
	rm -rf /opt/rootfs/lib/modules/3.5.0-sice/extra/btn_d*v.ko