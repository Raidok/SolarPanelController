/*
  Solar panel controller
  by Raidok
 */


#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>


// NEID VÕIB VABALT MUUTA
byte mac[6] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0x03 };
byte start[2] = { 6, 30 };
byte stop[2] = { 18, 30 };
byte rewind[2] = { 3, 30 };
long interval = 15000;
long ms = 1000;


// SIIT EDASI EI TASU VÄGA PUUTUDA
boolean left = false;
boolean right = false;
boolean leftEdge = false;
boolean rightEdge = false;
boolean leftBtn = false;
boolean rightBtn = false;
boolean isRunning = false;
byte tests = 0;
byte steps = 6;
byte iteration = 0;
long endTime;
long nextRun;

#define BTN1 2
#define BTN2 3
#define END1 4
#define END2 5
#define PIN1 7
#define PIN2 8
#define NTP_PACKET_SIZE 48
#define MAX_TIME 21000

IPAddress timeServer(193, 40, 5, 113);
byte packetBuffer[NTP_PACKET_SIZE];
EthernetUDP Udp;



void setup() {
  // begin serial debugging
  Serial.begin(9600);
  Serial.println("Setup started");
  
  // begin ethernet
  Serial.print("IP: ");
  Ethernet.begin(mac);
  Serial.println(Ethernet.localIP());
  
  // set pinmodes
  pinMode(PIN1, OUTPUT);
  digitalWrite(PIN1, HIGH);
  pinMode(PIN2, OUTPUT);
  digitalWrite(PIN2, HIGH);
  
  // setting time
  /*Udp.begin(8888);
  Serial.println("Time.");
  sendNTPpacket(timeServer); 
  do {
    delay(1000);
  } while (!parseNTPPacket());*/
  
  // set a little delay
  //setTime(8, 2, 50, 1, 1, 11);
  
  endTime = millis() + 1000;
  nextRun = millis() + 1000;
  Serial.println("Start!");
  getTimeString();
}


void loop() {
  
  handleMoving();
  
  if (tests == 0) {
    moveRight(MAX_TIME);
    tests++;
    return;
  } else if (tests == 1) {
    if (right) {
      return;
    }
    tests++;
    return;
  } else if (tests == 2) {
    delay(100);
    moveLeft(MAX_TIME);
    interval = millis();
    tests++;
    return;
  } else if (tests == 3) {
    if (left) {
      return;
    }
    interval = millis() - interval;
    Serial.print("TRIP: ");
    Serial.println(interval);
    ms = interval / steps;
    Serial.print("STEP: ");
    Serial.println(ms);
    Serial.print("nxt: ");
    nextRun = millis() + ms;
    Serial.println(nextRun);
    Serial.print("now: ");
    Serial.println(millis());
    Serial.print("INTERVAL: ");
    interval = stop[0] - start[0];
    interval = interval / steps * 60 * 60 * 1000;
    Serial.println(interval);
    tests++;
    return;
  }
  
  if (leftBtn) {
    Serial.println("VASAK NUPP");
    moveLeft(ms/4);
  } else if (rightBtn) {
    Serial.println("PAREM NUPP");
    moveRight(ms/4);
  }
  
  timer();
}


void handleMoving() {// delay

  delay(10);
  
  // lets check the boundaries
  leftEdge = parseAnalog(END1);
  rightEdge = parseAnalog(END2);
  leftBtn = parseAnalog(BTN1);
  rightBtn = parseAnalog(BTN2);
  isRunning = checkClock();
  
  if (leftEdge && left && endTime > millis()) {
    //Serial.println("LEFT");
    digitalWrite(PIN1, LOW);
  } else {
    digitalWrite(PIN1, HIGH);
    left = false;
  }
  
  if (rightEdge && right && endTime > millis()) {
    //Serial.println("RIGHT");
    digitalWrite(PIN2, LOW);
  } else {
    digitalWrite(PIN2, HIGH);
    right = false;
  }
}


boolean parseAnalog(byte pin) {
  /*Serial.print("ANALOG PIN ");
  Serial.print(pin);
  Serial.print(" : ");*/
  int reading = analogRead(pin);
  //Serial.println(reading);
  return reading < 50;
}

boolean moveLeft(long time) {
  if (!(left || right) && leftEdge) {
    Serial.println("MOVE LEFT ");
    left = true;
    endTime = millis() + time;
    return true;
  }
  Serial.println("MOVE LEFT UNSUCCESSFUL");
  //diag();
  return false;
}

boolean moveRight(long time) {
  if (!(left || right) && rightEdge) {
    Serial.println("MOVE RIGHT ");
    right = true;
    endTime = millis() + time;
    return true;
  }
  Serial.println("MOVE RIGHT UNSUCCESSFUL");
  //diag();
  return false;
}


/*void diag() {
  Serial.print(" leftEdge:");
  Serial.print(leftEdge);
  Serial.print(" leftBtn:");
  Serial.print(analogRead(BTN1));
  Serial.print(" left:");
  Serial.print(left);
  Serial.print(" rightEdge:");
  Serial.print(rightEdge);
  Serial.print(" rightBtn:");
  Serial.print(analogRead(BTN2));
  Serial.print(" right:");
  Serial.println(right);
}*/

// ALARMS

boolean checkClock() {
  return (hour() > start[0] && hour() < stop[0]) ||
    (hour() == start[0] && minute() >= start[1]) ||
    (hour() == stop[0] && minute() <= stop[1]);
}

void timer() {
  boolean run = nextRun < millis();
  if (run) { // step forward
    if (isRunning && iteration < steps) {
      Serial.println("TIMER");
      nextRun = millis() + interval;
      moveRight(ms);
      iteration++;
    } else if (hour() == rewind[0] && minute() == rewind[1]) {
      Serial.println("REWIND");
      moveLeft(MAX_TIME); // turn right for 30s or until reaches end
      nextRun = millis() + 60000; // a minute forward
      iteration = 0;
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

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0; // Stratum, or type of clock
  packetBuffer[2] = 6; // Polling Interval
  packetBuffer[3] = 0xEC; // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49; 
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}

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

