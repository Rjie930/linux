#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define FILE_NAME "/dev/led"
int main(void)
{
 int fd,val;
 int i = 0;
 fd = open(FILE_NAME,O_RDWR);
 if(fd < 0)
 printf("open error!\n");
 
 while(1)
 {
 val = 0xfe;
 for(i=0;i<4;i++)
 {
 write(fd,&val,sizeof(val));
 val = val<<1 | val>>7;
 usleep(200000);
 }
 }
 close(fd);
 return 0;
}
