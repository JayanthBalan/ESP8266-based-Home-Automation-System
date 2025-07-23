#include <WiFi.h>
#include "DHT.h"

const char* ssid = "ESP32_ACCESS_POINT";
const char* password = "pass";

const int proxsense = 21;
const int darksense = 25;
const int tempsense = 26;

const int light = 22;
const int fan = 23;

const int templim1 = 29;
const int templim2 = 27;
int proxcheck = 0;
bool userin = false;

WiFiServer server(80);
DHT tempdht(tempsense, DHT11);

String header;
String mode = "auto";
String lightstate = "off";
String fanstate = "off";

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 10000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(light, OUTPUT);
  digitalWrite(light, LOW);
  pinMode(fan, OUTPUT);
  digitalWrite(fan, LOW);

  pinMode(proxsense, INPUT);
  pinMode(darksense, INPUT);
  pinMode(tempsense, INPUT);

  Serial.println("Setting up access point...");
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (IP) {
    Serial.println("Access Point created successfully.");
  }
  else {
    Serial.println("Failed to create Access Point.");
  }

  tempdht.begin();
  server.begin();
}


void loop() {
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("...New Client Connected...");
    String currentLine = "";

    while (client.connected() && (currentTime - previousTime) <= timeoutTime) {

      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            sendHTML(client);

            if (mode == "manual") {
              if (header.indexOf("GET /light/on") >= 0) {
                Serial.println("Light ON");
                lightstate = "on";
                digitalWrite(light, HIGH);
                Serial.println("Light turned ON");
              }
              else if (header.indexOf("GET /light/off") >= 0) {
                Serial.println("Light OFF");
                lightstate = "off";
                digitalWrite(light, LOW);
                Serial.println("Light turned OFF");
              }

              if (header.indexOf("GET /fan/on") >= 0) {
                Serial.println("Fan ON");
                fanstate = "on";
                digitalWrite(fan, HIGH);
                Serial.println("Fan turned ON");
              }
              else if (header.indexOf("GET /fan/off") >= 0) {
                Serial.println("Fan OFF");
                fanstate = "off";
                digitalWrite(fan, LOW);
                Serial.println("Fan turned OFF");
              }
            }

            if (header.indexOf("GET /mode/manual") >= 0) {
              Serial.println("Mode MANUAL");
              mode = "manual";
              Serial.println("Mode set to MANUAL");
            }
            else if (header.indexOf("GET /mode/auto") >= 0) {
              Serial.println("Mode AUTO");
              mode = "auto";
              Serial.println("Mode set to AUTO");
            }
            
            break;
          }
          else {
            currentLine = "";
          }
        }
        else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  if (mode == "auto") {
    automode();
  }
}

void sendHTML(WiFiClient client) {
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");

  client.println("<style>html { font-family: monospace; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: yellowgreen; border: none; color: white; padding: 16px 40px;");
  client.println("text-decoration: none; font-size: 32px; margin: 2px; cursor: pointer;}");
  client.println(".button2 {background-color: gray;}</style></head>");

  client.println("<body><h1>ESP32 Web Server</h1>");
  client.println("<p>Control Light and Fan State</p>");

  if (lightstate == "off") {
    client.println("<p><a href=\"/light/on\"><button class=\"button\">ON</button></a></p>");
  }
  else {
    client.println("<p><a href=\"/light/off\"><button class=\"button button2\">OFF</button></a></p>");
  }
  
  if (fanstate == "off") {
    client.println("<p><a href=\"/fan/on\"><button class=\"button\">ON</button></a></p>");
  }
  else {
    client.println("<p><a href=\"/fan/off\"><button class=\"button button2\">OFF</button></a></p>");
  }
  
  if (mode == "auto") {
    client.println("<p><a href=\"/mode/manual\"><button class=\"button button2\">AUTO</button></a></p>");
  }
  else {
    client.println("<p><a href=\"/mode/auto\"><button class=\"button\">MANUAL</button></a></p>");
  }

  client.println("</body></html>");

  client.println();
}

void automode() {
  delay(2000);
  proxcheck = digitalRead(proxsense);
  if (proxcheck == 0) {
    proxcheck = 1;
    userin = !userin;

    if (userin == true) {
      delay(2000);
      float temp = tempdht.readTemperature();
      int dark = digitalRead(darksense);

      if (temp >= templim1) {
        digitalWrite(fan, HIGH);
        fanstate = "on";
      }
      else if (temp <= templim2) {
        digitalWrite(fan, LOW);
        fanstate = "off";
      }

      if (dark == 0) {
        digitalWrite(light, HIGH);
        lightstate = "on";
      }
      else {
        digitalWrite(light, LOW);
        lightstate = "off";
      }
    }
    else {
      digitalWrite(light, LOW);
      digitalWrite(fan, LOW);
    }
  }
  delay(2000);
}
