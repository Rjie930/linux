#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h> 
#define FILE_NAME "/dev/my_key_drv"
static unsigned int gpm4con;
int main(void)
{
	int fd;
	int val = 0 ;
	fd = open(FILE_NAME,O_RDWR);
	if (fd<0)
	{
		printf("open false");
		return -1;
	}
	while(1)
	{
		read(fd,&val,sizeof(val));
		printf("key %d press!\n",val);
		gpm4con^=0x1<<val-1;
		write(fd,&gpm4con,1);
	}
	close(fd);
	return 0;

}
