obj-m += misc.o 
flags=-C /home/sice/linux-3.5 M=`pwd` 
modules: 
	make ${flags} modules 
	arm-linux-gcc misc_test.c -o misc_test
install: 
	make ${flags} INSTALL_MOD_PATH=/opt/rootfs modules_install 
	cp misc_test misc.ko /opt/rootfs
clean: 
	make ${flags} clean 
	rm -rf misc
