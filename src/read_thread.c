#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "read_thread.h"
#include "pc_serial.h"

#include "packet_handling_thread.h"

#include "generic_packet.h"
#include "gp_receive.h"

uint8_t cont_read_thread = 1;
pthread_t read_thread_ref;

uint32_t good_packets = 0;
uint32_t bad_packets = 0;

void *read_thread(void *ptr)
{
   uint8_t retval;
   ssize_t bytes_read;
   uint8_t read_buffer[READ_BLOCK_SIZE];
   uint32_t ii;

   GenericPacket receive_packet;

   /* The packet must be initialized the first time. We'll just use a
    * dummy byte this time. */
   retval = gp_receive_byte(0x00, GP_CONTROL_INITIALIZE, &receive_packet);

   while(cont_read_thread)
   {
      retval = serial_read_array(read_buffer, READ_BLOCK_SIZE, &bytes_read);
      if(retval != SERIAL_SUCCESS)
      {
         if(retval == SERIAL_ERROR_PARTIAL_READ)
         {
            printf("Serial partial read\n");
         }
         else
         {
            printf("Serial read error!!!!\n");
         }
      }

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
               retval = gp_receive_byte(read_buffer[ii], GP_CONTROL_RUN, &receive_packet);
               if((retval == GP_CHECKSUM_MATCH))
               {
                  good_packets++;
                  /* printf("Checksum Match!\n"); */
                  retval = add_gp_to_circ_buffer(receive_packet);
                  if(retval != GP_SUCCESS)
                  {
                     /* Increment status variable so that we can know that bad
                      * things are going on...
                      */
                  }
               }
               else
               {
                  if(retval == GP_ERROR_CHECKSUM_MISMATCH)
                  {
                     bad_packets++;
                     printf("Checksum Fail:  Bad(%u)  Good(%u)!\n", bad_packets, good_packets);
                     gp_print_packet(receive_packet);
                  }
               }
            }



            /* for(ii=0; ii<bytes_read; ii++) */
            /* { */
            /*    printf("0x%02X ", read_buffer[ii]); */
            /* } */
            /* printf("\n"); */

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
      fprintf(stderr, "Error creating read thread!\n");
   }
}

void join_read_thread(void)
{

   cont_read_thread = 0;

   /* wait for the second thread to finish */
   if(pthread_join(read_thread_ref, NULL))
   {
      fprintf(stderr, "Error joining read thread!\n");
   }

}
