/* Implementation for all J1939 and CAN functionality */
#include "J1939.h"
#include <TeensyTimerTool.h>
using namespace TeensyTimerTool;
OneShotTimer t1;

/* Globals */
// Ideally, there can be a multitude of such buffers needed for each connection. 
// The max number is the number of devices on the network and a broadcast and this must be provided in conf.h
// Malloc for arduino may lead to quick heap fragmentation issues. 
// Accordingly, use of a static buffer is recommended for data storage etc. Teensy 3.6 has 256K of memory. 
// A J1939 connection can carry a max of 1785 bytes. Theoritically max of 500 (number of devices * 2 because of directed and BAM) connections can be active at a time.  
// This means -- 500*1785 bytes => ~ 900,000 This is way more that 256,000. CAN recommend 30 devices on a network. That means 60 connections implying ~ 110,000 bytes.
// This is still viable but given global variable space is not all of the 256K.
// Given that malloc may lead to fragmentation soon, this is the best we can do and hope the number of devices is much less than 30, may be ~10 implying 36K space.
// To use this though, we need to create a hash index for the sources and use that for rapid index retrieval and use those indexes to locate the actual buffer in the 2-d array.
// The hasmap does not need to create 1785 bytes for each row, instead only a byte (0-253)
// HERE IT GOES
uint8_t hashmap[255];
uint32_t transport_ongoing_pgns[NUM_DEVICES_ON_NETWORK*2];
uint16_t transport_buf_len[NUM_DEVICES_ON_NETWORK*2]; //For sending and recieving
uint16_t transport_buf_expected_bytes[NUM_DEVICES_ON_NETWORK*2]; //For sending and recieving
uint8_t transport_buf[NUM_DEVICES_ON_NETWORK*2][1785]; //For sending and recieving
// max storage for all of the above (assuming the CAN limit of 30 devices) = 255 + 60*4 + 60*2*2 + 60*1785 = ~108K.
// Search time is constant.
uint8_t t_buf_index = -1;
#define TPCMPF 0xec
#define TPDTPF 0xeb
extern uint16_t max_num_packets_cts = 255;
uint32_t cts_id = 0x18ec0000 | SRC; // Must reaplce the DA later
uint8_t cts_dt[8];
uint32_t rts_id = 0x18ec0000 | SRC;
uint8_t rts_dt[8];


/* Temp globals */
int key;
uint8_t da;
uint8_t sa;
uint8_t pf;
uint8_t ps;

void process_transport_async(){
  Serial.println("ABC");
}

bool transport_setup(){
  /* Setup the 200 ms timer for send operations*/
  t1.begin(process_transport_async);
  
  /* Setup transport stuff */
  for (int i = 0; i <255; i++){
    hashmap[i] = 255;
  }

  // We are using loops here, memset will internally do a loop for for each of these anyways
  for (int i = 0; i <NUM_DEVICES_ON_NETWORK*2; i++){
    transport_ongoing_pgns[i] = 0;
    transport_buf_len[i] = 0;
    transport_buf_expected_bytes[i] = 0;
    memset(transport_buf[i],0,1785);
  } 
  
  /*Setup default messages like the CTS*/
  return true;
}

int get_loc(uint8_t src, uint8_t dst){
  if (hashmap[src] == 255){
    t_buf_index++;
    hashmap[src] = t_buf_index;
  }
  
  return hashmap[src] + (dst/255);
}

void transport_read(){
    j1939_pdu_rcv.len = 0;
    pf = (can_frame_rcv.id & 0x00ff0000) >> 16;
    ps = (can_frame_rcv.id & 0x0000ff00) >> 8;
    if (pf >= 240){
      da = 0xff;
    }
    else{
          da = ps;
    }
    sa = can_frame_rcv.id & 0xff;
    if (LOOPBACK == 0 && sa == SRC){return;}
    if (!(da == SRC || da == 0xff)){return;}
    /* Is this a transport message; if so treat is specially */
    /* If this is not a transport message, pass its buffer back to the user; */
    /* Also note that even if abort, timeout etc. critical, even if they arrive they 
     * essentially abort the connection but that means the transport never forwards anything up. 
     * So, abort and timeouts canbe safely neglected.
     */
    switch (pf){
      case TPCMPF:
          if (!(can_frame_rcv.dt[0] == 0x10 || can_frame_rcv.dt[0] == 0x20)){break;} //Only RTS and BAM are treated here
          key = get_loc(sa, da);
          transport_ongoing_pgns[key] = 0;
          memcpy(&transport_ongoing_pgns[key], &can_frame_rcv.dt[5], 3);
          transport_buf_expected_bytes[key] = 0;
          memcpy(&transport_buf_expected_bytes[key], &can_frame_rcv.dt[1], 2);
          
          if (can_frame_rcv.dt[0] == 0x10){
            can_frame_snd.id = cts_id | sa << 16; 
            max_num_packets_cts = (can_frame_rcv.dt[3] < can_frame_rcv.dt[4]) ? can_frame_rcv.dt[3] : can_frame_rcv.dt[4];
            memset(cts_dt, 0xff, 8);
            cts_dt[0] = 0x11;
            cts_dt[1] = max_num_packets_cts;
            cts_dt[2] = 1;
            memcpy(&cts_dt[5], &transport_ongoing_pgns[key], 3);            
            can_frame_snd.dt = cts_dt;
            data_link_write();
          }
  
        break;
      case TPDTPF:
          key = get_loc(sa, da);
          memcpy(&transport_buf[key][(can_frame_rcv.dt[0] - 1)*7], &can_frame_rcv.dt[1], 7); //This takes care of out of order packets
          transport_buf_len[key] += 7;
          //print_ex(true, "%d %d", transport_buf_recieved_len[key], transport_buf_expected_bytes[key]);
          if (transport_buf_len[key] >= transport_buf_expected_bytes[key]){
            j1939_pdu_rcv.pri = (can_frame_rcv.id & 0x1C000000) >> 26; // Using the priority pf the data frames 
            j1939_pdu_rcv.pgn = transport_ongoing_pgns[key];
            j1939_pdu_rcv.da = da;
            j1939_pdu_rcv.sa = sa;
            j1939_pdu_rcv.dt = transport_buf[key];
            j1939_pdu_rcv.len = transport_buf_expected_bytes[key];
            transport_buf_len[key] = 0;
            /* The actual buffer to store the data can be memset to 0, but that may be a waste of time
             *  as it will eventually be overwritten. If the sending device gets all sequence numbers correct, 
             *  i.e. 1,2,3,4... and not 1,3,4,5,7.... then this buffer will always be contigously overwritten 
             *  except the trailing bytes which are covered by the len that is returned in the j1939_pdu_rcv.
             */
          }
          break;
      default:
          j1939_pdu_rcv.pri = (can_frame_rcv.id & 0x1C000000) >> 26; // Using the priority pf the data frames 
          if (pf >= 240){
            j1939_pdu_rcv.pgn = pf << 8 | ps;
          }
          else{
            j1939_pdu_rcv.pgn = pf << 8;
          }
          j1939_pdu_rcv.da = da;
          j1939_pdu_rcv.sa = sa;
          j1939_pdu_rcv.dt = can_frame_rcv.dt;
          j1939_pdu_rcv.len = 8;
          break;
    }
}

bool transport_write(bool blocking){
  
  if (j1939_pdu_snd.len == 0){return;}
  if (j1939_pdu_snd.len > 8){
    
    // Break into chunks 
    key = get_loc(j1939_pdu_snd.da, j1939_pdu_snd.sa);
    memcpy(transport_buf[key], j1939_pdu_snd.dt, j1939_pdu_snd.len);
    //Data is always sent out in order -- if retransmit is needed, it substracts the length upto that and continues
    // Send RTS, set timer, when CTS is read, the sender callback should be called
    if (!blocking){
      
    }
    else{
      can_frame_snd.id = rts_id | j1939_pdu_snd.da << 16;
      rts_dt[0] = 0x10;
      memcpy(&rts_dt[1], j1939_pdu_snd.len, 2);
      rts_dt[3] = ceil(j1939_pdu_snd.len/7);
      rts_dt[4] = max_num_packets_cts;
      memcpy(&rts_dt[4], j1939_pdu_snd.pgn, 3); 
      data_link_write();
    }    
    
  }
  else{
    if ((j1939_pdu_snd.pgn & 0xff00) >> 8 < 240){j1939_pdu_snd.pgn = (j1939_pdu_snd.pgn & 0xff00) | j1939_pdu_snd.da;}
    can_frame_snd.id = j1939_pdu_snd.pri << 26 | j1939_pdu_snd.pgn << 8 | SRC;
    can_frame_snd.dt = j1939_pdu_snd.dt;
  }
  
  return true;
}
