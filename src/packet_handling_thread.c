#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "packet_handling_thread.h"

uint8_t cont_packet_handling_thread = 1;
pthread_t packet_handling_thread_ref;

#define GP_CIRC_BUFFER_SIZE 16
GenericPacket gp_circ_buffer[GP_CIRC_BUFFER_SIZE];
uint32_t gp_circ_buffer_head = 0;
uint32_t gp_circ_buffer_tail = 0;

void *packet_handling_thread(void *ptr)
{
   uint8_t retval;
   GenericPacket *gp_ptr;

   VOSPIFrame vospi_frame;
   uint32_t timestamp;

   while(cont_packet_handling_thread)
   {
      while(gp_circ_buffer_head != gp_circ_buffer_tail)
      {
         gp_circ_buffer_tail = gp_circ_buffer_tail + 1;
         if(gp_circ_buffer_tail >= GP_CIRC_BUFFER_SIZE)
         {
            gp_circ_buffer_tail = 0;
         }
         gp_ptr = &(gp_circ_buffer[gp_circ_buffer_tail]);
         switch(gp_ptr->gp[GP_LOC_PROJ_ID])
         {
            case GP_PROJ_UNIVERSAL:
               {
                  switch(gp_ptr->gp[GP_LOC_PROJ_SPEC])
                  {
                     case UNIVERSAL_TEST_PACKET:
                        break;
                     case UNIVERSAL_TIMESTAMP:
                        retval = extract_universal_timestamp(gp_ptr, &timestamp);
                        printf("timestamp:  %u\n", timestamp);
                        break;
                     case UNIVERSAL_ACK:
                        break;
                     case UNIVERSAL_STRING:
                        break;
                     case UNIVERSAL_BYTE:
                        break;
                     case UNIVERSAL_CHOMP:
                        break;
                     case UNIVERSAL_WORD:
                        break;
                     case UNIVERSAL_FLOAT:
                        break;
                     default:
                        /* Unhandled packet within project. */
                        printf("Unhandled Universal Packet!\n");
                        break;
                  }
               }
               break;
            case GP_PROJ_THERMAL:
               {
                  switch(gp_ptr->gp[GP_LOC_PROJ_SPEC])
                  {
                     case THERMAL_LEPTON_FRAME:
                        retval = extract_thermal_lepton_frame(gp_ptr, &vospi_frame);
                        if(retval == GP_SUCCESS)
                        {
                           retval = decode_thermal_lepton_frame(&vospi_frame);
                           if(retval == GP_SUCCESS)
                           {
                              /* We got a good frame...now what? */
                              printf("%u:We got a good lepton frame! h(%u) t(%u)\n", vospi_frame.number, gp_circ_buffer_head, gp_circ_buffer_tail);
                           }
                           else
                           {
                              printf("Unable to decode lepton frame!  %u\n", retval);
                           }
                        }
                        else
                        {
                           printf("Unable to extract lepton frame!  %u\n", retval);
                        }

                        break;
                     default:
                        /* Unhandled Thermal Packet */
                        break;
                  }
               }
               break;
            default:
               /* Increment status variable to let us know we have an
                * unhandled project type.
                */
               printf("Unhandled Project!\n");
               break;
         }
      }
   }
}

void create_packet_handling_thread(void)
{
   if(pthread_create(&packet_handling_thread_ref, NULL, packet_handling_thread, NULL))
   {
      fprintf(stderr, "Error creating packet handling thread!\n");
   }
}

void join_packet_handling_thread(void)
{

   cont_packet_handling_thread = 0;

   /* wait for the second thread to finish */
   if(pthread_join(packet_handling_thread_ref, NULL))
   {
      fprintf(stderr, "Error joining packet handling thread!\n");
   }

}

uint8_t add_gp_to_circ_buffer(GenericPacket packet)
{
   uint8_t retval;
   uint32_t temp_head;

   temp_head = gp_circ_buffer_head + 1;
   if(temp_head >= GP_CIRC_BUFFER_SIZE)
   {
      temp_head = 0;
   }
   retval = gp_copy_packet(packet, &(gp_circ_buffer[temp_head]));
   if(retval != GP_SUCCESS)
   {
      return retval;
   }
   gp_circ_buffer_head = temp_head;

   return GP_SUCCESS;

}
