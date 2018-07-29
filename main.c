//
//  set_voltage_new.c
//  
//
//  Original created by jstefanop on 1/25/18.
//
// Changes made by darval based on original work by jstefanop https://github.com/jstefanop/bitmain-tools.git
// with A3 ideas and data from FNT https://github.com/ForexNeuroTrader/A3_D3.git
// Further changes and enhancements based on work from bitmaintech https://github.com/bitmaintech/cgminer-dash.git
//


#include "pic_tool.h"
//
// There are two different PIC controllers (at least) in the bitmain product.  This code is an attempt to create a reasonably common
// solution to getting and setting the voltage through these PICs.  The one on the A3 (and reportedly the D3) appears to have a slightly
// more sophisticated protocol.  After the command header, it requires a length value, the payload (command / command&value) and a CRC. 
// The same is true of the response.  These conclusions are based on a very limited set of examples.
//
static unsigned char (*read_pic_value)(unsigned char command, int fd);
static void (*write_pic_value)(unsigned char command, int fd, unsigned char value);
static void (*write_pic_value2)(unsigned char command, int fd, unsigned char value, unsigned char value2);
static void (*write_pic_buf)(unsigned char command, int fd, unsigned char *buf);
static void (*write_command)(unsigned char command, int fd);



static void pic_reset(int chain, int fd)
{
    printf("Reseting pic at chain %d\n", chain+1);
    write_command(RESET_PIC, fd);
    usleep(600*1000);
}

static void pic_erase_flash_all(int chain, int fd)
{
    unsigned char start_addr_h = PIC_FLASH_POINTER_START_ADDRESS_H, start_addr_l = PIC_FLASH_POINTER_START_ADDRESS_L;
 
    printf("Erasing pic flash at chain %d\n", chain+1);
    write_pic_value2(SET_PIC_FLASH_POINTER, fd, start_addr_h, start_addr_l);
    for(unsigned i = 0; i < PIC_FLASH_LENGTH/PIC_FLASH_SECTOR_LENGTH; i++)
    {
        printf("E "); fflush(stdout);
        write_command(ERASE_PIC_FLASH, fd);
        usleep(500*1000);
    }    
    printf("\n");
}

static void pic_set_flash_pointer(int fd)
{
    printf("Setting pic flash pointers\n");
    write_pic_value2(SET_PIC_FLASH_POINTER, fd, PIC_FLASH_POINTER_START_ADDRESS_H, PIC_FLASH_POINTER_START_ADDRESS_L);
}

static void pic_send_data_to_pic(int fd, unsigned char *buf)
{
    printf("W"); fflush(stdout);
    write_pic_buf(SEND_DATA_TO_PIC, fd, buf);
}

static void pic_write_data_into_flash(int fd)
{
    printf(" "); fflush(stdout);
    write_command(WRITE_DATA_INTO_FLASH, fd);
}

static void update_pic_program(int chain, int fd)
{
    unsigned char program_data[10*FILE_BUF_SIZE] = {0};
    FILE * pic_program_file;
    unsigned int i=0;
    unsigned char data_read[16]= {0}, buf[16]= {0};
    unsigned int data_int = 0;

    printf("Update pic program on chain %d\n", chain+1);

    // read upgrade file first, if it is wrong, don't erase pic, but just return;
    pic_program_file = fopen(PIC_PROGRAM, "r");
    if(!pic_program_file)
    {
        printf("\n%s: open %s failed\n", __FUNCTION__, PIC_PROGRAM);
        return;
    }
    fseek(pic_program_file,0,SEEK_SET);
    memset(program_data, 0x0, 10*FILE_BUF_SIZE);

    printf("pic_flash_length = %d\n", PIC_FLASH_LENGTH);

    for(i=0; i<PIC_FLASH_LENGTH; i++)
    {
        if(fgets((char *)data_read, sizeof(data_read)-1 , pic_program_file) == NULL)
        {
            fclose(pic_program_file);
            printf("error reading pic program\n");
            return;
        }
        data_int = strtoul((char *)data_read, NULL, 16);
        program_data[2*i + 0] = (unsigned char)((data_int >> 8) & 0x000000ff);
        program_data[2*i + 1] = (unsigned char)(data_int & 0x000000ff);
    }

    fclose(pic_program_file);

    // after read upgrade file correct, erase pic
    pic_reset(chain, fd);
    pic_erase_flash_all(chain, fd);

    // write data into pic
    pic_set_flash_pointer(fd);

    for(i=0; i<PIC_FLASH_LENGTH/PIC_FLASH_SECTOR_LENGTH*4; i++)
    {
        memcpy(buf, program_data+i*16, 16);
        pic_send_data_to_pic(fd, buf);
        pic_write_data_into_flash(fd);
    }
    printf("\nDone\n");
}

static void print_usage()
{
    printf("Use: pic_tool <-a || -l> -c chain [-v voltage] [-qp]\n");
    printf("You must chose -a OR -l to select A3 or L3 PIC\n");
    printf("If you only list the chain, it will read the current voltage\n");
    printf("If you include the optional voltage, it will set the chain's voltage\n");
    printf("Flags: -q only outputs the voltage the pic is set to\n");
    printf("       -p includes the pic version\n");
    printf("Read example:\n");
    printf("./pic_tool -a -c 1\n");
    printf("Write example:\n");
    printf("./pic_tool -l -c 1 -v b0\n");
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
    int jump = 0;
    int force = 0;
    int c;
    unsigned char target_pic_version;

    while ((c = getopt (argc, argv, "alc:v:qpr:j:f")) != -1)
    {
        switch (c)
        {
        case 'r':
            reset++;
            // fall through and set chain
        case 'c':
            chain = atoi(optarg);
            break;
        case 'j':
            jump++;
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
        case 'f':
            force++;
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
        write_pic_value2 = write_L3_pic_value2;
        write_pic_buf = write_L3_pic_buf;
        write_command = write_L3_pic;
        target_pic_version = 0x03;
    }
    if(a3)
    {
        read_pic_value = read_A3_pic_value;
        write_pic_value = write_A3_pic_value;
        write_pic_value2 = write_A3_pic_value2;
        write_pic_buf = write_A3_pic_buf;
        write_command = write_A3_pic;
        target_pic_version = 0x81;       
    }

    if(chain > 4 || chain == 0)
    {
        printf("Invalid chain #, valid range 1-4\n\n");
        print_usage();
        exit(EXIT_FAILURE);
    }

    unsigned char set_voltage = 0xff;
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
        pic_reset(chain, fd);
        exit(EXIT_SUCCESS);
    }

    if( jump )
    {
        printf("Jumping pic at chain %d\n", chain+1);
        write_command(JUMP_FROM_LOADER_TO_APP, fd);
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
        printf("If you want to reload the pic program, you must include the -f (force) flag and not the -q (quiet) flag\n");
        if( force && !quiet)
        {
            update_pic_program(chain, fd);
            exit(EXIT_SUCCESS);
        }
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
    
    if( volt != NULL )
    {
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
    }

    exit(EXIT_SUCCESS);
}



