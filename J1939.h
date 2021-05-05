#ifndef J1939_HEAD
#define J1939_HEAD

/* Header for all J1939 and CAN functionality */
/*--------------------------------------------*/
#include <PriUint64.h>
#include <FlexCAN_T4.h>
#include "CAN-Logger-3-Teensy36-Mounted.h"
# include "conf.h"

/* Common */
/*--------------------*/
void error(String message);
void J1939_setup();
void print_ex(bool newline, char * format, ...);
bool j1939_read(bool blocking);
bool j1939_write(bool blocking, uint8_t pri, uint32_t pgn, uint8_t da, uint8_t * dt);

/* Application externs */
/*--------------------*/
 /* AT THIS TIME THERE ARE NO IMPLEMENATIONS HERE. THE j1939 DA IS TYPICALLY REQUIRED FOR THIS.*/


/* Network externs */
/*--------------------*/
bool network_setup();
extern void network_function(); 


/* Transport externs */
/*--------------------*/
struct transport_return_type{
  uint8_t pri;
  uint32_t pgn;
  uint8_t da;
  uint8_t sa;
  uint8_t* dt;
  uint16_t len;
};
// I/O supporting duplex operation
extern transport_return_type j1939_pdu_rcv;
extern transport_return_type j1939_pdu_snd;

extern uint16_t max_num_packets_cts;

bool transport_setup();
extern void transport_read();
extern bool transport_write(bool blocking);

/* Data-link externs */
/*--------------------*/
struct dlc_return_type{
  uint32_t id;
  uint8_t* dt;
};
// I/O supporting duplex operation
extern dlc_return_type can_frame_rcv;
extern dlc_return_type can_frame_snd;

/* Wrapper externs */
bool data_link_setup();
extern void data_link_read(bool blocking);
extern bool data_link_write();

/* Library specific externs */
bool data_link_flexcan_t_four_setup();
extern void data_link_flexcan_t_four_read();
extern bool data_link_flexcan_t_four_write();

#endif
