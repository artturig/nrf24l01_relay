#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

int relay = 8;

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// Radio pipe addresses for the 2 nodes to communicate.
//const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
//const uint8_t addresses[][6] = {"1Node","2Node"};
byte addresses[][6] = {"1Node", "2Node", "3Node"};

char * convertNumberIntoArray(unsigned short number, unsigned short length) {
  char * arr = (char *) malloc(length * sizeof(char)), * curr = arr;
  do {
    *curr++ = number % 10;
    number /= 10;
  } while (number != 0);
  return arr;
}

unsigned short getId(char * rawMessage, unsigned short length){
  unsigned short i = 0;
  unsigned short id = 0;
  for( i=1; i< length; i++){
    id += rawMessage[i]*pow( 10, i-1 );
  }
  return id;
}

unsigned short getMessage( char * rawMessage){
  unsigned short message = rawMessage[0];
  return (unsigned short)message;
}
unsigned short getLength( unsigned int rudeMessage){
  unsigned short length = (unsigned short)(log10((float)rudeMessage)) + 1;
  return length;
}

void setup(void)
{
  Serial.begin(57600);
  pinMode(relay, OUTPUT);
 // digitalWrite(relay, HIGH);
  printf_begin();
  printf("\nRemote Switch Arduino\n\r");

  //
  // Setup and configure rf radio
  //

  radio.begin();
//  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.setRetries(15,15);

  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  radio.startListening();
  radio.printDetails();
}

// read current relay state from pin
int getState(unsigned short pin){
  boolean state = digitalRead(pin);
  state == true ? 0 : 1;
  sendToListener(state);
  return state;
}

// send message to listener so we can save action to database
void sendToListener(unsigned short action) {
  String stringOne = "2Node:";
     // First, stop listening so we can talk
  radio.stopListening();
  stringOne += action;
        // add leading ; to notify receiver
  stringOne += ";";
        // get stingOne length
  int length = stringOne.length()+1;

        // define charBuf size
  char charBuf[length];

        // convert stringOne to char array
  stringOne.toCharArray(charBuf, length);

        // write charBuf and length to radio aka send stuff
  bool ok = radio.write(&charBuf, length);
          // check if write to radio is successfull
  if (ok) {
    Serial.println("Transfer OK");
  } else {
    Serial.println("Transfer Fail");
  }

        // Now, resume listening so we catch the next packets and get back to main loop
  radio.startListening();
}


void doAction(unsigned short id, unsigned short action){

  if( action == 0 ){
        // turn relay off
    digitalWrite(id, HIGH);
    sendToListener(action);

  }else{
        // turn relay on
    digitalWrite(id, LOW);
    sendToListener(action);
  }

}
void sendCallback(unsigned short callback){
   // First, stop listening so we can talk
  radio.stopListening();

      // Send the final one back.
  radio.write( &callback, sizeof(unsigned short) );
  printf("Sent response.\n\r");

      // Now, resume listening so we catch the next packets.
  radio.startListening();
}

void performAction(unsigned short rawMessage){
  unsigned short action, id, length, callback;
  char * castedMessage;

  length = getLength(rawMessage);
  castedMessage = convertNumberIntoArray(rawMessage, length);
  action = getMessage(castedMessage);
  id = getId(castedMessage, length);

  if (action == 0 || action ==1){
    callback = action;
    doAction(id, action);
  }else if(action == 2){
    callback = getState(id);
  }
  //sendCallback(callback);
}

void loop(void)
{
    // if there is data ready
  if ( radio.available() )
  {
      // Dump the payloads until we've gotten everything
    unsigned short message;
    bool done;
//      char * new;
    unsigned short rawMessage;
    done = false;
    while ( radio.available() )
    {
        // Fetch the payload, and see if this was the last one.
      radio.read( &rawMessage, sizeof(unsigned long) );
        // Spew it
        //printf("Got message %d...",rawMessage);
      performAction(rawMessage);

      delay(10);
    }
  }
}
