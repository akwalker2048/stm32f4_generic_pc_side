#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "read_thread.h"
#include "pc_serial.h"

uint8_t cont_read_thread = 1;
pthread_t read_thread_ref;

void *read_thread(void *ptr)
{
   uint8_t retval;
   ssize_t bytes_read;
   uint8_t read_buffer[READ_BLOCK_SIZE];
   uint32_t ii;

   while(cont_read_thread)
   {
      retval = serial_read_array(read_buffer, READ_BLOCK_SIZE, &bytes_read);
      if(retval == SERIAL_ERROR_READ)
      {
         printf("Bailing out on read_thread!\n");
         cont_read_thread = 0;
      }
      else
      {
         if(bytes_read > 0)
         {
            for(ii=0; ii<bytes_read; ii++)
            {
               printf("0x%02X ", read_buffer[ii]);
            }
            printf("\n");

            /* for(ii=0; ii<bytes_read; ii++) */
            /* { */
            /*    if((0x20 < read_buffer[ii])&&(read_buffer[ii] < 0x7F)) */
            /*    { */
            /*       printf("%c ", (char)read_buffer[ii]); */
            /*    } */
            /*    else */
            /*    { */
            /*       printf("0x%02X ", read_buffer[ii]); */
            /*    } */
            /* } */
            /* printf("\n"); */
            fflush(stdout);
         }
      }

   }

}


void create_read_thread(void)
{
   if(pthread_create(&read_thread_ref, NULL, read_thread, NULL))
   {
      fprintf(stderr, "Error creating thread\n");
   }
}

void join_read_thread(void)
{

   cont_read_thread = 0;

   /* wait for the second thread to finish */
   if(pthread_join(read_thread_ref, NULL))
   {
      fprintf(stderr, "Error joining thread\n");
   }

}
