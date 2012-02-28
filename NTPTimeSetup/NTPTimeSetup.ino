/*
 Udp NTP Client
 Modified by Raidok to work in Estonia
 
 Timezone information: http://home-4.tiscali.nl/~t876506/TZworld.html
 */

#include <SPI.h>         
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>

// MAC address of Ethernet shield
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0x03 };

// local port to listen for UDP packets
unsigned int localPort = 8888;
// ee.pool.ntp.org NTP server
IPAddress timeServer(193, 40, 5, 113);
// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE= 48;
//buffer to hold incoming and outgoing packets
byte packetBuffer[ NTP_PACKET_SIZE];
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  Serial.begin(9600);

  // start Ethernet and UDP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;);
  }
  Udp.begin(localPort);
  Serial.println("Started!");

  // send an NTP packet to a time server
  sendNTPpacket(timeServer); 
  Serial.println("NTP packet sent!");

  // wait and then see if a reply is available
  do {
    delay(1000);
  } while (!parseNTPPacket());
}

void loop() {
  digitalClockDisplay();
  delay(1000);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(dayStr(weekday()));
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(monthShortStr(month()));
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


// Stuff related to setting time

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







