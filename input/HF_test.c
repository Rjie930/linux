#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
int main()
{
int fd = 0;
unsigned char btn_status;
struct input_event evbuf;
fd = open("/dev/input/event3",O_RDONLY);
if(fd < 0)
{
printf("open \"/dev/input/event3\" error!\n");
return -1;
}
while(1)
{
read(fd,&evbuf,sizeof(evbuf));
if(evbuf.code == KEY_A)
{
if(evbuf.value == 1)
printf("key1 pressed!\n");
else if(evbuf.value == 0)
printf("key1 released!\n");
}
else if(evbuf.code == KEY_B)
{
if(evbuf.value == 1)
printf("key2 pressed!\n");
else if(evbuf.value == 0)
printf("key2 released!\n");
}
else if(evbuf.code == KEY_C)
{
if(evbuf.value == 1)
printf("key3 pressed!\n");
else if(evbuf.value == 0)
printf("key3 released!\n");
}
else if(evbuf.code == KEY_D)
{
if(evbuf.value == 1)
printf("key4 pressed!\n");
else if(evbuf.value == 0)
printf("key4 released!\n");
}
}
close(fd);
return 0;
}

