#ifndef ccTalk_h
#define ccTalk_h
#include "Arduino.h"

#define MAXDATALENGTH 58

class milistimer
{
  public:
    void startt(int tdelay); // start the countdown with tdelay in miliseconds
    bool isready(void);// return true if timer expired
  private:
    unsigned long target; 
};

class ccTalk : public Stream {
public:

  ccTalk(Stream *s=&Serial);

  size_t
    write(uint8_t c);
  int available();
  int read();
  int peek();
  void flush();

  // RX state machines coding
  enum RX_State_ {RXidle,RXloop,RXansw,RXcomplete,RXflush,RXerr_unexpected_byte_in_idle,RXerr_no_loopback,RXerr_wrong_loopback,RXerr_answer_timeout,\
        RXerr_msg_length,RXerr_checksum_failed} RX_state;

  static const char* const RX_msg[];
  //static const char* RX_msg[] = {"RXidle","RXloop","RXansw","RXcomplete","RXflush","RXerr_unexpected_byte_in_idle","RXerr_no_loopback","RXerr_wrong_loopback","RXerr_answer_timeout",\
      "RXerr_msg_length","RXerr_checksum_failed"};

  unsigned char RX_buffer[64];// the receive buffer 
  unsigned char TX_buffer[64];// the transmit buffer , not really because is used only to format the message and to compare with loopback
  unsigned char RX_bytecount; // used to count the bytes received

  unsigned int coin_value[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // to store the coin value
  unsigned int coineventcounter;//used to follow the events from read credit or error codes
  unsigned int buffered_events; //how many events are queued
  int credit;

  const unsigned char maxDataLength = MAXDATALENGTH; // the maximum data field length
  const unsigned long interbytetimeout = 100;// the timeout for next byte in the same message 
  const unsigned long answertimeout = 300;// the answer timeout , it might be longer for some pools like eeprom writing

  void ccTalkReceive(); //the main RX process to be called in loop 
  int watchdog_poll();
  // cctalk receive process functions , the main process should be called until RX_state >= RX_idle
  void comm_init();// opens the serial port and flush the RX buffer 
  void device_init(); //resets device, disables inhibits and sets coin values
  void diagnostic();// runs initialization sequence
  void device_reset(); //just resets device
  char * get_master_inhibit(); //get master inhibit status
  void master_inhibit_on(); //disable coins
  void master_inhibit_off(); //enable coins
  char * get_inhibit(); //get individual inhibits
  void inhibit_on(); //disable coins (all individual channels)
  void inhibit_off(); //enable coins (all individual channels)
  int read_credit(); //get credits
  void breakrx();// flush the RX buffer - Usable from any RX state 
  void clearrxerror();// flush the buffer without restarting the timer, usable only from RX error states


  void startrx();// init the RX process to wait for an answer , use after sending a pool with cctsend, 
  // the main process should be called until RX_state >= RXcomplete


  // Send a pool command = cctalk pool byte , see headers.h
  // dest = destination field 40 for bill acceptor, 2 for coin acceptor etc
  // D00..D07 = data field , you dont need to fill all if the data field is shorter
  // if you need longer data field then fill the next bytes in the TX_buffer[12] and higher
  void cctsend(unsigned char command,unsigned char  dest,unsigned char msglength,\
    unsigned char D00=0,unsigned char D01=0,unsigned char D02=0,unsigned char D03=0,
    unsigned char D04=0,unsigned char D05=0,unsigned char D06=0,unsigned char D07=0);

  char * printASCIIdata(); //returns the ASCII data string

  char * printBINdata(); //returns the bin data field as ASCII string

private:
  Stream
    *stream;

  char get_data[MAXDATALENGTH];
  int length;

  milistimer comt;
  milistimer somedelay;// to handle the break after a comm error
};

#endif
