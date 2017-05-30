#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#include "pc_serial.h"

pthread_mutex_t serial_mutex;

int fd = 0;
char *default_portname = "/dev/ttyUSB0";

uint8_t open_serial_interface(char *port, int ispeed, int ospeed)
{
   uint8_t retval;

   if(strncmp(port, "default", 7) == 0)
   {
      fd = open(default_portname, O_RDWR | O_NOCTTY | O_SYNC);
      if(fd < 0)
      {
         printf("Error opening %s: %s\n", default_portname, strerror(errno));
         return SERIAL_ERROR_OPEN_DEFAULT;
      }
   }
   else
   {
      fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
      if(fd < 0)
      {
         printf("Error opening %s: %s\n", port, strerror(errno));
         return SERIAL_ERROR_OPEN_USER;
      }
   }

   retval = set_serial_interface_attribs(ispeed, ospeed);
   if(retval)
   {
      return retval;
   }

   return 0;

}

/* Originally From:  http://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c */
uint8_t set_serial_interface_attribs(int ispeed, int ospeed)
{
   struct termios tty;

   if (tcgetattr(fd, &tty) < 0) {
      printf("Error from tcgetattr: %s\n", strerror(errno));
      return SERIAL_ERROR_GET_ATTR;
   }


   cfsetispeed(&tty, (speed_t)ispeed);
   cfsetospeed(&tty, (speed_t)ospeed);

   tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
   tty.c_cflag &= ~CSIZE;
   tty.c_cflag |= CS8;         /* 8-bit characters */
   tty.c_cflag &= ~PARENB;     /* no parity bit */
   tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
   tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

   /* setup for non-canonical mode */
   tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
   tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
   tty.c_oflag &= ~OPOST;

   /* From termios man page: */
   /* MIN == 0, TIME == 0 (polling read) */
   /*         If data is available, read(2) returns immediately, with the */
   /*         lesser of the number of bytes available, or the number of */
   /*         bytes requested.  If no data is available, read(2) returns 0. */

   /*  MIN > 0, TIME == 0 (blocking read) */
   /*         read(2) blocks until MIN bytes are available, and returns up */
   /*         to the number of bytes requested. */

   /*  MIN == 0, TIME > 0 (read with timeout) */
   /*         TIME specifies the limit for a timer in tenths of a second. */
   /*         The timer is started when read(2) is called.  read(2) returns */
   /*         either when at least one byte of data is available, or when */
   /*         the timer expires.  If the timer expires without any input */
   /*         becoming available, read(2) returns 0.  If data is already */
   /*         available at the time of the call to read(2), the call behaves */
   /*         as though the data was received immediately after the call. */

   /*  MIN > 0, TIME > 0 (read with interbyte timeout) */
   /*         TIME specifies the limit for a timer in tenths of a second. */
   /*         Once an initial byte of input becomes available, the timer is */
   /*         restarted after each further byte is received.  read(2) */
   /*         returns when any of the following conditions is met: */

   /*         *  MIN bytes have been received. */

   /*         *  The interbyte timer expires. */

   /*         *  The number of bytes requested by read(2) has been received. */
   /*            (POSIX does not specify this termination condition, and on */
   /*            some other implementations read(2) does not return in this */
   /*            case.) */

   /*         Because the timer is started only after the initial byte */
   /*         becomes available, at least one byte will be read.  If data is */
   /*         already available at the time of the call to read(2), the call */
   /*         behaves as though the data was received immediately after the */
   /*         call. */
   tty.c_cc[VMIN] = 0;
   tty.c_cc[VTIME] = 0;

   if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      printf("Error from tcsetattr: %s\n", strerror(errno));
      return SERIAL_ERROR_SET_ATTR;
   }

   return 0;
}


/* Read and Write Functions */
uint8_t serial_write_array(uint8_t *bytes_to_write, uint32_t num_bytes_to_write, ssize_t *num_bytes_written)
{
   uint32_t remainder, additional, new_num;
   uint8_t cpy_bytes[512];
   uint32_t ii, dd;


   /* 4 Byte Boundary!
    *   This is needed to work with the STM32F4 family of parts when using
    *   DMA.  The parts require that a FIFO be used and the smallest
    *   chunk is 4 bytes.  If you do not write multiples of 4 bytes...
    *   the remainder is not written to memory (it's still in the FIFO)...
    *   but the NDTR register has already decremented which would normally
    *   indicate that a transfer took place.
    */
   remainder = num_bytes_to_write%4;
   if(remainder != 0)
   {
      additional = 4 - remainder;
   }
   else
   {
      additional = 0;
   }
   new_num = num_bytes_to_write + additional;

   /* printf("num_bytes_to_write:\t%u\n", num_bytes_to_write); */
   /* printf("remainder:\t%u\n", remainder); */
   /* printf("additional:\t%u\n", additional); */
   /* printf("new_num:\t%u\n", new_num); */

   for(ii=0; ii<num_bytes_to_write; ii++)
   {
      cpy_bytes[ii] = bytes_to_write[ii];
   }
   for(ii=num_bytes_to_write; ii<(new_num); ii++)
   {
      cpy_bytes[ii] = 0x00;
   }


   if(fd != 0)
   {
      pthread_mutex_lock(&serial_mutex);

      /* Temporary debug. */
      printf("Sending Packet:\n");
      for(dd=0; dd<new_num; dd++)
      {
         printf("%u\t0x%X\n", dd, cpy_bytes[dd]);
      }

      /* *num_bytes_written = write(fd, bytes_to_write, num_bytes_to_write); */
      *num_bytes_written = write(fd, cpy_bytes, new_num);
      if(*num_bytes_written < 0)
      {
         printf("Error from write: %ld, %d\n", *num_bytes_written, errno);
         return SERIAL_ERROR_WRITE;
      }
      else if(*num_bytes_written != new_num)
      {
         /* This should be rare! */
         printf("Partial Write!\n");
         return SERIAL_ERROR_PARTIAL_WRITE;
      }

      pthread_mutex_unlock(&serial_mutex);
   }
   else
   {
      printf("Can't write because we have a NULL file descriptor!\n");
      return SERIAL_ERROR_INVALID_FD;
   }

   return 0;
}

uint8_t serial_read_array(uint8_t *bytes_read, uint32_t num_bytes_to_read, ssize_t *num_bytes_read)
{

   if(fd != 0)
   {
      pthread_mutex_lock(&serial_mutex);

      *num_bytes_read = read(fd, bytes_read, num_bytes_to_read);
      if(*bytes_read < 0)
      {
         printf("Error from read: %ld, %d\n", *num_bytes_read, errno);
         return SERIAL_ERROR_READ;
      }
      else if(*num_bytes_read != *num_bytes_read)
      {
         /* This will happen often...we are non-blocking! */
         return SERIAL_ERROR_PARTIAL_READ;
      }

      pthread_mutex_unlock(&serial_mutex);
   }
   else
   {
      printf("Can't read because we have a NULL file descriptor!\n");
      return SERIAL_ERROR_INVALID_FD;
   }

   return 0;
}
