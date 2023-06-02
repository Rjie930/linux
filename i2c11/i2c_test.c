#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
void print_usage(char *file)
{
    printf("%s r addr\n", file);
    printf("%s w addr val\n", file);
}

typedef struct 
{
	char name[15];
	char gender;    //'M'或‘F'
}stu;

char buff[16];
char buf[2];

int main(int argc, char **argv)
{
    int fd;
	stu stu;
    unsigned char buf[2];

    if ((argc != 3) && (argc != 5))
    {
        print_usage(argv[0]);
        return -1;
    }
    fd = open("/dev/at24", O_RDWR); // 打开设备
    if (fd < 0)
    {
        printf("can't open /dev/at24\n");
        return -1;
    }
    
 	buf[0] = strtoul(argv[2], NULL, 0);
 	
    if (strcmp(argv[1], "r") == 0)
    {
        //buf[0] = strtoul(argv[2], NULL, 0);
        int i=0;
        for(;i<sizeof(buff);i++){
        	read(fd, buf, 1);
        	buff[i]=buf[0];
        }
        stu=(stu)buff;
        printf("stu2: %s, %c\n", stu.name, stu.gender);
    }
    else if ((strcmp(argv[1], "w") == 0) && (argc == 5))
    {
        strncpy(stu.name, argv[4], sizeof(stu.name) - 1);
        stu.gender = argv[5][0];
        buff= (char [])stu;
        int i=0;
        for(;i<sizeof(buff);i++)
        {
        	buf[1]=buff[0];
        	if (write(fd, buf, 2) != 2)
            printf("write err, addr = 0x%02x, data = 0x%02x\n", buf[0], buf[1]);
            }
    }
    else
    {
        print_usage(argv[0]);
        return -1;
    }
    return 0;
}
