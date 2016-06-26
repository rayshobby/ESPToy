/* ======== ESP8266 Demo ========
 * Send AT command through Serial
 * ==============================
 *
 * Type AT command in Serial monitor
 * to interact with ESP8266
 *
 * If using Arduino's Serial monitor,
 * choose line ending to be 'Both NL & CR'.
 *
 * Written by Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=10156
 */

/* ======================================
 * If you don't know ESP's baud rate,
 * use the SearchBaud example to find it.
 * ======================================
 */
#define ESP_BAUD_RATE 115200 // assume esp's baud rate is 115200

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

void setup() {
  dbg.begin(DBG_BAUD_RATE);
  esp.begin(ESP_BAUD_RATE);  
}

void loop() {
  if(dbg.available()) {
    char c = dbg.read();
    
/* If your Serial monitor cannot append
 * \r\n at the end of a line, uncomment
 * the code below to use character '&'
 * to end the line. For example:
 * AT&
 * AT+GMR&
 */ 
    /*if(c=='&') {  // use & to end the line if your serial monitor
                  // does not automatically append \r\n in the end
      esp.write('\r');
      esp.write('\n');
    } else*/
    {
      esp.write(c);
    }
  }
  if(esp.available()) {
    dbg.write(esp.read());
  }
}


