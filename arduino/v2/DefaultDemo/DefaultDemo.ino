#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "html_home.h"

#define BUTTON  0
#define LED_R   4
#define LED_G   2
#define LED_B   5

#define HTML_OK                0x00
#define HTML_SUCCESS           0x01

extern const char html_home[] PROGMEM;

ESP8266WebServer server(80);
uint red, green, blue;

void start_network_ap(const char *ssid, const char *pass) {
  if(!ssid) return;
  Serial.println(F("AP mode"));
  if(pass)
    WiFi.softAP(ssid, pass);
  else
    WiFi.softAP(ssid);
  WiFi.mode(WIFI_AP); // start in AP_STA mode
  WiFi.disconnect();  // disconnect from router
}

char dec2hexchar(byte dec) {
  if(dec<10) return '0'+dec;
  else return 'A'+(dec-10);
}

String get_ap_ssid() {
  static String ap_ssid = "";
  if(!ap_ssid.length()) {
    byte mac[6];
    WiFi.macAddress(mac);
    ap_ssid = "ESPTOY_";
    for(byte i=3;i<6;i++) {
      ap_ssid += dec2hexchar((mac[i]>>4)&0x0F);
      ap_ssid += dec2hexchar(mac[i]&0x0F);
    }
  }
  return ap_ssid;
}

void server_send_html(String html) {
  server.send(200, "text/html", html);
}

void server_send_result(byte code, const char* item = NULL) {
  String html = F("{\"result\":");
  html += code;
  if (!item) item = "";
  html += F(",\"item\":\"");
  html += item;
  html += F("\"");
  html += F("}");
  server_send_html(html);
}

bool get_value_by_key(const char* key, uint& val) {
  if(server.hasArg(key)) {
    val = server.arg(key).toInt();   
    return true;
  } else {
    return false;
  }
}

void on_home() {
  Serial.println(F("request homepage"));
  String html = "";
  html += FPSTR(html_home);
  server_send_html(html);
}

void on_json() {
  String html = "";
  html += F("{\"adc\":");
  html += analogRead(A0);
  html += F(",\"but\":");  
  html += (1-digitalRead(BUTTON));
  html += F(",\"red\":");
  html += red;
  html += F(",\"green\":");
  html += green;
  html += F(",\"blue\":");
  html += blue;
  html += F("}");
  server_send_html(html);  
}

void led_set_color(uint r, uint g, uint b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

void on_change_color() {
  get_value_by_key("r", red);
  get_value_by_key("g", green);
  get_value_by_key("b", blue);
  led_set_color(red, green, blue);
  server_send_result(HTML_SUCCESS);
}

void setup()
{
  Serial.begin(115200);
  WiFi.persistent(false); // turn off persistent, fixing flash crashing issue
  
  pinMode(BUTTON, INPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);    
  
  led_set_color(255, 255, 255);
  delay(500);
  led_set_color(0, 0, 0);
 
  red = green = blue = 0;
  
  String ap_ssid = get_ap_ssid();
  start_network_ap(ap_ssid.c_str(), NULL);
  server.on("/",   on_home);  
  server.on("/ja", on_json);
  server.on("/cc", on_change_color);  
  server.begin();
  Serial.println(F("server started"));
}

void loop(void){
  server.handleClient();
}
