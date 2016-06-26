/* ======== ESP8266 Demo ========
 * Search for ESP8266's baud rate
 * ==============================
 *
 * The baud rate is displayed to
 * Serial monitor
 *
 * Written by Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=10156
 */

// If using software Serial for debugging
// use the definitions below
/*#include <SoftwareSerial.h>
SoftwareSerial dbg(7,8);  // use pins 7, 8 for software Serial 
#define esp Serial
#define DBG_BAUD_RATE 9600  // software Serial can only go up to 9600bps
*/

// If your MCU has dual hardware USARTs (e.g. ATmega644)
// use the definitions below
#define dbg Serial    // use Serial for debug
#define esp Serial1   // use Serial1 to talk to esp8266
#define DBG_BAUD_RATE 115200  // hardware Serial can go up to 115200bps

char buffer[100];
const unsigned long baudrate[] = {9600, 19200, 28800, 38400, 57600, 115200};

// By default we are looking for OK\r\n
char OKrn[] = "OK\r\n";
// wait for ESP8266 to respond until string term is found
// or timeout has reached
// return true if term is found
bool wait_for_esp_response(int timeout=1000, char* term=OKrn) {
  unsigned long t=millis();
  bool found=false;
  int i=0;
  int len=strlen(term);
  // wait for at most timeout milliseconds
  // or if term is found
  while(millis()<t+timeout) {
    if(esp.available()) {
      buffer[i++]=esp.read();
      if(i>=len) {
        if(strncmp(buffer+i-len, term, len)==0) {
          found=true;
          break;
        }
      }
    }
  }
  buffer[i]=0;
  return found;
}

void setup() {

  dbg.begin(DBG_BAUD_RATE);
  dbg.print("ESP baud rate is: ");
  int i;
  for(i=0;i<sizeof(baudrate)/sizeof(unsigned long);i++) {
    esp.begin(baudrate[i]);
    esp.print("AT\r\n");
    if(wait_for_esp_response()) {
      dbg.println(baudrate[i]);
      break;
    }
  }
  if(i==sizeof(baudrate)/sizeof(unsigned long)) {
    dbg.println("not found.");
  }
}

void loop() {

}


