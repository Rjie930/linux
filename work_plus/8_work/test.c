#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "list.h"

struct student
{
    char name[20];
    int age;
    struct list_head node;
};

static LIST_HEAD(student_list);

void add_student()
{
    struct student *new_student = (struct student *)malloc(sizeof(struct student));
    printf("请输入学生姓名：");
    scanf("%s", new_student->name);
    printf("请输入学生年龄：");
    scanf("%d", &new_student->age);
    list_add_tail(&new_student->node, &student_list);
    printf("学生添加成功\n");
}

void delete_student()
{
    if (list_empty(&student_list))
    {
        printf("链表为空，无法删除学生\n");
        return;
    }
    char name[20];
    printf("请输入要删除的学生姓名：");
    scanf("%s", name);
    struct student *student;
    struct list_head *pos, *next;
    list_for_each_safe(pos, next, &student_list)
    {
        student = list_entry(pos, struct student, node);
        if (strcmp(student->name, name) == 0)
        {
            list_del(pos);
            free(student);
            printf("学生删除成功\n");
            return;
        }
    }
    printf("未找到要删除的学生\n");
}

void show_students()
{
    if (list_empty(&student_list))
    {
        printf("链表为空，无学生信息\n");
        return;
    }
    struct student *student;
    printf("学生信息列表：\n");
    printf("%-20s %-10s\n", "姓名", "年龄");
    list_for_each_entry(student, &student_list, node)
    {
        printf("%-20s %-10d\n", student->name, student->age);
    }
}

int main()
{
    char command;
    while (1)
    {
        printf("\n请输入操作命令（a-添加学生，d-删除学生，s-显示学生信息，q-退出）：");
        scanf(" %c", &command);
        switch (command)
        {
        case 'a':
            add_student();
            break;
        case 'd':
            delete_student();
            break;
        case 's':
            show_students();
            break;
        case 'q':
            printf("退出程序\n");
            exit(0);
        default:
            printf("无效的命令，请重新输入\n");
            break;
        }
    }
    return 0;
}
