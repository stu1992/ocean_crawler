#include <NMEAGPS.h>
// include file system library
#include "FS.h"
// include SD card library
#include "SD.h"
// include SPI library
#include "SPI.h"
// Define SD card connection
#define SD_MOSI     23
#define SD_MISO     19
#define SD_SCLK     18
#define SD_CS       5
File myFile;
//======================================================================
//  Program: NMEAloc.ino
//
//  Description:  This program only parses an RMC sentence for the lat/lon.
//
//  Prerequisites:
//     1) NMEA.ino works with your device (correct TX/RX pins and baud rate)
//     2) The RMC sentence has been enabled.
//     3) Your device sends an RMC sentence (e.g., $GPRMC).
//
//  'Serial' is for debug output to the Serial Monitor window.
//
//  License:
//    Copyright (C) 2014-2017, SlashDevin
//
//    This file is part of NeoGPS
//
//    NeoGPS is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NeoGPS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with NeoGPS.  If not, see <http://www.gnu.org/licenses/>.
//
//======================================================================



// pull out serial pins for upload
// tx UNO pin 1 goes to rx gps pin
// rx UNO pin 0 goes to tx gps pin
// 9600 baud rate for serial
#include <GPSport.h>
static const int RXPin = 16, TXPin = 17;

// system globals
const int WATCHDOG_RESET = 60;
const int REST_PAYLOAD_SIZE = 10;
int watchdog = WATCHDOG_RESET;
bool foundSignal = false;
const unsigned long POWER_DOWN_PERIOD = 10;
const unsigned long DIAGNOSTIC_PERIOD = 50;
const int GPS_POWER = 8;
const int SdchipSelect = 4;
bool networkUp = true;
unsigned long lastDiagnosticMessage = 0;
unsigned long wokeUp = 0;

// diagnostics

uint32_t lastLockTimestamp = 0; // if we buffer diagnostics
// retries
unsigned int tries = 0;
char triesThisTime = 0; // we only want to count tries for a successful lock
unsigned int successfulLocks = 0;
float averageTryCount = 0;

// satelites
unsigned int attempts = 0;
unsigned int totalSatellites = 0;
float averageSatelliteCount = 0;

// wakups
unsigned int wakeups = 0;
unsigned int locks = 0;
float averageWakeupPerLock = 0;

// file consts

//------------------------------------------------------------
// Check that the config files are set up properly

#if !defined( NMEAGPS_PARSE_RMC )
  #error You must uncomment NMEAGPS_PARSE_RMC in NMEAGPS_cfg.h!
#endif

#if !defined( GPS_FIX_TIME )
  #error You must uncomment GPS_FIX_TIME in GPSfix_cfg.h!
#endif

#if !defined( GPS_FIX_LOCATION )
  #error You must uncomment GPS_FIX_LOCATION in GPSfix_cfg.h!
#endif

#if !defined( GPS_FIX_SPEED )
  #error You must uncomment GPS_FIX_SPEED in GPSfix_cfg.h!
#endif

#if !defined( GPS_FIX_SATELLITES )
  #error You must uncomment GPS_FIX_SATELLITES in GPSfix_cfg.h!
#endif

#ifdef NMEAGPS_INTERRUPT_PROCESSING
  #error You must *NOT* define NMEAGPS_INTERRUPT_PROCESSING in NMEAGPS_cfg.h!
#endif

//------------------------------------------------------------

static NMEAGPS  gps; // This parses the GPS characters

//----------------------------------------------------------------
//  Print the 32-bit integer degrees *as if* they were high-precision floats

static void printL( Print & outs, int32_t degE7 );
static void printL( Print & outs, int32_t degE7 )
{
  // Extract and print negative sign
  if (degE7 < 0) {
    degE7 = -degE7;
    outs.print( '-' );
  }

  // Whole degrees
  int32_t deg = degE7 / 10000000L;
  outs.print( deg );
  outs.print( '.' );

  // Get fractional degrees
  degE7 -= deg*10000000L;

  // Print leading zeroes, if needed
  int32_t factor = 1000000L;
  while ((degE7 < factor) && (factor > 1L)){
    outs.print( '0' );
    factor /= 10L;
  }
  
  // Print fractional degrees
  outs.print( degE7 );
}
static String stringL( int32_t degE7 );
static String stringL( int32_t degE7 )
{
  String result = "";
  // Extract and print negative sign
  if (degE7 < 0) {
    degE7 = -degE7;
    result += ( '-' );
  }

  // Whole degrees
  int32_t deg = degE7 / 10000000L;
  result += ( deg );
  result += ( '.' );

  // Get fractional degrees
  degE7 -= deg*10000000L;

  // Print leading zeroes, if needed
  int32_t factor = 1000000L;
  while ((degE7 < factor) && (factor > 1L)){
    result += ( '0' );
    factor /= 10L;
  }
  
  // Print fractional degrees
  result += ( degE7 );
  return result;
}

void clearReadingsBuffer(){
  DEBUG_PORT.print("flushing readings");
  File readingsDataFileIn = SD.open("/readings.txt");
  while (readingsDataFileIn.available()) {
    bool hasMultipleElements = false;
    DEBUG_PORT.print("{\"readings\":[");
    for(int i = 0; i < REST_PAYLOAD_SIZE && readingsDataFileIn.available() ; i ++ ){
      if(hasMultipleElements){
        DEBUG_PORT.print(","); // technically not correct JSON if we have a leading or trailing comma
      }
      DEBUG_PORT.print(readingsDataFileIn.readStringUntil('\n'));
      hasMultipleElements = true;
    }
    DEBUG_PORT.println("]}");
  }
  bool everythingWorked = true; //TODO we need line by line confirmation so we don't lose data before wiping flash
  if(everythingWorked){
    SD.remove("/readings.txt");
  }

  // now print out diagnostics
  DEBUG_PORT.print("flushing diagnostics");
  File diagnosticsDataFileIn = SD.open("/diag.txt");
  while (diagnosticsDataFileIn.available()) {
    bool hasMultipleElements = false;
    DEBUG_PORT.print("{\"diagnostics\":[");
    for(int i = 0; i < REST_PAYLOAD_SIZE && diagnosticsDataFileIn.available() ; i ++ ){
      if(hasMultipleElements){
        DEBUG_PORT.print(","); // technically not correct JSON if we have a leading or trailing comma
      }
      DEBUG_PORT.print(diagnosticsDataFileIn.readStringUntil('\n'));
      hasMultipleElements = true;
    }
    DEBUG_PORT.println("]}");
  }
  if(everythingWorked){
    SD.remove("/diag.txt");
  }
}

void diagnostics()
{
  averageTryCount =  (float)tries / (float)successfulLocks;
  averageSatelliteCount = (float)totalSatellites / (float)attempts;
  averageWakeupPerLock = (float)wakeups / (float)locks;
    DEBUG_PORT.print("UPDATE: {\"diagnostics\" : {");
    DEBUG_PORT.print("\"averageTryCount\" : ");
    DEBUG_PORT.print(averageTryCount);
    DEBUG_PORT.print(", \"averageSatelliteCount\" : ");
    DEBUG_PORT.print(averageSatelliteCount);
    DEBUG_PORT.print(", \"averageWakeupPerLock\" : ");
    DEBUG_PORT.print(averageWakeupPerLock);
    DEBUG_PORT.print(", \"uptime\" : ");
    DEBUG_PORT.print(millis());
    DEBUG_PORT.print(", \"lastLockTimestamp\" : ");
    DEBUG_PORT.print(lastLockTimestamp);
    DEBUG_PORT.println("}}");

    File dataFileOut = SD.open("/diag.txt", FILE_WRITE );
    if(dataFileOut){
      dataFileOut.print("{\"averageTryCount\" : ");
      dataFileOut.print(averageTryCount);
      dataFileOut.print(", \"averageSatelliteCount\" : ");
      dataFileOut.print(averageSatelliteCount);
      dataFileOut.print(", \"averageWakeupPerLock\" : ");
      dataFileOut.print(averageWakeupPerLock);
      dataFileOut.print(", \"uptime\" : ");
      dataFileOut.print(millis());
      dataFileOut.print(", \"lastLockTimestamp\" : ");
      dataFileOut.print(lastLockTimestamp);
      dataFileOut.print("}\n");
      dataFileOut.close();
    }else{
      DEBUG_PORT.println("failed to open diagnostics file");
    }
}

static void doSomeWork( const gps_fix & fix );
static void doSomeWork( const gps_fix & fix )
{
  //  This is the best place to do your time-consuming work, right after
  //     the RMC sentence was received.  If you do anything in "loop()",
  //     you could cause GPS characters to be lost, and you will not
  //     get a good lat/lon.
  //  For this example, we just print the lat/lon.  If you print too much,
  //     this routine will not get back to "loop()" in time to process
  //     the next set of GPS data.
  if (fix.valid.location) { // && fix.satellites > 5
    foundSignal = true;
    watchdog = WATCHDOG_RESET;
    
    // // create payload for post
    uint32_t timestamp = (NeoGPS::clock_t) fix.dateTime + 946684800UL;
    
    DEBUG_PORT.println("");
    DEBUG_PORT.print("UPDATE: {\"readings\" : [{\"timestamp\" : ");
    DEBUG_PORT.print(timestamp);
    DEBUG_PORT.print(", \"latitude\" : ");
    printL( DEBUG_PORT, fix.latitudeL() );
    DEBUG_PORT.print(", \"longitude\" : ");
    printL( DEBUG_PORT, fix.longitudeL() );
    DEBUG_PORT.println("}]}");

    File dataFileOut = SD.open("/readings.txt", FILE_WRITE );
    if(dataFileOut){
      dataFileOut.print("{\"timestamp\" : ");
      dataFileOut.print(timestamp);
      dataFileOut.print(", \"latitude\" : "); 
      dataFileOut.print(stringL(fix.latitudeL()));
      dataFileOut.print(", \"longitude\" : ");
      dataFileOut.print(stringL(fix.longitudeL()));
      dataFileOut.print("}\n");
      dataFileOut.close();
    }else{
      DEBUG_PORT.println("failed to open readings file");
    }

    lastLockTimestamp = timestamp; // diagnostics
    successfulLocks++; //diagnostics
    locks++; //diagnostics
    tries += triesThisTime;
    triesThisTime = 0;
  } else {
    // No valid location data yet!
    DEBUG_PORT.print(fix.satellites);
   DEBUG_PORT.println( " satellites" );
    // not required for benchmarking
  }
  totalSatellites += fix.satellites; // diagnostics

} // doSomeWork

//------------------------------------

static void GPSloop();
static void GPSloop()
{
  while (gps.available( Serial2 )){
   
    doSomeWork( gps.read() );
  }
    
} // GPSloop
  
//--------------------------

void setup()
{
  DEBUG_PORT.begin(115200);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEAloc.INO: started\n") );
  DEBUG_PORT.print( F("fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("NMEAGPS object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " GPS_PORT_NAME) );

  #ifdef NMEAGPS_NO_MERGING
    DEBUG_PORT.println( F("Only displaying data from xxRMC sentences.\n  Other sentences may be parsed, but their data will not be displayed.") );
  #endif

  DEBUG_PORT.flush();

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) 
  {
    Serial.println("SD Card MOUNT FAIL");
  } 
  else 
  {
    Serial.println("SD Card MOUNT SUCCESS"); 
    // SD.remove("/diag.txt");
    // SD.remove("/readings.txt");

    // File dataFileOut = SD.open("/diag.txt", FILE_WRITE );
    // if(dataFileOut){
    //   dataFileOut.print("\n");
    //   dataFileOut.close();
    // }else{
    //   Serial.println("diag failed"); 
    // }

    // File readFileOut = SD.open("/readings.txt", FILE_WRITE );
    // if(readFileOut){
    //   readFileOut.print("\n");
    //   readFileOut.close();
    // }else{
    //   Serial.println("readings failed"); 
    // }
  }

  Serial2.begin(9600, SERIAL_8N1, RXPin, TXPin); 
}

//--------------------------

void loop()
{
   while (gps.available( gpsPort )){
      while(foundSignal == false && gps.available( gpsPort )){
        doSomeWork( gps.read() );
        triesThisTime++; //diagnostics
        attempts++; //diagnostics
      }
      int powerDownPeriod = POWER_DOWN_PERIOD;
      // DEBUG_PORT.print(millis());
      // DEBUG_PORT.print(" > ");
      // DEBUG_PORT.print(wokeUp + ((unsigned long)WATCHDOG_RESET * 2000));
      // DEBUG_PORT.println("?");
      if(foundSignal || watchdog <= 0 || millis() > wokeUp + ((unsigned long)WATCHDOG_RESET * 2000)){// wokeUp will give us a rtc timeout. time out if gps serial is stalling
        DEBUG_PORT.println( "Sleeping" );
        DEBUG_PORT.flush();
        DEBUG_PORT.end();
        gpsPort.flush();
        Serial2.end();
        digitalWrite(GPS_POWER, LOW);
        while(powerDownPeriod > 0){
          delay(8000);
          //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
          powerDownPeriod -= 8;
        }
          digitalWrite(GPS_POWER, HIGH);
          delay(2000);
          DEBUG_PORT.begin(115200);
          while (!DEBUG_PORT)
          delay(2000);
          Serial2.begin(9600, SERIAL_8N1, RXPin, TXPin); 
          delay(2000);
          DEBUG_PORT.println( "Waking up" );
          if(millis() > lastDiagnosticMessage + DIAGNOSTIC_PERIOD && successfulLocks > 0 && locks > 0){
            diagnostics();
            lastDiagnosticMessage = millis();
          }
          if(networkUp){ // temporary fix for me to have my readings outside on a battery and not trigger a serial port dump
            clearReadingsBuffer();
          }
          wokeUp = millis();
          watchdog = WATCHDOG_RESET;
          wakeups++; //diagnostics
          
      }else{
        watchdog --;
      }
      foundSignal = false; // reset until the next power on
    }
}
