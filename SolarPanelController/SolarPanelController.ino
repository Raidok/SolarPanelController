/*
  Solar panel controller
  by Raidok
 */


// SEADISTA NEID
byte ip[4] = { 192, 168, 1, 2 };
byte mac[6] = { 0x90, 0xA2, 0xDA, 0x01, 0x02, 0x03 };
byte start[2] = { 6, 30 };
byte stop[2] = { 23, 30 };
byte rewind[2] = { 3, 3 };
long interval = 15000;
long ms = 5000;


// SIIT EDASI EI TASU VÄGA PUUTUDA
boolean left = false;
boolean right = false;
boolean leftEdge = false;
boolean rightEdge = false;
boolean isRunning = false;
long endTime = 0;
long nextRun = 0;

#define END1 4
#define END2 5
#define PIN1 7
#define PIN2 8
#define NTP_PACKET_SIZE 48

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <WebServer.h>
#include <Time.h>
WebServer webserver("", 80);
IPAddress timeServer(193, 40, 5, 113);
byte packetBuffer[NTP_PACKET_SIZE];
EthernetUDP Udp;



P(header) = "<!DOCTYPE html><html><head><title>Solar</title>"
  "<script type=\"text/javascript\" src=\"http://code.jquery.com/jquery-latest.min.js\"></script>"
  "<script language=\"javascript\">"
  "$(document).ready(function() {"
    "$('input[type=button]').click(function(src) {"
      "$(\"DIV#status\").fadeOut();"
      "var data = {}; data[src.target.name] = \"true\";"
      "$.post(\"ajax\", data, function(data) { $(\"DIV#status\").html(data).fadeIn().delay(1000).fadeOut(); });"
    "});"
  "});</script></head><body>";
P(footer) = "</body></html>";
P(analog) = "<br/>Analoog ";
P(colon) = ": ";
P(button) = "<input type=\"button\" name=\"";
P(value) = "\" value=\"";
P(close) = "\"/>";
P(statusDiv) = "<div id=\"status\" style=\"display:inline\"/>";
P(br) = "<br>";
P(controlPage) = "<br/><a href=\"control\">Juhtimine</a><br/>";
P(frontPage) = "<br/><a href=\"/\"></a><br/>";



void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();
  if (type != WebServer::HEAD) {
    server.printP(header);
    server.print(getTimeString());
    server.printP(br);
    for (byte i = 0; i < 6; i++) {
      server.printP(analog);
      server.print(i);
      server.printP(colon);
      server.print(analogRead(i));
    }
    server.printP(br);
    server.printP(controlPage);
    server.printP(footer);
  }
}


void controlCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  //if (server.checkCredentials("a29udDpyb2xsZXI=")) {
    server.httpSuccess();
    if (type != WebServer::HEAD) {
      server.printP(header);
      server.print(getTimeString());
      server.printP(br);
      
      server.printP(button);
      server.print("left");
      server.printP(value);
      server.print("Pööra vasakule");
      server.printP(close);
      
      server.printP(button);
      server.print("right");
      server.printP(value);
      server.print("Pööra paremale");
      server.printP(close);
      
      server.printP(statusDiv);
      
      server.printP(br);
      server.printP(frontPage);
      server.printP(footer);
    } else {
      P(wrongPass) = "Vale parool!";
      server.printP(wrongPass);
    }
  //}
}



void ajaxCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();
  if (type != WebServer::HEAD && type == WebServer::POST) {
    P(success) = "<font color=\"green\">Õnnestus</font>";
    P(failure) = "<font color=\"red\">Ebaõnnestus</font>";
    char name[10], value[10];
    while (server.readPOSTparam(name, 10, value, 10)) {
      Serial.print("POST PARAM: ");
      Serial.print(name);
      Serial.print(" // ");
      Serial.println(value);
      if ((String)name == "left") {
        moveLeft(ms/2) ? server.printP(success) : server.printP(failure);
      } else if ((String)name == "right") {
        moveRight(ms/2) ? server.printP(success) : server.printP(failure);
      }
    }
  }
}




void setup() {
  // begin serial debugging
  Serial.begin(9600);
  Serial.println("Setup started");
  
  // begin ethernet
  Ethernet.begin(mac, ip);
  
  // set pinmodes
  pinMode(PIN1, OUTPUT);
  digitalWrite(PIN1, HIGH);
  pinMode(PIN2, OUTPUT);
  digitalWrite(PIN2, HIGH);
  
  // server setup
  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("index", &defaultCmd);
  webserver.addCommand("control", &controlCmd);
  webserver.addCommand("ajax", &ajaxCmd);
  webserver.begin();
  
  
  // set time
  setTime(3, 3, 00, 1, 1, 11);
  Udp.begin(8888);
  //sendNTPpacket(timeServer); 
  Serial.println("Ajapäring.");
  /*do {
    delay(1000);
  } while (!parseNTPPacket());*/
  
  // set timer
  nextRun = millis() + 1000;
}


void loop() {
  // handle web requests
  webserver.processConnection();
  
  // delay
  delay(50);
  
  // lets check the boundaries
  leftEdge = parseAnalog(END1);
  rightEdge = parseAnalog(END2);
  isRunning = checkClock();
  
  if (!leftEdge && left && endTime > millis()) {
    //Serial.println("LEFT");
    digitalWrite(PIN1, LOW);
  } else {
    digitalWrite(PIN1, HIGH);
    left = false;
  }
  
  if (!rightEdge && right && endTime > millis()) {
    //Serial.println("RIGHT");
    digitalWrite(PIN2, LOW);
  } else {
    digitalWrite(PIN2, HIGH);
    right = false;
  }
  
  timer();
}


boolean parseAnalog(byte pin) {
  /*Serial.print("ANALOG PIN ");
  Serial.print(pin);
  Serial.print(" : ");*/
  int reading = analogRead(pin);
  //Serial.println(reading);
  return reading < 50;
}


boolean moveLeft(int time) {
  if (!(left || right) && !leftEdge) {
    Serial.println("MOVE LEFT ");
    left = true;
    endTime = millis() + time;
    return true;
  }
  Serial.println("MOVE LEFT UNSUCCESSFUL");
  return false;
}

boolean moveRight(long time) {
  if (!(left || right) && !rightEdge) {
    Serial.println("MOVE RIGHT ");
    right = true;
    endTime = millis() + time;
    return true;
  }
  Serial.println("MOVE RIGHT UNSUCCESSFUL");
  return false;
}


// ALARMS

boolean checkClock() {
  return (hour() > start[0] && hour() < stop[0]) ||
    (hour() == start[0] && minute() >= start[1]) ||
    (hour() == stop[0] && minute() <= stop[1]);
}

void timer() {
  boolean run = nextRun < millis();
  if (run) { // step forward
    if (isRunning) {
      Serial.println("TIMER");
      nextRun = millis() + interval;
      moveRight(ms);
    } else if (hour() == rewind[0] && minute() == rewind[1]) {
      Serial.println("REWIND");
      moveLeft(30000); // turn right for 30s or until reaches end
      nextRun = millis() + 60000; // a minute forward
    }
  }
}


// TIME

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
  Serial.print("Web request at ");
  Serial.println(timeString);
  return timeString;
}

String getDigits(int digits) {
  String returned = "";
  if (digits < 10) returned += "0";
  returned += digits;
  return returned;
}


unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}
/*



boolean parseNTPPacket() {
  Serial.println("Checking for incoming packets.");
  // check if we've received a packet
  if ( Udp.parsePacket() ) {
    Serial.println("Parsing packet.");
    // read the received packet into the buffer
    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);
    // since Time library uses Unix timestamp, we do this
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    // adjust it
    epoch += adjustDstEurope();
    // set the Arduino's clock to that
    setTime(epoch);
    // done
    return true;
  }
  // if we reach this, no packets were received (yet)
  Serial.println("No packets yet!");
  return false;
}

// Daylight saving time adjustment
// source http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1291637975/10#10
int adjustDstEurope() {
  // last sunday of march
  int beginDSTDate = (31 - (5 * year() / 4 + 4) % 7);
  int beginDSTMonth = 3;
  //last sunday of october
  int endDSTDate = (31 - (5 * year() / 4 + 1) % 7);
  int endDSTMonth = 10;
  // DST is valid as:
  if (((month() > beginDSTMonth) && (month() < endDSTMonth))
    || ((month() == beginDSTMonth) && (day() >= beginDSTDate))
    || ((month() == endDSTMonth) && (day() <= endDSTDate))) {
    return 10800;  // summertime = utc +3 hour
  } else {
    return 7200; // wintertime = utc +2 hour
  }
}
*/
