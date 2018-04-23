
//ccTalk tutorial coin acceptor host example
//Works with any stream based serial object
//Open a stream and begin, then pass to a ccTalk object

#include <NeoSWSerial.h>
#include "ccTalk.h"
#include "headers.h"

NeoSWSerial mySerial(8,9); // RX, TX
ccTalk SCA1(&mySerial);

int credit;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  SCA1.comm_init();
  while ( SCA1.RX_state != ccTalk::RXidle) {
    SCA1.ccTalkReceive();
  }
  SCA1.device_init();
  //verify inhibits are disabled, should print 255 255
  Serial.println("Getting inhibit status");
  Serial.println(SCA1.get_inhibit());
  
  //toggle inhibit to test functionality, should print 0 0
  Serial.println("Enabling inhibit");
  SCA1.inhibit_on();
  Serial.println("Getting inhibit status");
  Serial.println(SCA1.get_inhibit());
  
  SCA1.inhibit_off();
  Serial.println("Ready to accept coins");
}

void loop() {
  if (credit = SCA1.read_credit()){
    Serial.print("Credit inserted: ");
    Serial.println(credit);
  }
}