//
//  set_voltage_new.c
//  
//
//  Original created by jstefanop on 1/25/18.
//
// Changes made by darval based on original work by jstefanop with A3 ideas and data from FNT
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
#define RESET_PIC                           0x07
#define GET_VOLTAGE                         0x18
#define SET_VOLTAGE                         0x10
#define READ_PIC_SOFTWARE_VERSION           0x17

//
// There are two different PIC controllers (at least) in the bitmain product.  This code is an attempt to create a reasonably common
// solution to getting and setting the voltage through these PICs.  The one on the A3 (and reportedly the D3) appears to have a slightly
// more sophisticated protocol.  After the command header, it requires a length value, the payload (command / command&value) and a CRC. 
// The same is true of the response.  These conclusions are based on a very limited set of examples.
//

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

static void send_pic_header(int fd)
{
    write_pic('\x55', fd);
    write_pic('\xaa', fd);
}

static unsigned char read_L3_pic_value(unsigned char command, int fd)
{
    send_pic_header(fd);
    write_pic(command, fd);
    usleep(400*1000);
    return read_pic(fd);
}

static unsigned char read_A3_pic_value(unsigned char command, int fd)
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

static void write_L3_pic_value(unsigned char command, int fd, unsigned char value)
{
    send_pic_header(fd);
    write_pic(command, fd);
    write_pic(value, fd);   
    usleep(400*1000);
 }

static void write_L3_pic(unsigned char command, int fd)
{
    send_pic_header(fd);
    write_pic(command, fd);
    usleep(400*1000);
 }

static void write_A3_pic_value(unsigned char command, int fd, unsigned char value)
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

static void write_A3_pic(unsigned char command, int fd)
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


void print_usage()
{
    printf("Use: set_voltage <-a || -l> -c chain [-v voltage] [-qp]\n");
    printf("You must chose -a OR -l to select A3 or L3 PIC\n");
    printf("If you only list the chain, it will read the current voltage\n");
    printf("If you include the optional voltage, it will set the chain's voltage\n");
    printf("Flags: -q only outputs the voltage the pic is set to\n");
    printf("       -p includes the pic version\n");
    printf("Read example:\n");
    printf("./set_voltage -a -c 1\n");
    printf("Write example:\n");
    printf("./set_voltage -l -c 1 -v b0\n");
}

int main (int argc, char *argv[])
{
    int chain = 0;
    char *volt = NULL;
    int quiet = 0;
    int pic_version = 0;
    int a3 = 0;
    int l3 = 0;
    int reset = 0;
    int c;
    unsigned char target_pic_version;
    unsigned char (*read_pic_value)(unsigned char command, int fd);
    void (*write_pic_value)(unsigned char command, int fd, unsigned char value);
    void (*write_command)(unsigned char command, int fd);

    while ((c = getopt (argc, argv, "alc:v:qpr:")) != -1)
    {
        switch (c)
        {
        case 'r':
            reset++;
            // fall through and set chain
        case 'c':
            chain = atoi(optarg);
            break;
        case 'v':
            volt = optarg;
            break;
        case 'a':
            a3++;
            break;
        case 'l':
            l3++;
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
    
    if( (a3 && l3) || (!l3 && !a3))
    {
        printf("You must chose either A3 OR L3 and not both\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    if(l3)
    {
        read_pic_value = read_L3_pic_value;
        write_pic_value = write_L3_pic_value;
        write_command = write_L3_pic;
        target_pic_version = 0x03;
    }
    if(a3)
    {
        read_pic_value = read_A3_pic_value;
        write_pic_value = write_A3_pic_value;
        write_command = write_A3_pic;
        target_pic_version = 0x81;       
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

    if( reset )
    {
        printf("Reseting pic at chain %d\n", chain+1);
        write_command(RESET_PIC, fd);
        exit(EXIT_SUCCESS);
    }

    version = read_pic_value(READ_PIC_SOFTWARE_VERSION, fd);
    if( pic_version )
    {
        printf("PIC version: 0x%02x\n", version);
    }

    if(version != target_pic_version)
    {
        printf("Wrong PIC version\n");
        exit(EXIT_FAILURE);
    }
    
    if(!quiet)
    {
        printf("reading voltage: ");
    }
    voltage = read_pic_value(GET_VOLTAGE, fd);
    
    printf("0x%02x\n", voltage);
    
    if( volt == NULL ) // readonly mode
    {
        exit(EXIT_SUCCESS);
    }

    if(!quiet)
    {
        printf("setting voltage\n");
    }
    write_pic_value(SET_VOLTAGE, fd, set_voltage);
    
    if(!quiet) 
    {
        printf("reading voltage: ");
    }
    voltage = read_pic_value(GET_VOLTAGE, fd);
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



