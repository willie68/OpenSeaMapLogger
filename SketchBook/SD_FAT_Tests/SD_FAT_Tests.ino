/*
  SD card file dump
 
 This example shows how to read a file from the SD card using the
 SD library and send it over the serial port.
 
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 created  22 December 2010
 by Limor Fried
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 
 */
#include <SPI.h>
#include <SdFat.h>

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 10;

SdFat sd;
SdFile myFile;

char buf[80];


void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(4800);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  Serial.print(F("Initializing SD card..."));
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
    Serial.println(F("Card failed, or not present"));
    // don't do anything more:
    return;
  }
  Serial.println(F("card initialized."));

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  if (myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
    //sd.errorHalt(F("opening test.txt for write failed"));
    // if the file is available, write to it:
    myFile.println(F("\r\neine Möhre für 2"));
    myFile.close();
    /*
    dataFile = SD.open("1/test.txt",FILE_READ);
     while (dataFile.available()) {
     Serial.write(dataFile.read());
     }
     Serial.write(dataFile.name());
     dataFile.close();
     */
    Serial.println(F("write ok test.txt"));
  } 
  else {    
    Serial.println(F("error opening test.txt"));
  }
}

void loop()
{
  writeTimeStamp(millis());
}

#define print2Dec(value, stream) \
if (value < 10) {  \
    stream.print("0");  \
  }\
  stream.print(value);\

#define print3Dec(value, stream) \
if (value < 100) {  \
    stream.print("0");  \
  }\
  if (value < 10) {  \
    stream.print("0");  \
  }\
  stream.print(value);\


void writeTimeStamp(unsigned long time) {
  word mil = time % 1000;
  word div = time / 1000;

  byte sec = div % 60;
  byte minute = (div / 60) % 60;
  byte hour = (div / 3600) % 24; 

  print2Dec(hour, Serial);
  Serial.print(":");
  print2Dec(minute, Serial);
  Serial.print(":");
  print2Dec(sec, Serial);
  Serial.print(".");
  print3Dec(mil, Serial);
}


