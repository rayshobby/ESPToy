/* ====== ESP8266 Toy Demo ============
 * A simple web server (SD verison)
 * - serve HTML page from external SD card
 * - read and display analog pin values
 * - set RGB led color
 * ====================================
 *
 * This demo requires connecting an external
 * SD card module to ESPToy, and copy the index.htm
 * to the card. The SD card uses SPI interface --
 * most of the SPI pins (SCK, MISO, MOSI)
 * are available in the ISP pinout area; the 
 * chip select (CS) pin is set to D4 in this demo.
 * You can change the CS pin to other pins available
 * in the pinour area.
 *
 * Change SSID and PASS to match your WiFi settings.
 * The IP address is displayed to debug Serial
 * upon successful connection.
 *
 * Written by Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=10156
 */
 
#define ESP_BAUDRATE 115200  // assume 115200 baud rate

#include <SdFat.h>

#define SSID  "your_ssid"      // change this to match your WiFi SSID
#define PASS  "your_password"  // change this to match your WiFi password
#define PORT  "80"             // using port 80 by default

#define PIN_BUTTON  3
#define PIN_RED     12
#define PIN_GREEN   13
#define PIN_BLUE    14
#define PIN_SD_CS   4        // CS (chip select pin) of SD card

// Ethernet buffer
#define BUFFER_SIZE 1000
char buffer[BUFFER_SIZE];

SdFat sd;
byte red, green, blue;

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

byte findKeyVal (const char *str,char *strbuf, uint8_t maxlen,const char *key);

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

// send an AT command with timeout (default 1 second)
byte send_at(String comm, int timeout=1000) {
  esp.println(comm);
  dbg.println(comm);
  return wait_for_esp_response(timeout);
}

void setup() {

  // setup GPIO pins
  digitalWrite(PIN_BUTTON, HIGH);
  pinMode(PIN_BUTTON, OUTPUT);
  
  digitalWrite(PIN_RED, LOW);
  pinMode(PIN_RED, OUTPUT);
  
  digitalWrite(PIN_GREEN, LOW);
  pinMode(PIN_GREEN, OUTPUT);

  digitalWrite(PIN_BLUE, LOW);
  pinMode(PIN_BLUE, OUTPUT);

  esp.begin(ESP_BAUDRATE);
  
  dbg.begin(9600);

  // set up sd card
  if(!sd.begin(PIN_SD_CS, SPI_HALF_SPEED)) {
    dbg.println(F("Failed to initialize SD card!"));
  }
  
  setupWiFi();

  // print device IP address
  dbg.print(F("ip addr:"));
  send_at("AT+CIFSR");
  dbg.print(buffer);
}

// read data from ESP8266 until \r\n is found
// non-blocking
bool read_till_eol() {
  static int i=0;
  if(esp.available()) {
    buffer[i++]=esp.read();
    if(i==BUFFER_SIZE)  i=0;
    if(i>1 && buffer[i-2]==13 && buffer[i-1]==10) {
      buffer[i]=0;
      i=0;
      return true;
    }
  }
  return false;
}

// extract the action string of get command
void get_comm(char* src, char* dst, int size) {
  int i;
  for(i=0;i<size;i++) {
    dst[i]=src[i];
    if(dst[i]==' ') break;
  }
  dst[i]=0;
}

void loop() {
  int ch_id, packet_len;
  char *pb;  
  if(read_till_eol()) {
    dbg.print(buffer);
    if(strncmp(buffer, "+IPD,", 5)==0) {
      // request format: +IPD,ch,len:data
      sscanf(buffer+5, "%d,%d", &ch_id, &packet_len);
      if (packet_len > 0) {
        pb = buffer+5;
        while(*pb!=':') pb++;
        pb++;
        if (strncmp(pb, "GET / ", 6) == 0) {
          dbg.println(F("-> serve homepage"));
          wait_for_esp_response();
          serve_file("index.htm", ch_id);
        } else if (strncmp(pb, "GET /favicon.ico ", 17) == 0) {
          dbg.println(F("-> serve favicon"));
          wait_for_esp_response();          
          serve_favicon(ch_id);          
        } else if (strncmp(pb, "GET /ja ", 8) == 0) {
          dbg.println(F("-> serve ja"));
          wait_for_esp_response();
          serve_ja(ch_id);
        } else if (strncmp(pb, "GET /cc?", 8) == 0) {
          dbg.println(F("->cc received"));
          char tmp[16];
          get_comm(pb+8, tmp, 16);
          wait_for_esp_response();
          serve_change_color(ch_id, tmp);
        }  else {
          // no service handler, assume requesting a file on the SD card
          int i=0;
          while(1) {
            pb[i]=pb[i+5];
            if(pb[i]==' ') {
              pb[i]=0;
              break;
            }
            i++;
          }
          dbg.print("-> serve ");
          dbg.print(pb);
          char tmp[16];
          strcpy(tmp, pb);
          wait_for_esp_response();
          serve_file(tmp, ch_id);
        }
      }
    }
  }
}

// send data using the AT+CIPSEND command
// close the connection if close==true
bool send_data(String data, int ch_id, bool close=false) {
  bool success = false;
  esp.print("AT+CIPSEND=");
  esp.print(ch_id);
  esp.print(",");
  esp.println(data.length());
  if(wait_for_esp_response(2000, "> ")) {
    esp.print(data);
    if(wait_for_esp_response(5000)) {
      success = true;
    }
  }
  if(close) {
    esp.print("AT+CIPCLOSE=");
    esp.println(ch_id);
    wait_for_esp_response();
  }
  return success;
}

// change RGB LED color
void change_color(char* comm, char* key, int pin, byte* pin_var=NULL) {
  char tmp[10];
  if(findKeyVal(comm, tmp, 10, key)) {
    int i=atoi(tmp);
    if(i>=0 && i<=255) {
      analogWrite(pin, i);
      if(pin_var) *pin_var = i;
    }
  }
}

// get command to handle color change
void serve_change_color(int ch_id, char* getcomm) {
  change_color(getcomm, "r", PIN_RED, &red);
  change_color(getcomm, "g", PIN_GREEN, &green);
  change_color(getcomm, "b", PIN_BLUE, &blue);
  
  String html = "HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-cache\r\n\r\n{\"result\":0}";
  send_data(html, ch_id, true);
}


void serve_ja(int ch_id) {
  String html = "HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-cache\r\n\r\n";
  html+="{\"a\":[";
  for(byte i=0;i<8;i++) {
    html+=analogRead(i);
    if(i!=7) html+=",";
  }
  html+="],\"b\":";
  html+=(int)(digitalRead(PIN_BUTTON)?0:1);
  html+=",\"red\":";
  html+=(int)red;
  html+=",\"green\":";
  html+=(int)green;
  html+=",\"blue\":";
  html+=(int)blue;
  html+="}";
  send_data(html, ch_id, true);
}

void serve_favicon(int ch_id) {
  String html = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-Length:0\r\n\r\n";
  send_data(html, ch_id, true);
}


void serve_file(char* name, int ch_id) {
  if(!sd.exists(name)) {
    dbg.println(F("file not found"));
    return;
  }
  SdFile file;
  file.open(name, O_READ); 
  String html="HTTP/1.0 200 OK\r\n\r\n";
  send_data(html, ch_id);

  unsigned long nbytes = file.fileSize();
  int nread;
  while(nbytes>=BUFFER_SIZE) {
    esp.print("AT+CIPSEND=");
    esp.print(ch_id);
    esp.print(",");
    esp.println(BUFFER_SIZE);
    dbg.println(BUFFER_SIZE);
    if(wait_for_esp_response(2000, "> ")) {
      nread = file.read(buffer, BUFFER_SIZE);
      esp.write((byte *)buffer, nread);
      if(wait_for_esp_response(5000)) {
        //dbg.println("SEND ok");
      }
    }
    nbytes -= nread;
  }
    
  // last packet
  if(nbytes > 0) {
    esp.print("AT+CIPSEND=");
    esp.print(ch_id);
    esp.print(",");
    esp.println(nbytes);
    dbg.println(nbytes);
    if(wait_for_esp_response(2000, "> ")) {
      nread = file.read(buffer, nbytes);
      esp.write((byte *)buffer, nbytes);
      if(wait_for_esp_response(5000)) dbg.println("SEND ok");
    }      
    nbytes = 0;  
  }
  file.close();
  esp.print("AT+CIPCLOSE=");
  esp.println(ch_id);
  wait_for_esp_response();
}

void setupWiFi() {
  // query esp
  send_at("AT");

  // set mode 1 (client)
  send_at("AT+CWMODE=1");

  // reset WiFi module
  send_at("AT+RST", 3000);
 
  // join AP
  // this may take a while, so set 5 seconds timeout
  String comm="AT+CWJAP=\"";
  comm+=SSID;
  comm+="\",\"";
  comm+=PASS;
  comm+="\"";
  send_at(comm, 5000);

  // start server
  send_at("AT+CIPMUX=1");
  
  comm="AT+CIPSERVER=1,"; // turn on TCP service
  comm+=PORT;
  send_at(comm);
}

// Helper function to find key-value pair in an HTTP GET command string
byte findKeyVal (const char *str,char *strbuf, uint8_t maxlen,const char *key)
{
    uint8_t found=0;
    uint8_t i=0;
    const char *kp;
    kp=key;
    
    while(*str &&  *str!=' ' && *str!='\n' && found==0){
        if (*str == *kp){
            kp++;
            if (*kp == '\0'){
                str++;
                kp=key;
                if (*str == '='){
                    found=1;
                }
            }
        }else{
            kp=key;
        }
        str++;
    }

    if (found==1){
        // copy the value to a buffer and terminate it with '\0'
        while(*str &&  *str!=' ' && *str!='\n' && *str!='&' && i<maxlen-1){
            *strbuf=*str;
            i++;
            str++;
            strbuf++;
        }
        *strbuf='\0';
    }
    return(i);
}
