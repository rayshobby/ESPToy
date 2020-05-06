#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "html_home.h"
#include "pitches.h"

#define BUTTON  0
#define NEOP 13
#define BUZZ 15

#define HTML_OK                0x00
#define HTML_SUCCESS           0x01

extern const char html_home[] PROGMEM;

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, NEOP, NEO_GRB+NEO_KHZ800);

ESP8266WebServer server(80);
uint8_t red, green, blue;
uint8_t show_rainbow = 1;

void play_startup_tune() {
  static int melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
  static int duration[] = {8, 8, 8, 8};
  
  for (byte i = 0; i < sizeof(melody)/sizeof(int); i++) {
    uint delaytime = 1000/duration[i];
    tone(BUZZ, melody[i], delaytime);
    delay(delaytime * 1.2);
    noTone(BUZZ);
  }
}

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

bool get_value_by_key(const char* key, uint8_t& val) {
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

void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void led_set_color(uint32_t c) {
	pixel.setPixelColor(0, c);
	pixel.show();
}

void on_change_color() {
  get_value_by_key("r", red);
  get_value_by_key("g", green);
  get_value_by_key("b", blue);
	show_rainbow = 0;
  led_set_color(red, green, blue);
	tone(BUZZ, 440, 50);
	delay(60);
	noTone(BUZZ);
  server_send_result(HTML_SUCCESS);
}

void setup()
{
  pixel.begin();
	pixel.clear();
  pixel.setBrightness(64);

  digitalWrite(BUZZ, LOW);
  pinMode(BUZZ, OUTPUT);
  play_startup_tune();

  Serial.begin(115200);
  
  pinMode(BUTTON, INPUT);  

  red = green = blue = 0;
  
  String ap_ssid = get_ap_ssid();
  start_network_ap(ap_ssid.c_str(), NULL);
  server.on("/",   on_home);  
  server.on("/ja", on_json);
  server.on("/cc", on_change_color);  
  server.begin();
  Serial.println(F("server started"));
}

void loop(){
  server.handleClient();
  
  if(digitalRead(BUTTON)==LOW) {
  	show_rainbow = !show_rainbow;
  	if(!show_rainbow) led_set_color(0);
  	tone(BUZZ, 200, 50);
  	delay(250);
  	noTone(BUZZ);
  }
  
	if(show_rainbow) demo_rainbow();  
}

uint32_t rainbow_timeout = 0;
uint32_t rainbow_hue = 0;
void demo_rainbow() {
	if(millis() > rainbow_timeout) {
		led_set_color(pixel.gamma32(pixel.ColorHSV(rainbow_hue)));
		rainbow_hue += 64;
		rainbow_timeout = millis() + 3;
  }
}
