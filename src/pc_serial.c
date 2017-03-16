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

   if(fd != 0)
   {
      pthread_mutex_lock(&serial_mutex);

      *num_bytes_written = write(fd, bytes_to_write, num_bytes_to_write);
      if(*num_bytes_written < 0)
      {
         printf("Error from write: %ld, %d\n", *num_bytes_written, errno);
         return SERIAL_ERROR_WRITE;
      }
      else if(*num_bytes_written != num_bytes_to_write)
      {
         /* This should be rare! */
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
