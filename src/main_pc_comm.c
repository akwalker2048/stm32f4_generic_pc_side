#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#include "keyboard.h"
#include "pc_serial.h"
#include "read_thread.h"
#include "packet_handling_thread.h"

int main(int argc, char *argv[])
{
    int fd;
    int wlen;
    int ispeed, ospeed;

    int key;
    uint8_t retval;
    uint8_t cont;

    char *test_str="test string!";
    ssize_t bytes_written;

    int jj;

    GenericPacket gp;

    /* baudrate 115200, 8 bits, no parity, 1 stop bit */
    /* #define  B57600   0010001 */
    /* #define  B115200  0010002 */
    /* #define  B230400  0010003 */
    /* #define  B460800  0010004 */
    /* #define  B500000  0010005 */
    /* #define  B576000  0010006 */
    /* #define  B921600  0010007 */
    /* #define  B1000000 0010010 */
    /* #define  B1152000 0010011 */
    /* #define  B1500000 0010012 */
    /* #define  B2000000 0010013 */
    /* #define  B2500000 0010014 */
    /* #define  B3000000 0010015 */
    /* #define  B3500000 0010016 */
    /* #define  B4000000 0010017 */
    /* #define __MAX_BAUD B4000000 */

    ispeed = B3000000;
    ospeed = B3000000;
    /* ispeed = B115200; */
    /* ospeed = B115200; */
    if(argc == 1)
    {

       retval = open_serial_interface("default", ispeed, ospeed);
       if(retval)
       {
          exit(retval);
       }
    }
    else if(argc == 2)
    {
       retval = open_serial_interface(argv[1], ispeed, ospeed);
       if(retval)
       {
          exit(retval);
       }
    }
    else
    {
       printf("Too many arguments...exiting!!!!\n");
       printf("  0)  Run with no arguments and it will attempt to open serial\n");
       printf("  1)  Run with one argument that is the port you wish to open\n");
       exit(-1);
    }


    create_packet_handling_thread();
    create_read_thread();

    /* retval = create_universal_code_ver(&gp, test_str); */
    /* serial_write_array(gp.gp, gp.packet_length, &bytes_written); */


    /* simple noncanonical input */
    cont = 1;
    do
    {
       key = getkey();
       if(key == 'x')
       {
          cont = 0;
       }
       else if(key == 't')
       {
          /* serial_write_array((uint8_t *)test_str, strlen(test_str), &bytes_written); */
          /* uint8_t create_universal_code_ver(GenericPacket *packet, char *codever); */
          retval = create_universal_code_ver(&gp, test_str);
          /* retval = create_universal_ack(&gp); */
          printf("Write universal code version packet! Length = %u\n", gp.packet_length);
          printf("Send:\n");
          for(jj=0; jj<gp.packet_length; jj++)
          {
             printf("0x%2X ", gp.gp[jj]);
          }
          printf("\nReceive:\n");
          serial_write_array(gp.gp, gp.packet_length, &bytes_written);
       }

    }while(cont);

    join_read_thread();
    join_packet_handling_thread();

    return 0;

}
