//
//  set_voltage_new.c
//  
//
//  Original created by jstefanop on 1/25/18.
//
// Changes made by darval based on original work by jstefanop
//

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

#define PIC_COMMAND_1                       0x55
#define PIC_COMMAND_2                       0xaa
#define GET_VOLTAGE                         0x18
#define SET_VOLTAGE                         0x10
#define JUMP_FROM_LOADER_TO_APP             0x06
#define RESET_PIC                           0x07
#define READ_PIC_SOFTWARE_VERSION           0x17
static unsigned char Pic_command_1 = PIC_COMMAND_1;
static unsigned char Pic_command_2 = PIC_COMMAND_2;
static unsigned char Pic_set_voltage = SET_VOLTAGE;
static unsigned char Pic_get_voltage = GET_VOLTAGE;
static unsigned char Pic_read_pic_software_version = READ_PIC_SOFTWARE_VERSION;
static unsigned char Pic_jump_from_loader_to_app = JUMP_FROM_LOADER_TO_APP;
static unsigned char Pic_reset = RESET_PIC;


void pic_send_command(int fd)
{
    //printf("--- %s\n", __FUNCTION__);
    write(fd, &Pic_command_1, 1);
    write(fd, &Pic_command_2, 1);
}

void pic_read_pic_software_version(unsigned char *version, int fd)
{
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    write(fd, &Pic_read_pic_software_version, 1);
    read(fd, version, 1);
}

void pic_read_voltage(unsigned char *voltage, int fd)
{
    
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    
    write(fd, &Pic_get_voltage, 1);
    read(fd, voltage, 1);
    
    usleep(500000);
}

void pic_set_voltage(unsigned char *voltage, int fd)
{
    
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    
    write(fd, &Pic_set_voltage, 1);
    write(fd, voltage, 1);
    
    usleep(500000);
}



void pic_jump_from_loader_to_app(int fd)
{
    pic_send_command(fd);
    
    //printf("\n--- %s\n", __FUNCTION__);
    write(fd, &Pic_jump_from_loader_to_app, 1);
}

void pic_reset(int fd)
{
    pic_send_command(fd);
    
    printf("\n--- %s\n", __FUNCTION__);
    write(fd, &Pic_reset, 1);
    usleep(600*1000);
}

void print_usage()
{
    printf("Use: set_voltage -c chain [-v voltage] [-qp]\n");
    printf("If you only list the chain, it will read the current voltage\n");
    printf("If you include the optional voltage, it will set the chain's voltage\n");
    printf("Flags: -q only outputs the voltage the pic is set to\n");
    printf("       -p includes the pic version\n");
    printf("Read example:\n");
    printf("./set_voltage -c 1\n");
    printf("Write example:\n");
    printf("./set_voltage -c 1 -v b0\n");
}

int main (int argc, char *argv[])
{
    int chain = 0;
    char *volt = NULL;
    int quiet = 0;
    int pic_version = 0;
    int c;

    while ((c = getopt (argc, argv, "c:v:qp")) != -1)
    {
        switch (c)
        {
        case 'c':
            chain = atoi(optarg);
            break;
        case 'v':
            volt = optarg;
            break;
        case 'q':
            quiet++;
            break;
        case 'p':
            pic_version++;
            break;
        case '?':
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }
    
 
    if(chain > 4 || chain == 0)
    {
        printf("Invalid chain #, valid range 1-4\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    unsigned char set_voltage;
    if( volt != NULL ) 
    {
        set_voltage = strtol(volt, NULL, 16);
        if(set_voltage > 0xfe)
        {
            printf("Invalid hex voltage, valid range 0x00-0xfe\n");
            exit(EXIT_FAILURE);
        }
    }
    int fd;
    char filename[40];
    unsigned char version = 0;
    unsigned char voltage = 0;
    int const i2c_slave_addr[4] = {0xa0,0xa2,0xa4,0xa6};
    
    chain--;
    
    sprintf(filename,"/dev/i2c-0");
    
    if ((fd = open(filename,O_RDWR)) < 0) 
    {
        printf("Failed to open the bus\n");
        exit(EXIT_FAILURE);
    }
    
    if (ioctl(fd,I2C_SLAVE,i2c_slave_addr[chain] >> 1 )) 
    {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        exit(EXIT_FAILURE);
    }
   // pic_reset(fd);
   // pic_jump_from_loader_to_app(fd);
    pic_read_pic_software_version(&version, fd);
    if( pic_version )
    {
        printf("PIC version: 0x%02x\n", version);
    }

    if(version != 0x03)
    {
        printf("Wrong PIC version\n");
        exit(EXIT_FAILURE);
    }
    
    if(!quiet)
    {
        printf("reading voltage: ");
    }
    pic_read_voltage(&voltage, fd);
    
    printf("0x%02x\n", voltage);
    
    if( volt == NULL ) // readonly mode
    {
        exit(EXIT_SUCCESS);
    }

    if(!quiet)
    {
        printf("setting voltage\n");
    }
    pic_set_voltage(&set_voltage, fd);
    
    if(!quiet) 
    {
        printf("reading voltage: ");
    }
    pic_read_voltage(&voltage, fd);
    printf("0x%02x\n", voltage);
    
   // pic_reset(fd);
    
    if(voltage != set_voltage)
    {
        printf("ERROR: Voltage was not successfully set\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        if(!quiet)
        {
            printf("Success: Voltage updated!\n");
        }
    }
    exit(EXIT_SUCCESS);
}



