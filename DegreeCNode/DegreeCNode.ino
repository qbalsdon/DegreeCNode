#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <FirebaseArduino.h>

#include "constants.h"

#define DS18S20_Pin D3
#define BLUE_PIN D6
#define GREEN_PIN D5
#define RED_PIN D4

#define COLOUR_COUNT 10
#define INT_MAX 214748364
#define MAIN_DELAY 10
#define T_COUNT 144
#define IGNORE_TEMP 125
#define COUNT_MAX 10000 //5 minutes (millis)

String colours[COLOUR_COUNT] = { "e6194b", "3cb44b", "ffe119", "0082c8", "f58231", "911eb4", "46f0f0", "d2f53c", "fabebe", "e6beff" };
String colour = "";
long count;
double average;
String macAddr;
boolean registered = false;
int countLoop = 0;
int blinkCount = 0;
int blinkOn = false;
float temperatures[T_COUNT];
int temperatureIndex = 0;

ESP8266WebServer server(80);
OneWire ds(DS18S20_Pin);
HTTPClient http;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Hello ESP8266");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setColour("aa0000");
  String ipAddress = setupServer();
  setColour("00aa00");
  unsigned char mac[6];
  WiFi.macAddress(mac);  
  macAddr = macToStr(mac);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.setString(FIREBASE_IP, WiFi.localIP().toString());
  setColour("0000aa");
  count = 0;
  average = 0;
  setColour("000000");
  digitalWrite(LED_BUILTIN, LOW);

  for (int i = 0; i < T_COUNT; i++){
    temperatures[i] = IGNORE_TEMP;
  }

  /*StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& data = root.createNestedArray("readings");
  for (int i = 0; i < 10; i++){
    data.add(float_with_n_digits(100.0 / (i + 1), 2));
  }
  Firebase.set(FIREBASE_VALUE, data);*/
}

String setupServer() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("");

  // Wait for connection
  boolean high = true;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    digitalWrite(LED_BUILTIN, high);
    high = !high;
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/reset", handleReset);
  server.on("/average", handleAverage);
  server.on("/temperature", handleTemperature);
  server.on("/blink", handleBlink);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  return WiFi.localIP().toString();
}



String getInterpolatedColour(float difference) {
  if (difference < 0) {
    difference = 0;
  }
  if (difference > 100) {
    difference = 100;
  }
  //BLUE - COLD!
  int r1 = 18;
  int g1 = 228;
  int b1 = 232;

  //RED - HOT!
  int r2 = 232;
  int g2 = 18;
  int b2 = 68;

  int rValue = r1 + ((r2 - r1) * (difference / 100.0));
  int gValue = g1 + ((g2 - g1) * (difference / 100.0));
  int bValue = b1 + ((b2 - b1) * (difference / 100.0));

  return String(rValue, HEX) + String(gValue, HEX) + String(bValue, HEX);
}

void handleRoot() {
  String labels = "";
  String data = "";

  int count = 0;
  int t_index = temperatureIndex;
  String delim = "";
  while (temperatures[t_index] != IGNORE_TEMP) {
    labels += delim;
    labels += count++;
    data += delim;
    data += temperatures[t_index];

    delim = ",";
    t_index--;
    if (t_index < 0) {
      t_index = T_COUNT - 1;
    }
    if (t_index == temperatureIndex) break;
  }

  if (data == "") {
    data = "0";
    labels = "0";
  }
  
  String result="<!doctype html><html><head><title>Camlin Temperature Network</title><script src=\"http://www.chartjs.org/dist/2.7.1/Chart.bundle.js\"></script><script src=\"http://www.chartjs.org/samples/latest/utils.js\"></script><script language=\"javascript\">const COLOURS = [\"#e6194b\", \"#3cb44b\", \"#ffe119\", \"#0082c8\", \"#f58231\", \"#911eb4\", \"#46f0f0\", \"#d2f53c\", \"#fabebe\", \"#e6beff\"];var flip = true;function getUrl() {var url = window.location.href;var arr = url.split(\"/\");return arr[0] + \"//\" + arr[2];}function blink() {var xhttpBlink = new XMLHttpRequest();xhttpBlink.open(\"GET\", \"http://\" + getUrl() + \"/blink\", true);xhttpBlink.send();var element = document.getElementById(\"img_\"+mac);var colour = element.style.background;for (var i = 0; i < 34; i++) {var time = i;setTimeout(function() {element.style.backgroundColor = (flip) ? \"transparent\" : colour;flip = !flip;}, time * 250);}}function updateChart() {var config = {type: 'line',data: {labels: ["+labels+"],datasets: [{label: \"Temperature Data\",backgroundColor: COLOURS[0],borderColor: COLOURS[0],data: ["+data+"],fill: false,}]},options: {responsive: true,title:{display:true,text: \"Temperature Data\"},tooltips: {mode: 'index',intersect: false,},hover: {mode: 'nearest',intersect: true},scales: {xAxes: [{display: true,scaleLabel: {display: true,labelString: 'Time'}}],yAxes: [{display: true,scaleLabel: {display: true,labelString: 'Temperature ºc'}}]}}};var ctx = document.getElementById(\"canvas\").getContext(\"2d\");if (window.myLine != null) window.myLine.destroy();window.myLine = new Chart(ctx, config);}function reloadPage() {window.location.reload();}</script><style>canvas{-moz-user-select: none;-webkit-user-select: none;-ms-user-select: none;}</style></head><body><div style=\"width:75%;float:left;\"><canvas id=\"canvas\"></canvas></div><div style=\"width:25%;float:left;\" id=\"control\"></div><br><br><script>window.onload = function() {updateChart();};</script></body></html>";
  server.send(200, "text/html", result);
}

void handleReset() {
  average = 0;
  count = 0;
  server.send(200, "text/html", "Success");
}

void handleAverage() {
  server.send(200, "text/html", "{\"average\": " + String(average) + ", \"mac\": \"" + macAddr + "\"}");
}

void handleTemperature() {
  server.send(200, "text/html", "{\"temperature\": " + String(average) + "}");
}

void handleBlink() {  
  blinkCount = 2000;
  server.send(200, "text/html", "Blinking!");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setColour(int red, int green, int blue) {
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);  
}

int hexToInt(String hex) {
  int value = 0;
  int a = 0;
  int b = hex.length() - 1;
  for (; b >= 0; a++, b--) {
    if (hex[b] >= '0' && hex[b] <= '9') {
      value += (hex[b] - '0') * (1 << (a * 4));
      } else {
        value += ((hex[b] - 'A') + 10) * (1 << (a * 4));
      }
    }
    return value;
}

void setColour(String hex) {
  hex.toUpperCase();
  String redString = hex.substring(0, 2);
  String greenString = hex.substring(2, 4);
  String blueString = hex.substring(4, 6);
      
  setColour(hexToInt(redString), hexToInt(greenString), hexToInt(blueString));
}

float getTemperature() {
 //returns the temperature in Celsius

 byte data[12];
 byte addr[8];

 if ( !ds.search(addr)) {
   //no more sensors on chain, reset search
   ds.reset_search();
   return -1001;
 }

 if ( OneWire::crc8( addr, 7) != addr[7]) {
   Serial.println("CRC is not valid!");
   return -1002;
 }

 if ( addr[0] != 0x10 && addr[0] != 0x28) {
   Serial.print("Device is not recognized");
   return -1003;
 }

 ds.reset();
 ds.select(addr);
 ds.write(0x44,1); // start conversion, with parasite power on at the end

 byte present = ds.reset();
 ds.select(addr);  
 ds.write(0xBE); // Read Scratchpad

 
 for (int i = 0; i < 9; i++) { // we need 9 bytes
  data[i] = ds.read();
 }
 
 ds.reset_search();
 
 byte MSB = data[1];
 byte LSB = data[0];

 float tempRead = ((MSB << 8) | LSB); //using two's compliment
 float TemperatureSum = tempRead / 16;
 
 return TemperatureSum;
}

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void updateAverage(float temperature) {
  if (temperature < -125 || temperature > 125) return;
  count = (count % INT_MAX) + 1;
  average -= average / count;
  average += temperature / count;
}

void updateTemperatureArray() {
  int t_index = temperatureIndex;
  temperatures[temperatureIndex] = average;
  average = 0;
  count = 0;
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& data = root.createNestedArray("readings");
   
  while (temperatures[t_index] != IGNORE_TEMP) {
    data.add(float_with_n_digits(temperatures[t_index], 2));
    
    t_index--;
    if (t_index < 0) {
      t_index = T_COUNT - 1;
    }
    if (t_index == temperatureIndex) break;
  }
  
  Firebase.set(FIREBASE_VALUE, data);
  if (Firebase.failed()) {
    setColour("aa0000");
    Serial.print("setting array failed:");
    Serial.println(Firebase.error());
  }
  temperatureIndex++;
  if (temperatureIndex == T_COUNT) {
    temperatureIndex = 0;
  }
}

void loop() {
  server.handleClient();
  updateAverage(getTemperature());

  /*if (blinkCount > 0) {
    blinkCount = blinkCount - 10;
    if (blinkCount % 50 == 0) {
      setColour(colour);
    } else {
      setColour("000000");
    }
  } else {
    setColour(colour);
  }*/

  countLoop ++;
 
  if (countLoop >= (COUNT_MAX / MAIN_DELAY)) {
    updateTemperatureArray();
    countLoop = 0;
  }
  delay(MAIN_DELAY); 
}

