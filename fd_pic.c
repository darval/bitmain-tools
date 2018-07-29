#include "pic_tool.h"

void write_pic(unsigned char byte, int fd)
{
    int count = 5;
    while( count-- && (write(fd, &byte, 1) != 1))
        usleep(100*1000);
    if( count == 0 ) // timed out
    {
        printf("Timed out writing to pic\n");
        exit(EXIT_FAILURE);
    }
}

unsigned char read_pic(int fd)
{
    int count = 5;
    unsigned char value;
    while(count-- && (read(fd, &value, 1) != 1))
        usleep(100*1000);
    if( count == 0 ) // timed out
    {
        printf("Timed out reading from pic\n");
        exit(EXIT_FAILURE);
    }
    return value;
}

void send_pic_header(int fd)
{
    write_pic('\x55', fd);
    write_pic('\xaa', fd);
}
