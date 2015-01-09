/* ====== ESP8266 Toy Demo ============
 * This is the default demo for ESPToy:
 * - starts up in AP mode
 * - serve HTML page from flash memory
 * - read and display light/temperature values
 * - allows you to set RGB led color
 * ====================================
 *
 * After power-on, wait for 5-10 seconds
 * for the green LED to blink once.
 * At this point the module will create
 * a local AP named ESP_xxxxx. Use your
 * cell phone or computer to connect
 * to this AP, then open a browser and
 * type in http://192.168.4.1. You should
 * see the homepage then.
 * 
 * Written by Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=10156
 */

#define PORT  "80"           // using port 80 by default

#define PIN_BUTTON  3
#define PIN_RED     12
#define PIN_GREEN   13
#define PIN_BLUE    14
#define PIN_LIGHT   0
#define PIN_TEMP    1

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

const unsigned long baudrate[] = {9600, 115200, 57600, 19200, 19200, 38400};

void setup() {

  // search ESP baud rate
  dbg.begin(DBG_BAUD_RATE);
  dbg.print("Search ESP baud rate: ");
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
    esp.begin(9600);  // assume 9600 as default
  }
  
  send_at("AT+CIOBAUD=115200");
  esp.begin(115200);
  
  // setup GPIO pins
  digitalWrite(PIN_BUTTON, HIGH);
  pinMode(PIN_BUTTON, OUTPUT);
  
  digitalWrite(PIN_RED, LOW);
  pinMode(PIN_RED, OUTPUT);
  
  digitalWrite(PIN_GREEN, LOW);
  pinMode(PIN_GREEN, OUTPUT);

  digitalWrite(PIN_BLUE, LOW);
  pinMode(PIN_BLUE, OUTPUT);

  setupAP();

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
        } else {
          dbg.println(F("-> serve ja"));
          wait_for_esp_response();
          serve_ja(ch_id);          
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
  html+="{\"tempc\":";
  html+=get_tempc(analogRead(PIN_TEMP));
  html+=",\"lit\":";
  html+=analogRead(PIN_LIGHT);
  html+=",\"but\":";
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

prog_char homepage[] PROGMEM = "<!DOCTYPE html><html> <head> <title>ESPToy Demo</title> <meta name='viewport' content='width=device-width, initial-scale=1'> </head> <body> <div> <h3>ESPToy Demo</h3> <hr> <div> <table cellpadding=2> <script>function w(s){document.write(s);}function id(s){return document.getElementById(s);}</script> <tr><td><b>T(&deg;C)</b>:</td><td><label id='lbl_tc'></label></td></tr><tr><td><b>T(&deg;F)</b>:</td><td><label id='lbl_tf'></label></td></tr><tr><td><b>Light</b>:</td><td><label id='lbl_lit'></label></td></tr><tr><td><b>Button</b>:</td><td><label id='lbl_but'></label></td></tr><tr><td><b>Red</b>:</td><td><input type='range' id='s_r' value=0 min=0 max=255></input></td></tr><tr><td><b>Green</b>:</td><td><input type='range' id='s_g' value=0 min=0 max=255></input></td></tr><tr><td><b>Blue</b>:</td><td><input type='range' id='s_b' value=0 min=0 max=255></input></td></tr></table> <div> <button id='btn_ref' onclick='loadValues()'>Refresh Values</button> <button id='btn_sub' onclick='setColor()'>Set Color</button> </div></div></div>"
"<script>var devip='http://192.168.4.1'; function loadValues(){var xmlhttp; if(window.XMLHttpRequest) xmlhttp=new XMLHttpRequest(); else xmlhttp=new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.onreadystatechange=function(){if(xmlhttp.readyState==4 && xmlhttp.status==200){var jd=JSON.parse(xmlhttp.responseText); id('lbl_tc').innerHTML=jd.tempc/10; id('lbl_tf').innerHTML=((jd.tempc/10)*1.8+32)>>0; id('lbl_lit').innerHTML=jd.lit; id('lbl_but').innerHTML=(jd.but)?'pressed':'released'; id('s_r').value=jd.red; id('s_g').value=jd.green; id('s_b').value=jd.blue;}}; xmlhttp.open('GET',devip+'/ja',true); xmlhttp.send();}function setColor(){var xmlhttp; if(window.XMLHttpRequest) xmlhttp=new XMLHttpRequest(); else xmlhttp=new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.open('GET',devip+'/cc?'+'r='+id('s_r').value+'g='+id('s_g').value+'b='+id('s_b').value,true); xmlhttp.send();}setTimeout(loadValues, 500); </script> </body></html>";

void serve_homepage(int ch_id) {
  String html="HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nCache-Control: max-age=604800, public\r\n\r\n";
  send_data(html, ch_id);

  unsigned long nbytes = sizeof(homepage)-1;
  unsigned long offset = 0;
  while(nbytes>=BUFFER_SIZE) {
    esp.print("AT+CIPSEND=");
    esp.print(ch_id);
    esp.print(",");
    esp.println(BUFFER_SIZE);
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

void setupAP() {
  // query esp
  send_at("AT");

  // set mode 3 (AP+station)
  send_at("AT+CWMODE=3");

  // reset WiFi module
  send_at("AT+RST");
  delay(3000);
  
  // join AP
  // start server
  send_at("AT+CIPMUX=1");
  
  String comm="AT+CIPSERVER=1,"; // turn on TCP service
  comm+=PORT;
  send_at(comm);
  
  digitalWrite(PIN_GREEN, HIGH);
  delay(500);
  digitalWrite(PIN_GREEN, LOW);
}

const float temps[] = {
// according to TDR datasheet: http://www.360yuanjian.com/product/downloadFile-14073-proudct_pdf_doc-pdf.html
// this table defines the reference resistance of the thermistor under each temperature value from -20 to 40 Celcius
940.885,  // -20 Celcius
888.148,  // -19 Celcius
838.764,
792.494,
749.115,
708.422,
670.228,
634.359,
600.657,
568.973,
539.171,
511.127,
484.723,
459.851,
436.413,
414.316,
393.473,
373.806,
355.239,
337.705,
321.140,
305.482,
290.679,
276.676,
263.427,
250.886,
239.012,
227.005,
217.106,
207.005,
198.530,
188.343,
179.724,
171.545,
163.780,
156.407,
149.403,
142.748,
136.423,
130.410,
124.692,
119.253,
114.078,
109.152,
104.464,
100.000,
95.747,
91.697,
87.837,
84.157,
80.650,
77.305,
74.115,
71.072,
68.167,
65.395,
62.749,
60.222,
57.809,
55.503,
53.300    // +40 Celcius
};

// Get temperature (in Celcius)
long get_tempc(int av){
  dbg.println(av);
  float resistance = (1024.0/av-1.0)*100;
  // search in TDR table for the closest resistance
  // then do linear interpolation
  for(int i = 1; i < (sizeof(temps)/sizeof(long)); i++){
    if(resistance >= temps[i]){
      float temp = 10.0*(i-20);
      temp -= 10.0*(resistance-temps[i])/(temps[i-1]-temps[i]);
      return (long)temp;
    }
  }
  return -40;
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


