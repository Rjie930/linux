obj-m += irq_key.o 
flags=-C /home/sice/linux-3.5 M=`pwd` 
modules: 
	make ${flags} modules 
install: 
	make ${flags} INSTALL_MOD_PATH=/opt/rootfs modules_install 
clean: 
	make ${flags} clean 
	rm -rf led_app
