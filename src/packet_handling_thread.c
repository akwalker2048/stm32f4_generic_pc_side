#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>

#include "packet_handling_thread.h"

#include "create_image.h"
#include "create_image_rgb.h"

extern FILE *fid_pc_comm_out;
extern FILE *fid_sonar;
extern FILE *fid_motor;

#define NUM_PIXELS (VOSPI_ROWS * VOSPI_COLS)
uint16_t pixels[NUM_PIXELS];
int pixel_index;

uint16_t image_num = 0;
uint32_t time_ms = 0;

uint8_t cont_packet_handling_thread = 1;
pthread_t packet_handling_thread_ref;

#define GP_CIRC_BUFFER_SIZE 16
GenericPacket gp_circ_buffer[GP_CIRC_BUFFER_SIZE];
uint32_t gp_circ_buffer_head = 0;
uint32_t gp_circ_buffer_tail = 0;

#define NUM_SAMPLES 32

void *packet_handling_thread(void *ptr)
{
   uint8_t retval;
   GenericPacket *gp_ptr;

   VOSPIFrame vospi_frame;
   uint32_t timestamp;

   uint32_t ii, aa;

   uint32_t word;
   float flt;

   char codever[256];
   char ustr[256];

   float voltage;
   uint8_t inches;

   uint8_t byte;

   double avg_dist = 0.0;
   double max_dist = 0.0;
   double min_dist = 0.0;
   double sum_dist = 0.0;
   double std_dist = 0.0;
   double sum_x_squared = 0.0;
   double dist;

   float p, i, d;

   motor_feedback_t mf;

   float pos_rad;

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
                        fprintf(fid_pc_comm_out, "timestamp:  %u\n", timestamp);
                        break;
                     case UNIVERSAL_ACK:
                        fprintf(fid_pc_comm_out, "We got an ACK!\n");
                        break;
                     case UNIVERSAL_STRING:
                        retval = extract_universal_string(gp_ptr, ustr);
                        fprintf(fid_pc_comm_out, "UNIVERSAL_STRING:  %s\n", ustr);
                        break;
                     case UNIVERSAL_BYTE:
                        retval = extract_universal_byte(gp_ptr, &byte);
                        printf("Universal Byte!\t %u\n", byte);
                        break;
                     case UNIVERSAL_CHOMP:
                        break;
                     case UNIVERSAL_WORD:
                        retval = extract_universal_word(gp_ptr, &word);
                        fprintf(fid_pc_comm_out, "UNIVERSAL_WORD:  %u\n", word);
                        break;
                     case UNIVERSAL_FLOAT:
                        retval = extract_universal_float(gp_ptr, &flt);
                        fprintf(fid_pc_comm_out, "UNIVERSAL_FLOAT:  %f\n", flt);
                        break;
                     case UNIVERSAL_CODE_VER:
                        retval = extract_universal_code_ver(gp_ptr, codever);
                        fprintf(fid_pc_comm_out, "Code Version:  %s\n", codever);
                        break;
                     default:
                        /* Unhandled packet within project. */
                        fprintf(fid_pc_comm_out, "Unhandled Universal Packet!\n");
                        break;
                  }
               }
               break;
            case GP_PROJ_THERMAL:
               {
                  switch(gp_ptr->gp[GP_LOC_PROJ_SPEC])
                  {
                     case THERMAL_LEPTON_FRAME:
                        {
                           retval = extract_thermal_lepton_frame(gp_ptr, &vospi_frame);
                           if(retval == GP_SUCCESS)
                           {
                              retval = decode_thermal_lepton_frame(&vospi_frame);
                              if(retval == GP_SUCCESS)
                              {
                                 /* We got a good frame...now what? */
                                 fprintf(fid_pc_comm_out, "%u(%u):We got a good lepton frame! h(%u) t(%u)\n", vospi_frame.number, vospi_frame.type, gp_circ_buffer_head, gp_circ_buffer_tail);

                                 for(ii=0; ii<VOSPI_COLS; ii++)
                                 {
                                    if(vospi_frame.number < VOSPI_ROWS)
                                    {
                                       pixel_index = get_index(ii, vospi_frame.number);
                                       pixels[pixel_index] = ((uint16_t)vospi_frame.data[ii*2+1+4]<<8) + (uint16_t)vospi_frame.data[ii*2+4];
                                    }
                                 }


                                 if(vospi_frame.number == 59)
                                 {
                                    /* image_num++; */
                                    /* create_image(pixels, VOSPI_COLS, VOSPI_ROWS, image_num); */
                                    /* create_image_rgb(pixels, VOSPI_COLS, VOSPI_ROWS, image_num); */
                                 }

                              }
                              else
                              {
                                 fprintf(fid_pc_comm_out, "Unable to decode lepton frame!  %u\n", retval);
                              }
                           }
                           else
                           {
                              fprintf(fid_pc_comm_out, "Unable to extract lepton frame!  %u\n", retval);
                           }
                        }
                        break;
                     case THERMAL_BEGIN_LEPTON_IMAGE:
                        {
                           extract_thermal_begin_lepton_image(gp_ptr, &image_num, &time_ms);
                           fprintf(fid_pc_comm_out, "image_num = %u\ttime_ms = %u\n", image_num, time_ms);
                        }
                        break;
                     case THERMAL_END_LEPTON_IMAGE:
                        {
                           fprintf(fid_pc_comm_out, "End Image!\n");
                           create_image(pixels, VOSPI_COLS, VOSPI_ROWS, image_num);
                           create_image_rgb(pixels, VOSPI_COLS, VOSPI_ROWS, image_num);
                        }
                        break;
                     case THERMAL_IMAGE_TIMEOUT:
                        {
                           fprintf(fid_pc_comm_out, "Image Timeout!\n");
                        }
                        break;
                     default:
                        /* Unhandled Thermal Packet */
                        break;
                  }
               }
               break;
            case GP_PROJ_ANALOG:
               {
                  switch(gp_ptr->gp[GP_LOC_PROJ_SPEC])
                  {
                     case ANALOG_VOLTAGE:
                        retval = extract_analog_voltage(gp_ptr, &voltage);
                        fprintf(fid_pc_comm_out, "Analog Voltage = %f!\n", voltage);
                        break;
                     case ANALOG_BATTERY_VOLTAGE:
                        retval = extract_analog_voltage(gp_ptr, &voltage);
                        aa++;
                        /* fprintf(fid_pc_comm_out, "Analog Battery Voltage = %f!\n", voltage); */

                        /* Distance in Inches */
                        dist = (voltage*512.0/4.8);
                        /* Distance in mm */
                        dist = dist*25.4;

                        sum_dist += dist;
                        sum_x_squared += pow(dist, 2.0);

                        if(max_dist < dist)
                        {
                           max_dist = dist;
                        }
                        if(min_dist > dist)
                        {
                           min_dist = dist;
                        }

                        if(aa%NUM_SAMPLES == 1)
                        {
                           /* sum_dist = 0.0; */
                           /* avg_dist = 0.0; */
                           min_dist = dist;
                           max_dist = dist;
                           /* sum_x_squared = pow(dist, 2.0); */
                        }

                        if(aa%NUM_SAMPLES == 0)
                        {
                           avg_dist = sum_dist / (double)NUM_SAMPLES;
                           std_dist = sqrt((sum_x_squared / (double)NUM_SAMPLES) - pow(avg_dist, 2.0));
                           fprintf(fid_pc_comm_out, "Avg Analog Sonar Distance = %f\tmax(%f)\tmin(%f)\tstd(%f)\n", avg_dist, max_dist, min_dist, std_dist);
                           fprintf(fid_pc_comm_out, "%.2f\t%.2f\t%.2f\t%.2f\n\n", avg_dist, max_dist, min_dist, std_dist);

                           sum_x_squared = 0.0;
                           sum_dist = 0.0;
                           avg_dist = 0.0;

                        }
                        fprintf(fid_sonar, "Analog Sonar Distance = %f mm!\n", dist);
                        fflush(fid_sonar);
                        break;
                     default:
                        /* Unhandled Analog Packet */
                        break;
                  }
               }
               break;
            case GP_PROJ_SONAR:
               {
                  switch(gp_ptr->gp[GP_LOC_PROJ_SPEC])
                  {
                     case SONAR_MAXBOT_SERIAL:
                        retval = extract_sonar_maxbot_serial(gp_ptr, &inches);
                        fprintf(fid_pc_comm_out, "Sonar Dist (inches):  %u\n", inches);
                        break;
                     default:
                        /* Unhandled Sonar Packet */
                        break;
                  }
               }
               break;
            case GP_PROJ_MOTOR:
               {
                  switch(gp_ptr->gp[GP_LOC_PROJ_SPEC])
                  {
                     case MOTOR_FEEDBACK:
                        retval = extract_motor_feedback(gp_ptr, &mf);
                        /* fprintf(fid_pc_comm_out, "MF:  time(%f)\tcmd(%f)\tmsr(%f)\terr(%f)\tierr(%f)\tderr(%f)\tout(%f)\tencoder(%u)\n", mf.time, mf.cmd, mf.msr, mf.err, mf.ierr, mf.derr, mf.out, mf.encoder); */
                        fprintf(fid_motor, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%u\n", mf.time, mf.cmd, mf.msr, mf.err, mf.ierr, mf.derr, mf.out, mf.encoder);
                        fflush(fid_motor);
                        break;
                     case MOTOR_RESP_POSITION:
                        retval = extract_motor_resp_position(gp_ptr, &pos_rad);
                        /* fprintf(fid_pc_comm_out, "Motor Pos (rad):  %f\n", pos_rad); */
                        break;
                     case MOTOR_SET_PID:
                        retval = extract_motor_set_pid(gp_ptr, &p, &i, &d);
                        fprintf(fid_pc_comm_out, "MOTOR_SET_PID:  p(%f)\ti(%f)\td(%f)\n", p, i, d);
                        /* printf("MOTOR_SET_PID:  p(%f)\ti(%f)\td(%f)\n", p, i, d); */
                        break;
                     case MOTOR_RESP_PID:
                        retval = extract_motor_resp_pid(gp_ptr, &p, &i, &d);
                        fprintf(fid_pc_comm_out, "MOTOR_RESP_PID:  p(%f)\ti(%f)\td(%f)\n", p, i, d);
                        /* printf("MOTOR_RESP_PID:  p(%f)\ti(%f)\td(%f)\n", p, i, d); */
                        break;
                     case MOTOR_START:
                        fprintf(fid_pc_comm_out, "MOTOR_START\n");
                        /* printf("MOTOR_START\n"); */
                        break;
                     case MOTOR_STOP:
                        fprintf(fid_pc_comm_out, "MOTOR_STOP\n");
                        /* printf("MOTOR_STOP\n"); */
                        break;
                     default:
                        /* Unhandled Motor Packet */
                        break;
                  }
               }
               break;
            default:
               /* Increment status variable to let us know we have an
                * unhandled project type.
                */
               fprintf(fid_pc_comm_out, "Unhandled Project!\n");
               break;
         }
      }

      fflush(fid_pc_comm_out);

   }

   fclose(fid_pc_comm_out);
   fclose(fid_sonar);
   fclose(fid_motor);

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
