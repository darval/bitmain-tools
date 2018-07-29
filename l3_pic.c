#include "pic_tool.h"

unsigned char read_L3_pic_value(unsigned char command, int fd)
{
    send_pic_header(fd);
    write_pic(command, fd);
    usleep(400*1000);
    return read_pic(fd);
}

void write_L3_pic_value(unsigned char command, int fd, unsigned char value)
{
    send_pic_header(fd);
    write_pic(command, fd);
    write_pic(value, fd);   
    usleep(400*1000);
}

void write_L3_pic_value2(unsigned char command, int fd, unsigned char value, unsigned char value2)
{
    send_pic_header(fd);
    write_pic(command, fd);
    write_pic(value, fd);   
    write_pic(value2, fd);   
    usleep(400*1000);
}

void write_L3_pic_buf(unsigned char command, int fd, unsigned char *buf)
{
    send_pic_header(fd);
    write_pic(command, fd);
    for(int i = 0; i < 16; i++)
    {
        write_pic(buf[i], fd);
    }
    usleep(400*1000);
 }

void write_L3_pic(unsigned char command, int fd)
{
    send_pic_header(fd);
    write_pic(command, fd);
    usleep(400*1000);
 }
