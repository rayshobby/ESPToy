/* ========= ESP8266 Demo ==========
 * Scan and list nearby WiFi Network
 * =================================
 *
 * The module will start in AP mode and
 * create a WiFi AP with ssid ESPxxxx.
 * First connect to this ssid (no security);
 * then type in IP address
 * 192.168.4.1
 * to list scanned nearby networks. Next
 * input the ssid and password of the
 * desired network, and click on 'Connect'.
 * The webpage will redirect in 10 seconds
 * and show the client IP address.
 *
 * Written by Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=10156
 */

/* ======================================
 * If you don't know ESP's baud rate,
 * use the SearchBaud example to find it.
 * ======================================
 */
#define ESP_BAUD_RATE 115200  // assume 115200 baud rate
#define PORT  "80"            // using port 80 by default

// Ethernet buffer
#define BUFFER_SIZE 1000
char buffer[BUFFER_SIZE];
boolean in_ap_mode = true;

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

#define PIN_GREEN   13

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
byte send_at(String comm, int timeout=2000) {
  esp.println(comm);
  dbg.println(comm);
  bool ret = wait_for_esp_response(timeout);
  return ret;
}

void setup() {

  pinMode(PIN_GREEN, OUTPUT);
  
  esp.begin(ESP_BAUD_RATE);
  
  dbg.begin(DBG_BAUD_RATE);
  dbg.println(F("Start in AP mode."));
  
  setupAP();
  
  // print AP IP address
  dbg.print(F("ip addr:"));
  send_at("AT+CIFSR");
  dbg.print(buffer);  
  
}

// read data from ESP8266 until \r\n is found
// non-blocking
bool read_till_eol() {
  static int i=0;
  if(esp.available()) {
    char c=esp.read();
    buffer[i++]=c;
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
        } else if (strncmp(pb, "GET /cw?", 8) == 0) {
          dbg.println(F("-> request to connect"));
          get_comm(pb+8, buffer+BUFFER_SIZE-64, 64);
          wait_for_esp_response();
          dbg.println(buffer+BUFFER_SIZE-64);
          server_connect(ch_id, buffer+BUFFER_SIZE-64);
        }
      }
    }
  }
}

// send data using the AT+CIPSEND command
// close the connection if close==true
bool send_data(String data, int ch_id, bool close=false) {
  bool success = false;
  esp.print(F("AT+CIPSEND="));
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
    esp.print(F("AT+CIPCLOSE="));
    esp.println(ch_id);
    wait_for_esp_response();
  }
  return success;
}

void serve_favicon(int ch_id) {
  String html = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n\r\n";
  send_data(html, ch_id, true);
}

void server_connect(int ch_id, char* comm) {
  String html = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n\r\n";
  char ssid[64];
  char password[64];
  if(!findKeyVal(comm, ssid, 64, "id")) {
    ssid[0]=0;
  }
  urlDecode(ssid);
  dbg.println(ssid);
  if(!findKeyVal(comm, password, 64, "pw")) {
    password[0]=0;
  }
  urlDecode(password);
  dbg.println(password);

  html+= "<head><meta http-equiv=\"refresh\" content=\"10;url=.\"></head>Connecting...";
  send_data(html, ch_id, true);

  setupClient(ssid, password);
}

char* find_next_ssid(char *p, char ssid[]) {

  // find the first number
  char c;
  while(1) {
    c = *p++;
    if(!c) {return 0;}
    if(c=='(') {
      break;
    }
  }
  // at this point, c should be the security number
  c=*p++;
  if(!(c>='0' && c<='4')) return 0;
  // at this point, c should be a comma
  c=*p++;
  if(c!=',') return 0;
  // at this point, c should be a quote
  c=*p++;
  if(c!='"') return 0;
  int i=0;
  while(1) {
    c = *p++;
    if(c=='"' || c==0) break;
    ssid[i++]=c;
  }
  ssid[i]=0;
  // scan till the end of line
  while(1) {
    c=*p++;
    if(!c) return 0;
    if(c=='\n') break;
  }
  return p;
}

char *find_client_ip(char *p) {
  char c;
  char *start;
  // skip two lines
  while(*p && *p!='\n') p++;  // first line: AT+CIFSR
  if(*p==0) return 0;
  p++;
  while(*p && *p!='\n') p++;  // second line: 192.168.4.1
  p++;
  if(*p==0) return 0;
  start=p;
  while(*p && *p!='\n') p++;  // third line: client ip
  *p=0;
  return start;  
}

void serve_homepage(int ch_id) {
  String html = "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n\r\n";
  if (in_ap_mode) {
    send_at("AT+CWLAP");
    wait_for_esp_response(8000);
    dbg.print(buffer);

    html += "<html><h3>Up to 8 nearest networks:</h3>";
    char ssid[64];
    char *p = buffer;
    int i=0;
    while(p && i<8) {
      p=find_next_ssid(p, ssid);
      if(!p) break;
      html += "<li>";
      html += String(ssid);
      i++;
      html += "</li><br>";      
    }
    html += "<form method=get action=cw><p>SSID: <input type='text' name='id' size='16'></p><p>Password: <input type='password' name='pw' size='16'></p><input type='submit' value='Connect'></form></html>";
    send_data(html, ch_id, true); 
  } else {
    send_at("AT+CIFSR");
    html += "<html>I am in client mode. My client IP is ";
    html += find_client_ip(buffer);
    html += "</html>";
    send_data(html, ch_id, true);
  }
  
}

void setupClient(char *ssid, char *password) {
  String comm="AT+CWJAP=\"";
  comm+=ssid;
  comm+="\",\"";
  comm+=password;
  comm+="\"";
  send_at(comm, 5000);

  // start server
  send_at("AT+CIPMUX=1");
  
  comm="AT+CIPSERVER=1,"; // turn on TCP service
  comm+=PORT;
  send_at(comm);  
  
  in_ap_mode = false;
  digitalWrite(PIN_GREEN, LOW);  
}

void setupAP() {
  // query esp
  send_at("AT");

  // set mode 2 (AP)
  send_at("AT+CWMODE=3");

  // reset WiFi module
  send_at("AT+RST");
  delay(3000);
 
  // start server
  send_at("AT+CIPMUX=1");
  
  String comm="AT+CIPSERVER=1,"; // turn on TCP service
  comm+=PORT;
  send_at(comm);
  
  in_ap_mode = true;
  digitalWrite(PIN_GREEN, HIGH);
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

void urlDecode (char *urlbuf)
{
    char c;
    char *dst = urlbuf;
    while ((c = *urlbuf) != 0) {
        if (c == '+') c = ' ';
        if (c == '%') {
            c = *++urlbuf;
            c = (h2int(c) << 4) | h2int(*++urlbuf);
        }
        *dst++ = c;
        urlbuf++;
    }
    *dst = '\0';
}

// convert a single hex digit character to its integer value
unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
