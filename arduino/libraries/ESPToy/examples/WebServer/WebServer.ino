/* ====== ESP8266 Toy Demo ============
 * A simple web server
 * - serve HTML page from flash memory
 * - read and display analog pin values
 * - set RGB led color
 * ====================================
 *
 * Change SSID and PASS to match your WiFi settings.
 * The IP address is displayed to debug Serial
 * upon successful connection.
 *
 * Written by Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=10156
 */

#define SSID  "your_ssid"      // change this to match your WiFi SSID
#define PASS  "your_password"  // change this to match your WiFi password
#define PORT  "80"             // using port 80 by default

/* ======================================
 * If you don't know ESP's baud rate,
 * use the SearchBaud example to find it.
 * ======================================
 */
#define ESP_BAUD_RATE 115200 // assume 115200 baud rate

#define PIN_BUTTON  3
#define PIN_RED     12
#define PIN_GREEN   13
#define PIN_BLUE    14

// Ethernet buffer
#define BUFFER_SIZE 1000
char buffer[BUFFER_SIZE];

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

  esp.begin(ESP_BAUD_RATE);
  
  dbg.begin(DBG_BAUD_RATE);
  
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
          serve_homepage(ch_id);
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

prog_char homepage[] PROGMEM = "<!DOCTYPE html><html> <head><title>ESP8266</title> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"stylesheet\" href=\"http://code.jquery.com/mobile/1.3.1/jquery.mobile-1.3.1.min.css\" type=\"text/css\"> <script src=\"http://code.jquery.com/jquery-1.9.1.min.js\" type=\"text/javascript\"></script> <script src=\"http://code.jquery.com/mobile/1.3.1/jquery.mobile-1.3.1.min.js\" type=\"text/javascript\"></script> </head> <body> <div data-role=\"page\"> <div data-role=\"header\"> <h3>ESP8266</h3> </div><div data-role=\"content\"> <table cellpadding=\"2\"> <script>function w(s){document.write(s);}function p(s){w('<label for=\"'+s+'\"><b>'+s+'</b></label><input type=\"range\" id=\"'+s+'\" value=0 min=0 max=255 data-mini=\"true\"></input>');}var i; for(i=0;i<8;i++){w('<tr><td><b>A'+i+'</b>:</td><td><label id=\"lbl_a'+i+'\"</label></td></tr>');}</script> <tr><td><b>S</b></td><td><label id=\"lbl_but\"></label></td></tr></table> <div class=\"ui-field-contain\"> <script>p('r');p('g');p('b');</script> <button data-theme=\"b\" id=\"btn_sub\">Change Color</button> </div></div></div><script>var devip=\".\"; $(\"#btn_sub\").click(function(e){var c=devip+\"/cc?\"; c+=\"r=\"+$('#r').val(); c+=\"&g=\"+$('#g').val(); c+=\"&b=\"+$('#b').val(); $.getJSON(c, function(jd){});}); $(document).ready(function(){setTimeout(\"s()\", 2000);}); function s(){$('#btn_sub').button('disable'); $.getJSON(devip+\"/ja\", function(jd){var i; for(i=0;i<8;i++){$('#lbl_a'+i).text(jd.a[i]);}$('#lbl_but').text(jd.b?\"pressed\":\"-\"); $('#btn_sub').button('enable');}); setTimeout(\"s()\", 5000);}</script> </body></html>";

void serve_homepage(int ch_id) {
  String html="HTTP/1.0 200 OK\r\n\r\n";
  send_data(html, ch_id);

  unsigned long nbytes = sizeof(homepage)-1;
  unsigned long offset = 0;
  dbg.println(nbytes);
  while(nbytes>=BUFFER_SIZE) {
    esp.print("AT+CIPSEND=");
    esp.print(ch_id);
    esp.print(",");
    esp.println(BUFFER_SIZE);
    dbg.println(BUFFER_SIZE);
    if(wait_for_esp_response(2000, "> ")) {
      memcpy_P(buffer, homepage+offset, BUFFER_SIZE);
      esp.write((byte *)buffer, BUFFER_SIZE);
      if(wait_for_esp_response(5000)) {
        //dbg.println("SEND ok");
      }
    }
    nbytes -= BUFFER_SIZE;
    offset += BUFFER_SIZE;
  }
    
  // last packet
  if(nbytes > 0) {
    esp.print("AT+CIPSEND=");
    esp.print(ch_id);
    esp.print(",");
    esp.println(nbytes);
    dbg.println(nbytes);
    if(wait_for_esp_response(2000, "> ")) {
      memcpy_P(buffer, homepage+offset, BUFFER_SIZE);
      esp.write((byte *)buffer, nbytes);
      if(wait_for_esp_response(5000)) dbg.println("SEND ok");
    }      
    nbytes = 0;  
  }
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
  //delay(3000);
  
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

