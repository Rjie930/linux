#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int main()
{
int fd = 0;
unsigned char btn_status;
fd = open("/dev/btn_drv",O_RDONLY);
if(fd < 0)
{
printf("open \"/dev/btn_drv\" error!\n");
return -1;
}
while(1)
{
read(fd,&btn_status,sizeof(btn_status));
printf("btn_status is %d\n",btn_status);
}
close(fd);
return 0;
}
