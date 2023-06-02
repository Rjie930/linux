#include<stdio.h>
#include<fcntl.h>
#include<poll.h>
#include<string.h>

#define KEY1_FILE_NAME "/dev/key1_dev"
#define KEY2_FILE_NAME "/dev/key2_dev"
#define FILE_MAX 2

int main(void)
{
	int fd[2];
	int val=20;
	int val2=0;
	struct pollfd pollfds[FILE_MAX];
	fd[0]=open(KEY1_FILE_NAME,O_RDWR);
	fd[1]=open(KEY2_FILE_NAME,O_RDWR);
	while(1){
	memset(pollfds,0,sizeof(struct pollfd));
	pollfds[0].fd=fd[0];
	pollfds[0].events=POLLIN;
	pollfds[1].fd=fd[1];
	pollfds[1].events=POLLIN;
	int ret=0;
	ret=poll(pollfds,FILE_MAX,-1);
	if(ret ==0)
	printf("time out!\n");
		else if (ret==-1)
		printf("error!\n");
		else if (ret>0){
		printf("key is pressed\n");
		if(pollfds[0].revents&POLLIN)
		{
		read(fd[0],&val,sizeof(char));
		printf("K1 is pressed value=%d\n",val);
		}
		if(pollfds[1].revents&POLLIN)
		{
		read(fd[1],&val2,sizeof(char));
		printf("key2 is pressed value=%d\n",val2);
		}
		}
		
		
	}
	

	close(fd);
	return 0;
}

