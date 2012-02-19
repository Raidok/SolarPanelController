/*
  Control Arduino outputs asynchronously with checkboxes
  by Raidok
*/



#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

// set these as necessary
byte mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
IPAddress ip(192, 168, 1, 2);

// all pages located at the root and on port 80
WebServer webserver("", 80);

int pin7 = 7;
boolean isPin7 = false;
int pin8 = 8;
boolean isPin8 = false;



// HTML

P(header) = "<!DOCTYPE html><html><head><title>Arduino</title>"
  "<script type=\"text/javascript\" src=\"http://code.jquery.com/jquery-latest.min.js\"></script>"
  "<script language=\"javascript\">"
  "$(document).ready(function() {"
    "$('input[type=checkbox]').click(function(src) {"
      "var data = {}; data[src.target.name] = src.target.checked;"
      "$.post(\"set\", data);"
    "});"
  "});</script></head><body>";
P(footer) = "</body></html>";
P(check1) = "<input type=\"checkbox\" name=\"";
P(check2) = "\"/>";
P(br) = "<br/>";



// PAGES

void helloCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();
  if (type != WebServer::HEAD) {
    P(hello) = "Hello! Click these to toggle pins!";
    server.printP(header);
    
    server.printP(hello);
    server.printP(br);
    
    server.printP(check1);
    server.print("pin7");
    server.printP(check2);
    server.print("pin7");
    server.printP(br);
    
    server.printP(check1);
    server.print("pin8");
    server.printP(check2);
    server.print("pin8");
    
    server.printP(footer);
  }
}

  
void setCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();
  if (type != WebServer::HEAD && type == WebServer::POST) {
    char name[10], value[10];
    int name_len, value_len;
    while (server.readPOSTparam(name, 10, value, 10)) {
      // cast char* to String for easier comparsion
      String nameStr = (String) name;
      String valueStr = (String) value;
      if (nameStr == "pin7") {
          isPin7 = parseBoolean(valueStr);
      } else if (nameStr == "pin8") {
          isPin8 = parseBoolean(valueStr);
      }
    }
  }
}



boolean parseBoolean(String value) {
  return value == "true";
}



// SETUP

void setup() {
  // initialize network connection
  Ethernet.begin(mac, ip);
  // default page
  webserver.setDefaultCommand(&helloCmd);
  // available pages
  webserver.addCommand("hello", &helloCmd);
  webserver.addCommand("set", &setCmd);
  // start server
  webserver.begin();
  // set pin modes
  pinMode(pin7, OUTPUT);
  pinMode(pin8, OUTPUT);
}



// LOOP

void loop() {
  // processing connections
  webserver.processConnection();
  
  // do other stuff
  digitalWrite(pin7, isPin7 ? HIGH : LOW);
  digitalWrite(pin8, isPin8 ? HIGH : LOW);
}
