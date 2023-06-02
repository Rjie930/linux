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

while(1) {
	val = 0xe; 
	write(fd,&val,sizeof(val));
	sleep(1);
	val=0xf;
	write(fd,&val,sizeof(val));
	sleep(1);
   } 
   
   close(fd); 
   return 0; 
   }
