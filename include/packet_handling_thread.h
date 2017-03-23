#include "generic_packet.h"
#include "gp_receive.h"
#include "gp_proj_thermal.h"
#include "gp_proj_universal.h"


void create_packet_handling_thread(void);
void join_packet_handling_thread(void);
uint8_t add_gp_to_circ_buffer(GenericPacket packet);
