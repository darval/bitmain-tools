#ifndef PIC_TOOL
#define PIC_TOOL

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PIC_FLASH_POINTER_START_ADDRESS_H       0x03
#define PIC_FLASH_POINTER_START_ADDRESS_L       0x00
#define PIC_FLASH_POINTER_END_ADDRESS_H         0x0f
#define PIC_FLASH_POINTER_END_ADDRESS_L         0x7f
#define PIC_FREQ_START_ADDRESS_H                0x0f
#define PIC_FREQ_START_ADDRESS_L                0xA0
#define PIC_FLASH_POINTER_FREQ_START_ADDRESS_H  0x0F
#define PIC_FLASH_POINTER_FREQ_START_ADDRESS_L  0xA0
#define PIC_FLASH_POINTER_FREQ_END_ADDRESS_H    0x0f
#define PIC_FLASH_POINTER_FREQ_END_ADDRESS_L    0xDF
#define FREQ_MAGIC                              0x7D

#define PIC_FLASH_LENGTH                    ((((unsigned int)PIC_FLASH_POINTER_END_ADDRESS_H<<8) + PIC_FLASH_POINTER_END_ADDRESS_L) - (((unsigned int)PIC_FLASH_POINTER_START_ADDRESS_H<<8) + PIC_FLASH_POINTER_START_ADDRESS_L) + 1)
#define PIC_FLASH_SECTOR_LENGTH             32
#define PIC_SOFTWARE_VERSION_LENGTH         1
#define PIC_VOLTAGE_TIME_LENGTH             6

#define PIC_COMMAND_1                       0x55
#define PIC_COMMAND_2                       0xaa
#define SET_PIC_FLASH_POINTER               0x01
#define SEND_DATA_TO_PIC                    0x02    // just send data into pic's cache
#define READ_DATA_FROM_PIC_FLASH            0x03
#define ERASE_PIC_FLASH                     0x04    // erase 32 bytes one time
#define WRITE_DATA_INTO_FLASH               0x05    // tell pic write data into flash from cache
#define JUMP_FROM_LOADER_TO_APP             0x06
#define RESET_PIC                           0x07
#define GET_PIC_FLASH_POINTER               0x08
#define SET_VOLTAGE                         0x10
#define SET_VOLTAGE_TIME                    0x11
#define SET_HASH_BOARD_ID                   0x12
#define READ_HASH_BOARD_ID                  0x13
#define SET_HOST_MAC_ADDRESS                0x14
#define ENABLE_VOLTAGE                      0x15
#define SEND_HEART_BEAT                     0x16
#define READ_PIC_SOFTWARE_VERSION           0x17
#define GET_VOLTAGE                         0x18
#define READ_VOLTAGE_SETTING_TIME           0x19
#define READ_WHICH_MAC                      0x20
#define READ_MAC                            0x21
#define WR_TEMP_OFFSET_VALUE                0x22
#define RD_TEMP_OFFSET_VALUE                0x23
#define PIC_PROGRAM                         "/sbin/pic.txt"
#define FILE_BUF_SIZE                       4028

// a3_pic.c
unsigned char read_A3_pic_value(unsigned char command, int fd);
void write_A3_pic_value(unsigned char command, int fd, unsigned char value);
void write_A3_pic_value2(unsigned char command, int fd, unsigned char value, unsigned char value2);
void write_A3_pic_buf(unsigned char command, int fd, unsigned char *buf);
void write_A3_pic(unsigned char command, int fd);

// l3_pic.c
unsigned char read_L3_pic_value(unsigned char command, int fd);
void write_L3_pic_value(unsigned char command, int fd, unsigned char value);
void write_L3_pic_value2(unsigned char command, int fd, unsigned char value, unsigned char value2);
void write_L3_pic_buf(unsigned char command, int fd, unsigned char *buf);
void write_L3_pic(unsigned char command, int fd);

// fd_pic.c
void write_pic(unsigned char byte, int fd);
unsigned char read_pic(int fd);
void send_pic_header(int fd);

#endif // PIC_TOOL