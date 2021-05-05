/* ---------------- Tester --------------- */
# include "J1939.h"
uint64_t data_num_64;

void setup() {
  J1939_setup();
}

void loop() {
  delay(1000);
  if (j1939_read(false)){
    if (j1939_pdu_rcv.len > 8){ 
      print_ex(true, "pri: %d, pgn: %x, da: %x, sa: %x, len: %d, data: %s", j1939_pdu_rcv.pri, j1939_pdu_rcv.pgn, j1939_pdu_rcv.da, j1939_pdu_rcv.sa, j1939_pdu_rcv.len, j1939_pdu_rcv.dt);
    }
   else{
      memcpy(&data_num_64, j1939_pdu_rcv.dt, 8);
      print_ex(false, "pri %d, pgn: %x, da: %x, sa: %x, len: %d, data: ", j1939_pdu_rcv.pri, j1939_pdu_rcv.pgn, j1939_pdu_rcv.da, j1939_pdu_rcv.sa, j1939_pdu_rcv.len);
      Serial.println(PriUint64<HEX>(data_num_64));   
    }
  }
  
  data_num_64 = __bswap64(0xdeadbeefcafebabe);
  if (j1939_write(false, 3, 0xf004, 0xff, (uint8_t *)&data_num_64));
}
