#include "pic_tool.h"

unsigned char read_A3_pic_value(unsigned char command, int fd)
{
    unsigned short crc = command + 4;
    char data[5] = {4, command, ((crc >> 8)&0xff), (crc & 0xff),0};

    send_pic_header(fd);
    for(int i = 0; i < 4; i++)
    {
        write_pic(data[i], fd);
    }
    usleep(400*1000);
    for(int i = 0; i < 5; i++)
    {
        data[i] = read_pic(fd);
    }
    // confirm we have a valid read
    if( data[0] == 5 && //length is valid
        data[1] == command )//response is to the right command
        {
            crc = data[0] + data[1] + data[2];
            if( data[3] == (crc >> 8) && // high order crc value
                data[4] == (crc & 0xff)) // low order byte matches
            {
                // good read
                return data[2];
            }
            else
            {
                printf("High = %x, low = %x, but crc = %x ", data[3], data[4], crc);            
            }
        }
    else
    {
        printf("Length = %d, command = %x, but ", data[0], data[1]);
    }
    printf("Read command failed\n");
    exit(EXIT_FAILURE);
}


void write_A3_pic_value(unsigned char command, int fd, unsigned char value)
{
    unsigned short crc = command + 5 + value;
    char data[5] = {5, command, value, ((crc >> 8)&0xff), (crc & 0xff)};

    send_pic_header(fd);
    for(int i = 0; i < 5; i++)
    {
        write_pic(data[i], fd );
    }
    usleep(500*1000);
    for(int i = 0; i < 2; i++)
    {
        data[i] = read_pic(fd);
    }
    // confirm we have a valid read
    if( data[0] == command && //response is to the right command
        data[1] == 1 ) // and it is valid
        {
            return;
        }
    printf("Command = %x, valid = %d, but write command failed\n", data[0], data[1]);
    exit(EXIT_FAILURE);
}

void write_A3_pic_value2(unsigned char command, int fd, unsigned char value, unsigned char value2)
{
    unsigned short crc = command + 6 + value;
    char data[6] = {6, command, value, value2, ((crc >> 8)&0xff), (crc & 0xff)};

    send_pic_header(fd);
    for(int i = 0; i < 6; i++)
    {
        write_pic(data[i], fd );
    }
    usleep(500*1000);
    for(int i = 0; i < 2; i++)
    {
        data[i] = read_pic(fd);
    }
    // confirm we have a valid read
    if( data[0] == command && //response is to the right command
        data[1] == 1 ) // and it is valid
        {
            return;
        }
    printf("Command = %x, valid = %d, but write command failed\n", data[0], data[1]);
    exit(EXIT_FAILURE);
}

void write_A3_pic_buf(unsigned char command, int fd, unsigned char *buf)
{
    unsigned short crc = command + 18;
    char data[2] = {18, command};

    send_pic_header(fd);
    for(int i = 0; i < 2; i++)
    {
        write_pic(data[i], fd );
    }
    for(int i = 0; i < 16; i++)
    {
        write_pic(buf[i], fd );
    }
    write_pic(((crc >> 8)&0xff), fd);
    write_pic(crc & 0xff, fd);
    usleep(500*1000);
    for(int i = 0; i < 2; i++)
    {
        data[i] = read_pic(fd);
    }
    // confirm we have a valid read
    if( data[0] == command && //response is to the right command
        data[1] == 1 ) // and it is valid
        {
            return;
        }
    printf("Command = %x, valid = %d, but write command failed\n", data[0], data[1]);
    exit(EXIT_FAILURE);
}

void write_A3_pic(unsigned char command, int fd)
{
    unsigned short crc = command + 4;
    char data[4] = {4, command, ((crc >> 8)&0xff), (crc & 0xff)};

    send_pic_header(fd);
    for(int i = 0; i < 4; i++)
    {
        write_pic(data[i], fd );
    }
    usleep(500*1000);
    for(int i = 0; i < 2; i++)
    {
        data[i] = read_pic(fd);
    }
    // confirm we have a valid read
    if( data[0] == command && //response is to the right command
        data[1] == 1 ) // and it is valid
        {
            return;
        }
    printf("Command = %x, valid = %d, but write command failed\n", data[0], data[1]);
    exit(EXIT_FAILURE);
}
