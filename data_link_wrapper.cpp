/* This is built for teensy 3.6 >> */
#include "J1939.h"

bool data_link_setup(){
  return data_link_flexcan_t_four_setup();
}

void data_link_read(bool blocking){
  data_link_flexcan_t_four_read();
  if (blocking){
    while (sizeof(can_frame_rcv.dt) == 0){}
  }
}

bool data_link_write(){
  return data_link_flexcan_t_four_write();
}

void data_link_read_callback(const CANFD_message_t &frame){
  can_frame_rcv.id = frame.id;
  can_frame_rcv.dt = &frame.buf[0];
}
