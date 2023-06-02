#include<stdio.h>
#include<fcntl.h>
#include "btn.h"
#define FILE_NAME "/dev/my_led_dev"

 unsigned int gpm4con_no=0xf0;
 unsigned int gpm4con_off=0xfe;
 unsigned int led_state=0;
int main(void)
{
	int fd;
	char val=20;
	char val2=0;
	
	fd = open(FILE_NAME,O_RDWR|O_NONBLOCK);
	
	ioctl(fd,TINY4412_BTN_IORESET);
	sleep(0.5);
	
	ioctl(fd,TINY4412_BTN_IOSET,&val);
	sleep(0.5);
	
	ioctl(fd,TINY4412_BTN_IOGET,&val2);
	printf("val2=%d\n",val2);

	char cmd;
	
	while(scanf("%c",&cmd))
	{
		switch(cmd){
			case 'l':ioctl(fd,TINY4412_LED_ALL_ON,&gpm4con_no);
			break;
	
			case 'o':ioctl(fd,TINY4412_LED_ALL_OFF,&gpm4con_off);
			break;
			
			case 'g':ioctl(fd,TINY4412_LED_STATE_GET,&led_state);
			if(led_state==0xe)
			printf("LED_ALL_OFF\n");
			else
			printf("LED_ALL_ON\n");
			break;
		}
	}


	//read(fd,&val,sizeof(val));
	//write(fd,&val,sizeof(val));

	close(fd);
	return 0;
}

