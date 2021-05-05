#include "J1939.h"

transport_return_type j1939_pdu_rcv;
transport_return_type j1939_pdu_snd;

dlc_return_type can_frame_rcv;
dlc_return_type can_frame_snd;

void print_ex(bool newline, char * format, ...)
{
  char prnt[256];
  memset(prnt, 0, 100);
  va_list args;
  va_start (args, format);
  vsprintf (prnt, format, args);
  va_end (args);
  if (newline) {Serial.println(prnt);}
  else{Serial.print(prnt);}
}

void error(String message){
  Serial.println(message);
  while(true){}; 
}

void J1939_setup(){
  if (!data_link_setup()){error("data_link initialization failed");}
  if (!transport_setup()){error("transport initialization failed");}
}

bool j1939_read(bool blocking){
  memset(&j1939_pdu_rcv, 0, sizeof(j1939_pdu_rcv));
  memset(&can_frame_rcv, 0, sizeof(can_frame_rcv));
  
  data_link_read(blocking);
  if (can_frame_rcv.dt == 0) {return false;}
  transport_read();
  if (j1939_pdu_rcv.len == 0) {return false;}
  return true;
}

bool j1939_write(bool blocking, uint8_t pri, uint32_t pgn, uint8_t da, uint8_t * dt){
  memset(&j1939_pdu_snd, 0, sizeof(j1939_pdu_snd));
  memset(&can_frame_snd, 0, sizeof(can_frame_snd));

  j1939_pdu_snd.pri = pri;
  j1939_pdu_snd.pgn = pgn;
  if ((j1939_pdu_snd.pgn & 0xff00) >> 8 >= 240){da = 0xff;}
  j1939_pdu_snd.da = da;
  j1939_pdu_snd.sa = SRC;
  j1939_pdu_snd.dt = dt;
  j1939_pdu_snd.len = sizeof(dt);
  
  if (!transport_write(blocking)) {return false;}
  if (!data_link_write()) {return false;}
}
