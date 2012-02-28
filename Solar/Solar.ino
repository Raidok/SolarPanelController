/*
  Solar panel 
*/


// time
#include <Time.h>

// network
#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0x03 };
IPAddress ip(192, 168, 1, 2);
WebServer webserver("", 80);

byte ledPin = 8;
boolean isLedOn = false;

// HTML

P(header) = "<!DOCTYPE html><html><head><title>Solar</title>"
  "<script type=\"text/javascript\" src=\"http://code.jquery.com/jquery-latest.min.js\"></script>"
  "<script language=\"javascript\">"
  "$(document).ready(function() {"
    "$('input[type=checkbox]').click(function(src) {"
      "var data = {}; data[src.target.name] = src.target.checked;"
      "$.post(\"set\", data, function(data) { /*alert(data);*/ });"
    "});"
  "});</script></head><body>";
P(footer) = "</body></html>";
P(timeDiv) = "<div style=\"position:absolute;right:0;top:0;\">";
P(closeDiv) = "</div>";
P(trS) = "<tr>";
P(trC) = "</tr>";
P(tdS) = "<td>";
P(tdC) = "</td>";
P(textInput_name) = "<input type=\"text\" name=\"";
P(textInput_value) = "\" size=\"2\" value=\"";
P(textInput_close) = "\"/>";
// settings
P(setTimeLink) = "<a href=\"setTime\">Muuda kellaaega/kuupäeva</a><br/>";
P(setLedLink) = "<a href=\"setLeds\">Lülita tulesid</a><br/>";



// PAGES

boolean defaultTemplate(WebServer &server, WebServer::ConnectionType type) {
  server.httpSuccess();
  server.printP(header);
  return type != WebServer::HEAD;
}

void defaultEnd(WebServer &server) {
  server.printP(timeDiv);
  server.print(getTimeString());
  server.printP(closeDiv);
  server.printP(footer);
}

void helloCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  if (defaultTemplate(server, type)) {
    server.print("Hello");
    defaultEnd(server);
  }
}

void settingsCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  if (defaultTemplate(server, type)) {
    server.printP(setTimeLink);
    server.printP(setLedLink);
    defaultEnd(server);
  }
}

void setTimeCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  if (defaultTemplate(server, type)) {
    
    if (type == WebServer::POST) {
      char name[5], value[5];
      int name_len, value_len;
      int d, m, y, H, M, S;
      while (server.readPOSTparam(name, 5, value, 5)) {
        switch(*name) {
          case 'd':
            d = ((String)value).toInt();
            break;
          case 'm':
            m = ((String)value).toInt();
            break;
          case 'y':
            y = ((String)value).toInt();
            break;
          case 'H':
            H = ((String)value).toInt();
            break;
          case 'M':
            M = ((String)value).toInt();
            break;
          case 'S':
            S = ((String)value).toInt();
            break;
          default:
            server.print("unknown ");
            server.print(name);
            server.print(":");
            server.println(value);
        }
      }
      if (y) setTime(H,M,S,d,m,y);

    } else {
      
      P(form_start) = "<form method=\"post\">"
      "<table><tr><td>Päev</td><td>Kuu</td><td>Aasta</td><td>&nbsp;</td><td>Tunnid</td><td>Minutid</td><td>Sekundid</td></tr><tr><td>";
      P(form_end) = "</td></tr></table><input type=\"submit\" value=\"Save\"/></form>";
      P(separator) = "\"/></td><td>";
      
      time_t t = now(); 
      
      server.printP(form_start);
      
      // day
      server.printP(textInput_name);
      server.print("d");
      server.printP(textInput_value);
      server.print(day(t));
      server.printP(separator);
      
      // month
      server.printP(textInput_name);
      server.print("m");
      server.printP(textInput_value);
      server.print(month(t));
      server.printP(separator);
      
      // year
      server.printP(textInput_name);
      server.print("y");
      server.printP(textInput_value);
      server.print(year(t));
      server.printP(separator);
      
      server.printP(tdC);
      server.printP(tdS);
      
      // hour
      server.printP(textInput_name);
      server.print("H");
      server.printP(textInput_value);
      server.print(hour(t));
      server.printP(separator);
      
      // minute
      server.printP(textInput_name);
      server.print("M");
      server.printP(textInput_value);
      server.print(minute(t));
      server.printP(separator);
      
      // second
      server.printP(textInput_name);
      server.print("S");
      server.printP(textInput_value);
      server.print(second(t));
      server.printP(separator);
      
      
      server.printP(textInput_name);
      server.print("X");
      server.printP(textInput_value);
      server.print("Y");
      server.printP(separator);
      
      server.printP(form_end);
      
    }
    defaultEnd(server);
    
  }
}


void setLedCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  if (defaultTemplate(server, type)) {
    P(check1) = "<input type=\"checkbox\" name=\"";
    P(check2) = "\"/>";
    server.printP(check1);
    server.print("ledPin");
    server.printP(check2);
  }
  defaultEnd(server);
}

  
void setCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();
  Serial.print("getCmd -> '");
  if (type != WebServer::HEAD && type == WebServer::POST) {
    char name[10], value[10];
    int name_len, value_len;
    while (server.readPOSTparam(name, 10, value, 10)) {
      String nameStr = (String) name;
      String valueStr = (String) value;
      Serial.print(name);
      Serial.print("' : '");
      Serial.print(value);
      Serial.println("'");
      if (nameStr == "ledPin") {
        Serial.println("ledPin");
        isLedOn = valueStr == "true";
        server.println("ledPin ");
        server.println(isLedOn ? "ON" : "OFF");
      } else {
        server.print(name);
        server.print(" : ");
        server.println(value);
      }
    }
  }
}



//

String getTimeString() {
  time_t t = now();
  String timeString = "";
  timeString += day(t);
  timeString += ".";
  timeString += month(t);
  timeString += ".";
  timeString += year(t);
  timeString += " ";
  timeString += getDigits(hour(t));
  timeString += ":";
  timeString += getDigits(minute(t));
  timeString += ":";
  timeString += getDigits(second(t));
  Serial.println(timeString);
  return timeString;
}

String getDigits(int digits) {
  String returned = "";
  if (digits < 10) returned += "0";
  returned += digits;
  return returned;
}


// SETUP

void setup() {
  // initialize network connection
  Ethernet.begin(mac, ip);
  // default page
  webserver.setDefaultCommand(&helloCmd);
  // available pages
  webserver.addCommand("index", &helloCmd);
  webserver.addCommand("settings", &settingsCmd);
  webserver.addCommand("setTime", &setTimeCmd);
  webserver.addCommand("setLeds", &setLedCmd);
  webserver.addCommand("set", &setCmd);
  // start server
  webserver.begin();
  Serial.begin(9600);
  setTime(0,0,0,1,1,2012);
  pinMode(ledPin, OUTPUT);
}



// LOOP

void loop() {
  // processing connections
  webserver.processConnection();
  
  // do other stuff
  digitalWrite(ledPin, isLedOn ? HIGH : LOW);
}
