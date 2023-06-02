#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void print_usage(char *file)
{
    printf("%s r addr\n", file);
    printf("%s w addr val\n", file);
}

typedef struct
{
    char name[15];
    char gender; //'M'或‘F'
} stu;

char buff[16];
char buf[2];
char addr;
int i = 0;

int main(int argc, char **argv)
{
    int fd;
    stu stu;

    if ((argc != 3) && (argc != 5))
    {
        print_usage(argv[0]);
        return -1;
    }

    fd = open("/dev/at24", O_RDWR);
    if (fd < 0)
    {
        printf("can't open /dev/at24\n");
        return -1;
    }

    addr= strtoul(argv[2], NULL, 0);
	buf[0]=addr;
	
    if (strcmp(argv[1], "r") == 0)
    {
        for (i = 0; i < sizeof(buff); i++)
        {
            read(fd, buf, 1);
            buff[i] = buf[0];
            
            buf[0]=++addr;
        }
        
        for (i = 0; i < sizeof(buff); i++)
            *((char *)&stu + i) = buff[i];
        printf("read stu2: adrr:%s name:%s gender:%c\n", argv[2],stu.name, stu.gender);
    }
    else if ((strcmp(argv[1], "w") == 0) && (argc == 5))
    {
        strncpy(stu.name, argv[3], sizeof(stu.name) - 1);
        stu.gender = argv[4][0];
		printf("write stu1:adrr:%s name:%s gender:%c\n", argv[2],stu.name, stu.gender);
        for (i = 0; i < sizeof(buff); i++){
            buff[i] = *((char *)(&stu) + i);
            }

        for (i = 0; i < sizeof(buff); i++)
        {
        buf[1] = buff[i];
        if (write(fd, buf, 2) != 2)
        	printf("write err, addr = 0x%02x, data = 0x%02x\n", buf[0], buf[1]);
        buf[0]++;
		usleep(3150);
        }
    }
    else
    {
        print_usage(argv[0]);
        return -1;
    }

    close(fd);
    return 0;
}

