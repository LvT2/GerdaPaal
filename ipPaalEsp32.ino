#include "secrets.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8
#define CS_PIN D8

MD_Parola dp = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// ===========================================================| Configuration

const char* ssid = WIFI_SSID;                 // wifi name
const char* wifiPassword = WIFI_PASSWORD;     // wifi password

bool httpPasswordEnabled = true;              // true | false, whether a password is enabled for a http POST request.
const char* httpPassword = HTTP_PASSWORD;     // POST request must start with the specified password.
const char* mdnsHostname = "gerdapaal";       // the .local extension gets appended automatically.

// ===========================================================| end Configuration
// ===========================================================| Global vars
ESP8266WebServer server(80);

String parsedNewlines[10];
bool cycleNewlines = false;
int currentAnimatingNewline = 0; // change to another data type?
int receivedNewlineCount = 0;

// ===========================================================| end global vars
// ===========================================================| Functions

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }
  server.send(404, "text/plain", message);
}

void nextNewline() {
  if (currentAnimatingNewline >= receivedNewlineCount) {
    currentAnimatingNewline = 0;
  }

  if (currentAnimatingNewline == 0) {
    dp.displayText(parsedNewlines[currentAnimatingNewline].c_str(), PA_CENTER, 30, 1000, PA_OPENING_CURSOR, PA_SCROLL_UP);

  } else if (currentAnimatingNewline >= receivedNewlineCount - 1) {
    dp.displayText(parsedNewlines[currentAnimatingNewline].c_str(), PA_CENTER, 30, 1000, PA_SCROLL_UP, PA_CLOSING_CURSOR);

  } else {
    dp.displayText(parsedNewlines[currentAnimatingNewline].c_str(), PA_CENTER, 30, 1000, PA_SCROLL_UP, PA_SCROLL_UP);

  }
  currentAnimatingNewline++;
}

void handleNewline(String newlineMess) {
  
  Serial.print("Message: [");
  Serial.print(newlineMess);
  Serial.println("]");

  Serial.print("Length: ");
  Serial.println(newlineMess.length());

  // add a warning if more than 10 newlines are sent in?
  receivedNewlineCount = 0;
  
  int start = 0;

  currentAnimatingNewline = 0;

  while (receivedNewlineCount < 10) {
    int newline = newlineMess.indexOf('\n', start);

    if (newline == -1) {
      // Last line
      parsedNewlines[receivedNewlineCount] = newlineMess.substring(start);
      Serial.println(receivedNewlineCount);
      Serial.println(parsedNewlines[receivedNewlineCount]);
      receivedNewlineCount++;
      cycleNewlines = true;
      break;
    }

    parsedNewlines[receivedNewlineCount] = newlineMess.substring(start, newline); // WAT zijn enumerations?
    Serial.println(receivedNewlineCount);
    Serial.println(parsedNewlines[receivedNewlineCount]);
    receivedNewlineCount++;

    start = newline + 1;
  }
}

void handlePost() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Jij gerda, gebruik /input!!!!!, only POST is Allowed"); // 405 bad method, dus POST of GET.  

  } else {

    static String message;
    message = server.arg("plain");

    if (httpPasswordEnabled) {
      if (server.header("authorization") != httpPassword) {
        server.send(401, "text/plain", "pass incorrect, you POSTed: " + message);
        return;
      }
    }

    cycleNewlines = false;

    if (server.hasArg("type")) {
      String type = server.arg("type");

      if (type == "newline") {
        handleNewline(message);
      } else {
        server.send(400, "text/plain", "you posted a unknown type.");
      }

    } else {
    dp.displayClear();
    dp.displayScroll(message.c_str(), PA_CENTER, PA_SCROLL_LEFT, 25);
    }
    server.send(200, "text/plain", "pass correct, you POSTed: " + message);
  }
}

// ===========================================================| end Functions
// ===========================================================| Setup
void setup(void) {
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  dp.begin();
  dp.displayClear();

  static  String ipAddr = "IP address: " + WiFi.localIP().toString();
  dp.displayText(ipAddr.c_str(), PA_CENTER, 40, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  if (MDNS.begin(mdnsHostname)) { Serial.println("MDNS responder started"); }

  server.onNotFound(handleNotFound);

  server.on("/input", handlePost);

  server.collectHeaders("authorization");

  server.begin();
  Serial.println("HTTP server started");
}

// ===========================================================| end Setup
// ===========================================================| Loop

void loop(void) {
  server.handleClient();
  MDNS.update();

  if (cycleNewlines) {
    nextNewline();
  }

  if (dp.displayAnimate()) {
    dp.displayReset();
  }
}