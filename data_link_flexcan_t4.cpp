/* This is built for teensy 3.6 >> */
#include "J1939.h"

FlexCAN_T4<CAN0, RX_SIZE_256, TX_SIZE_16> can0;
CAN_message_t frame;


bool data_link_flexcan_t_four_setup(){
  can0.begin();
  can0.setBaudRate(250000);
  frame.flags.extended = 1;
  return true;
}

void data_link_flexcan_t_four_read(){
  if (can0.read(frame)){
    can_frame_rcv.id = frame.id;
    can_frame_rcv.dt = &frame.buf[0];
  }
}

bool data_link_flexcan_t_four_write(){
  frame.id = can_frame_snd.id;
  memcpy(frame.buf, can_frame_snd.dt, 8); //2 memory fetch cycles
  while (!can0.write(frame));
  return true;
}
