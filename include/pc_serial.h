#ifndef INIT_SERIAL_H
#define INIT_SERIAL_H

#define SERIAL_ERROR_OPEN_DEFAULT   1
#define SERIAL_ERROR_OPEN_USER      2
#define SERIAL_ERROR_GET_ATTR       3
#define SERIAL_ERROR_SET_ATTR       4
#define SERIAL_ERROR_WRITE          5
#define SERIAL_ERROR_READ           6
#define SERIAL_ERROR_PARTIAL_WRITE  7
#define SERIAL_ERROR_PARTIAL_READ   8
#define SERIAL_ERROR_INVALID_FD     9

uint8_t open_serial_interface(char *port, int ispeed, int ospeed);
uint8_t set_serial_interface_attribs(int ispeed, int ospeed);

uint8_t serial_write_array(uint8_t *bytes_to_write, uint32_t num_bytes_to_write, ssize_t *num_bytes_written);
uint8_t serial_read_array(uint8_t *bytes_read, uint32_t num_bytes_to_read, ssize_t *num_bytes_read);


#endif
