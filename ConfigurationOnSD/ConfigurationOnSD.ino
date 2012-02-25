/*
  Store configuration on SD card
  by Raidok
 */



#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>
const int chipSelect = 4;
File file;

#define IP_SIZE 4
#define IP_SEPR "."
#define MAC_SIZE 6
#define MAC_SEPR ":"
#define TIME_SIZE 2
#define TIME_SEPR ":"



// modify these
char configFileName[] = "config.txt";
byte ip[IP_SIZE] = { 192, 168, 1, 2 };
byte mac[MAC_SIZE] = { 0x90, 0xA2, 0xDA, 0x01, 0x02, 0x03 };
byte start[TIME_SIZE] = { 6, 30 };
byte stop[TIME_SIZE] = { 23, 30 };
byte interval = 60;



void setup() {
  // begin serial debugging
  Serial.begin(9600);
  Serial.println("Setup started");
  
  // setting up SD
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present.");
  } else {
    configSetup();
  }
  
  Ethernet.begin(mac, ip);
  // other stuff with start/stop/interval
}



void loop() {
  Serial.println("loop...");
  delay(10000);
}



// SD CONFIGURATION

// read configuration from SD
void configSetup() {
  // check if configfile exists
  if (SD.exists(configFileName)) {
    Serial.print(configFileName);
    Serial.println(" exists! Reading it...");
    file = SD.open(configFileName);
    if (file) {
      readConfiguration(file);
      file.close();
    }
  } else {
    Serial.print(configFileName);
    Serial.println(" does not exist! Creating it...");
    file = SD.open(configFileName, FILE_WRITE);
    if (file) {
      writeDefaultConfiguration(file);
      file.close();
    }
  }
}

byte toByte(char str[], byte base) {
  return strtol(str, NULL, base);
}

String byteArrayToString(byte arr[], const char separator[], byte size) {
  String s = "";
  for (int i = 0; i < size; i++) {
    if (i != 0) {
      s += separator;
    }
    s += arr[i];
  }
  return s;
}



// READ

void readConfiguration(File file) {
  char c, key[10], val[20];
  byte i = 0;
  boolean isKey = true;
  while (file.available()) {
    c = file.read();
    //Serial.print(c);
    if (c == '=') { // end of key
      key[i] = '\0'; // end string
      isKey = false;
      i = 0;
      continue;
    } else if (c == '\n') { // end of line
      val[i] = '\0'; // end string
      isKey = true;
      i = 0;
      evaluate(key, val);
      val[i] = '\0'; //
      continue;
    }
    if (isKey) { // if is key, save to key buffer
      key[i] = c;
    } else {
      val[i] = c;
    }
    i++;
  }
}

void evaluate(char key[], char val[]) {
  Serial.print(" -> READ key:");
  Serial.print(key);
  Serial.print(" val:");
  Serial.println(val);
  
  if ((String)key == "IP") {
    Serial.print("IP: ");
    toByteArray(ip, val, ".", IP_SIZE, DEC);
    Serial.println(byteArrayToString(ip, " - ", IP_SIZE)); //debug
  } else if ((String)key == "MAC") {
    Serial.print("MAC: ");
    toByteArray(mac, val, ":", MAC_SIZE, HEX);
    Serial.println(byteArrayToString(mac, " - ", MAC_SIZE)); //debug
  } else if ((String)key == "START") {
    Serial.print("START: ");
    toByteArray(start, val, ":", TIME_SIZE, DEC);
    Serial.println(byteArrayToString(start, " - ", TIME_SIZE)); //debug
  } else if ((String)key == "STOP") {
    Serial.print("STOP: ");
    toByteArray(stop, val, ":", TIME_SIZE, DEC);
    Serial.println(byteArrayToString(stop, " - ", TIME_SIZE)); //debug
  } else if ((String)key == "INTERVAL") {
    Serial.print("INTERVAL: ");
    interval = toByte(val, DEC);
    Serial.println(interval); //debug
  } else {
    Serial.print("SOMETHING ELSE: ");
    Serial.print(key);
    Serial.print(" : ");
    Serial.println(val);
  }
}

void toByteArray(byte *arr, char str[], char separator[], byte size, byte base) {
  byte ret[size];
  char *tmp = str;
  char *token;
  for (int i = 0; (token = strtok_r(tmp, separator, &tmp)) != NULL && i < size; i++) {
    arr[i] = toByte(token, base);
  }
}



// WRITE

void writeDefaultConfiguration(File file) {
  writeLine(file, "IP", byteArrayToString(ip, IP_SEPR, IP_SIZE));
  writeLine(file, "MAC", byteArrayToString(mac, MAC_SEPR, MAC_SIZE));
  writeLine(file, "START", byteArrayToString(start, TIME_SEPR, TIME_SIZE));
  writeLine(file, "STOP", byteArrayToString(stop, TIME_SEPR, TIME_SIZE));
  writeLine(file, "INTERVAL", String(interval));
}

void writeLine(File file, String key, String val) {
  String line = "";
  line += key;
  line += "=";
  line += val;
  Serial.print("WRITING:");
  Serial.println(line);
  file.println(line);
}
