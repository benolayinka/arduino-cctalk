#include "Arduino.h"
#include "ccTalk.h"
#include "headers.h"
//

//The millistimer class used for delays
void milistimer::startt(int tdelay) {// start the timer witth delay in miliseconds. 
  unsigned long temp;
  temp = millis();
  target = temp + tdelay;
};

bool milistimer::isready(void) {//return true if timer is expired 
  unsigned long temp;
  temp = millis();
  if (target - temp > 86400000) // one day
    return true;
  else
    return false;
};

//constructor
ccTalk::ccTalk(Stream *s) : stream(s) {
  RX_bytecount = 0;
}

const char* const ccTalk::RX_msg[] = {"RXidle","RXloop","RXansw","RXcomplete","RXflush","RXerr_unexpected_byte_in_idle","RXerr_no_loopback","RXerr_wrong_loopback","RXerr_answer_timeout",\
      "RXerr_msg_length","RXerr_checksum_failed"};

size_t ccTalk::write(uint8_t c){
  stream->write(c);
};

int ccTalk::available(){
  return stream->available();
};

int ccTalk::read(){
  return stream->read();
};

int ccTalk::peek(){
  return stream->peek();
};

void ccTalk::flush(){
  stream->flush();
};

void ccTalk::cctsend(unsigned char command,unsigned char  dest,unsigned char msglength,\
unsigned char D00,unsigned char D01,unsigned char D02,unsigned char D03,\
unsigned char D04,unsigned char D05,unsigned char D06,unsigned char D07){
  unsigned char temp = 0;
  TX_buffer[0] = dest;
  TX_buffer[1] = msglength;
  TX_buffer[2] = 1; // implicit source , only for 8 bit checksum
  TX_buffer[3] = command;
  TX_buffer[4] = D00;TX_buffer[5] = D01;TX_buffer[6] = D02;TX_buffer[7] = D03;
  TX_buffer[8] = D04;TX_buffer[9] = D05;TX_buffer[10] = D06;TX_buffer[11] = D07;
  TX_buffer[msglength+4]=0;
  for (temp = 0 ; temp < msglength+4 ; temp++){// a simple checksum and send the message
    TX_buffer[msglength+4]-=TX_buffer[temp];
    stream->write(TX_buffer[temp]);
  }
    stream->write(TX_buffer[msglength+4]);// and the simple checksum
  startrx();// Start the receiving the answer 
}

void ccTalk::comm_init(){// init the serial port and flush RX buffer
  breakrx();
}

int ccTalk::watchdog_poll(){
  cctsend(simple_poll, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive(); // wait for an answer or error
  }
  if (RX_state != RXcomplete) {
    return 0;
  } else {
    return 1;
  }
    
}

void ccTalk::device_reset(){
  cctsend(reset_device, 2, 0); //ben added because credit buffer stays if power isn't reset
  while (RX_state < RXcomplete) {
    ccTalkReceive(); // wait for an answer or error
  }
  // if (RX_state != RXcomplete) //error handling
}

void ccTalk::device_init(){
  cctsend(simple_poll, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive(); // wait for an answer or error
  }
  // if (RX_state != RXcomplete) //error handling

  cctsend(reset_device, 2, 0); //ben added because credit buffer stays if power isn't reset
  while (RX_state < RXcomplete) {
    ccTalkReceive(); // wait for an answer or error
  }
  // if (RX_state != RXcomplete) //error handling

  for (int i = 0; i < 16 ; i++) coin_value[i] = 0; // Clean the coin value array

  // Get coin ID, filter garbage convert and store in coin_value as unsigned int
  for (unsigned char i = 1; i < 17; i++) {
    somedelay.startt(2);//for some unknown reason a little delay is needed between polls
    do {
    } while (!somedelay.isready());
    cctsend(request_coin_id, 2, 1, i);
    while (RX_state < RXcomplete) {
      ccTalkReceive();
    }
    if (RX_state != RXcomplete) {
      // if (RX_state != RXcomplete) //error handling
      //Serial.print(RX_msg[RX_state]);// Show the error if any
      //Serial.print(" at coin channel ");
      //Serial.println(i);
      break;
    }
    if ((RX_buffer[4] == 32) && (RX_buffer[4] == 32) && (RX_buffer[4] == 32) && (RX_buffer[4] == 32) && (RX_buffer[4] == 32) && (RX_buffer[4] == 32)) continue;
    if ((RX_buffer[4] == 46) && (RX_buffer[4] == 46) && (RX_buffer[4] == 46) && (RX_buffer[4] == 46) && (RX_buffer[4] == 46) && (RX_buffer[4] == 46)) continue;
    if ((65 > RX_buffer[4]) || (RX_buffer[4] > 90) || (65 > RX_buffer[5]) || (RX_buffer[5] > 90) || (65 > RX_buffer[9]) || (RX_buffer[9] > 90)) break;
    if ((48 > RX_buffer[6]) || (RX_buffer[6] > 57) || (48 > RX_buffer[7]) || (RX_buffer[7] > 57) || (48 > RX_buffer[8]) || (RX_buffer[8] > 57)) break;
    // printASCIIdata();// Print ASCII coin ID
    coin_value[i - 1] = (RX_buffer[6] - 48) * 100 + (RX_buffer[7] - 48) * 10 + RX_buffer[8] - 48;
  }
  //Serial.println("Coin values OK");
  //Serial.println("Setting individual inhibits");
  cctsend(modify_inhibit_status, 2, 2, 255, 255); //just enable all channels
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  //if (RX_buffer[3] == 0) Serial.println("OK");
  //else {
  //  Serial.println("Error setting inhibits");
  //}

  //Serial.println("Setting master inhibit");
  cctsend(modify_master_inhibit_status, 2, 1, 1);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  //if (RX_buffer[3] == 0) Serial.println("OK");
  //else {
  //  Serial.println("Error setting master inhibit");
  //}

}

char * ccTalk::get_master_inhibit(){
  //Serial.println("Getting master inhibit");
  cctsend(request_master_inhibit_status, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  return printBINdata();
}

void ccTalk::master_inhibit_off(){
  //Serial.println("Setting master inhibit");
  cctsend(modify_master_inhibit_status, 2, 1, 1);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // if (RX_buffer[3] == 0) Serial.println("OK");

}

void ccTalk::master_inhibit_on(){
  //Serial.println("Setting master inhibit");
  cctsend(modify_master_inhibit_status, 2, 1, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // if (RX_buffer[3] == 0) Serial.println("OK");
}

char * ccTalk::get_inhibit(){
  //Serial.println("Getting individual inhibit");
  cctsend(request_inhibit_status, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  return printBINdata();
}

void ccTalk::inhibit_off(){
  cctsend(modify_inhibit_status, 2, 2, 255, 255); //just enable all channels
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // if (RX_buffer[3] == 0) Serial.println("OK");
}

void ccTalk::inhibit_on(){
  cctsend(modify_inhibit_status, 2, 2, 0, 0); //just disable all channels
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // if (RX_buffer[3] == 0) Serial.println("OK");

}



void ccTalk::diagnostic(){
  cctsend(simple_poll, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive(); // wait for an answer or error
  }
  // if (RX_state != RXcomplete) //error handling

  cctsend(reset_device, 2, 0); //ben added because credit buffer stays if power isn't reset
  while (RX_state < RXcomplete) {
    ccTalkReceive(); // wait for an answer or error
  }
  // if (RX_state != RXcomplete) //error handling

  cctsend(request_manufacturer_id, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // printASCIIdata(); // Print the manufacturer ID

  cctsend(request_equipment_category_id, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // printASCIIdata(); // Print the equipment category ID

  cctsend(request_product_code, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // printASCIIdata(); // Print the product code

  cctsend(request_software_revision, 2, 0);
  while (RX_state < RXcomplete) {
    ccTalkReceive();
  }
  // if (RX_state != RXcomplete) //error handling
  // printASCIIdata(); // Print the Software revision

}

int ccTalk::read_credit(){
  cctsend(read_buffered_credit_or_error_codes, 2, 0);
    while (RX_state < RXcomplete) {
      ccTalkReceive();
    }
    if (RX_state != RXcomplete) {
      return -1; 
    }

    somedelay.startt(2);//for some unknown reason a little delay is needed between polls
    do {
    } while (!somedelay.isready());
    // find how many new events are queued, RX_buffer[4] data contains the event counter
    if (RX_buffer[4] >= coineventcounter)  buffered_events = RX_buffer[4] - coineventcounter;
    else buffered_events = RX_buffer[4] + 255 - coineventcounter;

    if (buffered_events > 5) {// overflow,events lost, put some error handling here
      buffered_events = 0;
      coineventcounter = RX_buffer[4]; //Clear the queued events
    }
    
    credit = 0;
    while (buffered_events > 0) {// Read buffered events one by one
      buffered_events--;
      coineventcounter = (coineventcounter + 1) % 256;//increment event counter
      if (coineventcounter == 0) coineventcounter = 1; // skip 0
      if (RX_buffer[(buffered_events << 2) + 5] == 0) { // event A = 0 means an error or coin rejected
        return -1;
        //Serial.print("Some error or coin rejected, error code: ");
        //Serial.println(RX_buffer[(buffered_events << 2) + 6]);// then event B is the error code
      } else {
        credit += coin_value[RX_buffer[(buffered_events << 2) + 5] - 1];
        //Serial.print("Sorter path ");
        //Serial.println(RX_buffer[(buffered_events << 2) + 6]); // event B show the sorter path
      }
    }
    return credit; //buffered credit
}

void ccTalk::breakrx(){ // stop transmission right away and flush the buffer
                //the transmission buffer might still send the remaining bytes                
                // cctalkreceive job will put the RX_state to RX_idle when ready
  RX_state = RXflush;
  comt.startt(answertimeout);
}

void ccTalk::clearrxerror(){
  RX_state = RXflush;// does not set the timer since it was allready set when the state was changed to error.
}

void ccTalk::startrx(){// put RX to wait for an answer . To use after cctsend only
          RX_state = RXloop;
          comt.startt(interbytetimeout);
          RX_bytecount = 0;
}

char * ccTalk::printASCIIdata() { // returns the ascii data field
  for (int i = 4 ; i < RX_buffer[1] + 4; i++) {
      //Serial.write(RX_buffer[i]);
      get_data[i-4] = RX_buffer[i];
    }
    get_data[RX_buffer[1]+4] = (char)0;
    return get_data;
  }
    
char * ccTalk::printBINdata() { // returns the bin data field as ASCII string
    length = 0;
    for (int i = 4 ; i < RX_buffer[1] + 4; i++) {
        length+= snprintf(get_data+length, MAXDATALENGTH-length, "%d ", RX_buffer[i]);
      }
    return get_data;
    }

void ccTalk::ccTalkReceive() {// the main process , should be called often enough. 
  unsigned char temprx;
  do {// at least one pass even there is no char in the buffer
    // as many passes as bytes in the receive buffer
    //Serial.println(RX_msg[RX_state]); //debugging
    switch (RX_state) {
      case RXidle: { // If a character was received then change the status to flush;
          if (stream->available()) {
            RX_state = RXflush;
            comt.startt(answertimeout);//set the flush timer
            temprx = stream->read();//blind read the received byte 
          }
          break;
        }
      case RXloop: {
          if (stream->available()) { // ignore the timeout if there is a char on queue
            temprx = stream->read();
            RX_buffer[RX_bytecount] = temprx;
            if (RX_buffer[RX_bytecount] != TX_buffer[RX_bytecount]) { //error , wrong loopback
              RX_state = RXerr_wrong_loopback;
              comt.startt(answertimeout);
              break;
            } else {                                               // no loopback error
              RX_bytecount++;
              if (RX_bytecount > (RX_buffer[1] + 4)) { //loopback ready, prepare for the answer
                RX_bytecount = 0;// reset the byte counter
                RX_state = RXansw;//
                comt.startt(answertimeout);// set the timeout for the answer
              } else {                             //loopback in progress
                comt.startt(interbytetimeout);//set the timeout for the next byte
              }
            }
          } else {
            if (comt.isready()) { // timeout
              RX_state = RXerr_no_loopback;// Error
              comt.startt(answertimeout);
            }
          }
          break;
        }
      case RXansw: {
          if (stream->available()) { // ignore the timeout if there is a char on queue
            temprx = stream->read();
            RX_buffer[RX_bytecount] = temprx;
            // skip source and destination check leaving them for later
            if ((RX_bytecount >= 1) && (RX_buffer[1] > maxDataLength)) { // check message legth
              RX_state = RXerr_msg_length;
              comt.startt(answertimeout);
              break;
            }
            RX_bytecount++;
            if (RX_bytecount > (RX_buffer[1] + 4)) { //message complete
              unsigned char checksum = 0;
              // do checksum
              for (RX_bytecount = 0; RX_bytecount++ ; RX_bytecount <= RX_buffer[1] + 4) {
                checksum = RX_buffer[RX_bytecount];
              }
              if (checksum != 0) { //checksum error
                RX_state = RXerr_checksum_failed;
                comt.startt(answertimeout);
                break;
              } else {// successfull
                RX_state = RXcomplete;// no timeout needed , a message can be sent right away
                break;
              }
            };
          } else {
            if (comt.isready()) { // timeout
              RX_state = RXerr_answer_timeout;// Error
              if (RX_bytecount > 0){//no reason to set the timeout if allready was a timeout
                comt.startt(answertimeout); 
              }
            }
          }
          break;
        }
      case RXcomplete: {
          //do nothing, stay here until the main program does something
          // just ignore all other bytes received if a complete correct answer was received
          // they will be handled when going to Idle state
          break;
        }
      case RXflush: {
          if (stream->available()) { //read all bytes until answer timeout
            temprx = stream->read();
            comt.startt(answertimeout);
          }
          if (comt.isready()) { // timeout
            RX_state = RXidle;// Error
          }
          break;
        }
      default: {// here are all errors
        // nothing can be done here we only keep an eye on the break length
          if (stream->available()) { //read all bytes, the timer is set where the error is found.
            temprx = stream->read();// 
            comt.startt(answertimeout);//restart the timer if a character is received
          }
        }
    }
  } while (stream->available());
}
