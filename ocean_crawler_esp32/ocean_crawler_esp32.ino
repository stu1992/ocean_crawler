#include "secrets.h"
#include <NMEAGPS.h>

// wifi library
#include <WiFi.h>
//#include <WiFiMulti.h>

#include <HTTPClient.h>
HTTPClient http;

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
const int WATCHDOG_WIFI = 15;
const int REST_PAYLOAD_SIZE = 10;
int watchdog = WATCHDOG_RESET;
bool foundSignal = false;
const unsigned long POWER_DOWN_PERIOD = 1200; // seconds
const unsigned long DIAGNOSTIC_PERIOD = 1000 * 60; // milliseconds
const int GPS_POWER = 22;
const int SdchipSelect = 4;
bool networkUp = true;
RTC_DATA_ATTR unsigned long lastDiagnosticMessage = 0;
RTC_DATA_ATTR unsigned long millisTracked = 0;
RTC_DATA_ATTR unsigned long wokeUp = 0;

// diagnostics

RTC_DATA_ATTR  uint32_t lastLockTimestamp = 0; // if we buffer diagnostics
// retries
RTC_DATA_ATTR  unsigned int tries = 0;
char triesThisTime = 0; // we only want to count tries for a successful lock
RTC_DATA_ATTR unsigned int successfulLocks = 0;
float averageTryCount = 0;

// satelites
RTC_DATA_ATTR unsigned int attempts = 0;
RTC_DATA_ATTR unsigned int totalSatellites = 0;
float averageSatelliteCount = 0;

// wakups
RTC_DATA_ATTR unsigned int wakeups = 0;
RTC_DATA_ATTR unsigned int locks = 0;
float averageWakeupPerLock = 0;

// crawler identification
const char* uuid =  "d9b9e24a-f66d-49ea-a7d3-42b770560bc6";
// const files
const char* readingsFile = "/readings.txt";
const char* diagnosticsFile = "/diag.txt";

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
  // fire up wifi conneciton
  int connectionAttempt = 0;
  WiFi.begin(SSID, WIFI_PASS);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED || connectionAttempt < WATCHDOG_WIFI) {
    delay(1000);
    connectionAttempt ++;
    Serial.print(".");
  }
  Serial.println("");

  // try and connect
  bool everythingWorked = true; // blunt tool to not remove unpersisted events.
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    WiFiClient client;
      // Your Domain name with URL path or IP address with path


    // construct our json body for post
    
    File readingsDataFileIn = SD.open(readingsFile);
    while (readingsDataFileIn.available() && everythingWorked) {
      http.begin(client, "http://192.168.0.150/api/addHistory");
      http.addHeader("content-type", "application/json");
      bool hasMultipleElements = false;
      String httpRequestDataReadings = "{\"uuid\":\""+String(UUID)+"\", \"readings\":";
      httpRequestDataReadings+="[";
      for(int i = 0; i < REST_PAYLOAD_SIZE && readingsDataFileIn.available() ; i ++ ){
        if(hasMultipleElements){
          httpRequestDataReadings +=","; // technically not correct JSON if we have a leading or trailing comma
        }
      httpRequestDataReadings+=(readingsDataFileIn.readStringUntil('\n'));
      hasMultipleElements = true;
      }
      httpRequestDataReadings+= "]}";
    int httpResponseCodeReadings = http.POST(httpRequestDataReadings);
    Serial.println(httpRequestDataReadings);
    Serial.println(httpResponseCodeReadings);
    if(httpResponseCodeReadings != 200){
      everythingWorked = false;
    }
    http.end();
    delay(1000);
    }

    File diagnosticsDataFileIn = SD.open(diagnosticsFile);
    while (diagnosticsDataFileIn.available()) {
      http.begin(client, "http://192.168.0.150/api/addDiagnostics");
      http.addHeader("content-type", "application/json");
      bool hasMultipleElements = false;
      String httpRequestDataDiagnostics = "{\"uuid\":\""+String(UUID)+"\", \"diagnostics\":";;
      httpRequestDataDiagnostics+="[";
      for(int i = 0; i < REST_PAYLOAD_SIZE && diagnosticsDataFileIn.available() ; i ++ ){
        if(hasMultipleElements){
          httpRequestDataDiagnostics +=","; // technically not correct JSON if we have a leading or trailing comma
        }
      httpRequestDataDiagnostics+=(diagnosticsDataFileIn.readStringUntil('\n'));
      hasMultipleElements = true;
      }
      httpRequestDataDiagnostics+= "]}";
    int httpResponseCodeDiagnostics = http.POST(httpRequestDataDiagnostics);
    Serial.println(httpRequestDataDiagnostics);
    Serial.println(httpResponseCodeDiagnostics);
    if(httpResponseCodeDiagnostics != 200){
      everythingWorked = false;
    }
    http.end();
    delay(1000);
    }
  if(everythingWorked){
    SD.remove(readingsFile);
    SD.remove(diagnosticsFile);
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
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
    DEBUG_PORT.print(millisTracked);
    DEBUG_PORT.print(", \"lastLockTimestamp\" : ");
    DEBUG_PORT.print(lastLockTimestamp);
    DEBUG_PORT.println("}}");

    File dataFileOut = SD.open(diagnosticsFile, FILE_APPEND);
    if(dataFileOut){
      dataFileOut.print("{\"averageTryCount\" : ");
      dataFileOut.print(averageTryCount);
      dataFileOut.print(", \"averageSatelliteCount\" : ");
      dataFileOut.print(averageSatelliteCount);
      dataFileOut.print(", \"averageWakeupPerLock\" : ");
      dataFileOut.print(averageWakeupPerLock);
      dataFileOut.print(", \"uptime\" : ");
      dataFileOut.print(millisTracked);
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
  if (fix.valid.location && fix.satellites >= 6) {
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

    File dataFileOut = SD.open(readingsFile, FILE_APPEND);
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
    wakeups++; //diagnostics
    // SD.remove(diagnosticsFile);
    // SD.remove(readingsFile);

    // File dataFileOut = SD.open(diagnosticsFile, FILE_WRITE );
    // if(dataFileOut){
    //   dataFileOut.print("\n");
    //   dataFileOut.close();
    // }else{
    //   Serial.println("diag failed"); 
    // }

    // File readFileOut = SD.open(readingsFile, FILE_WRITE );
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
        triesThisTime++; //diagnostics
        attempts++; //diagnostics
        doSomeWork( gps.read() );
      }
      int powerDownPeriod = POWER_DOWN_PERIOD;
      // DEBUG_PORT.print(millis());
      // DEBUG_PORT.print(" > ");
      // DEBUG_PORT.print(wokeUp + ((unsigned long)WATCHDOG_RESET * 2000));
      // DEBUG_PORT.println("?");
      if(foundSignal || watchdog <= 0 || millis() > ((unsigned long)WATCHDOG_RESET * 2000)){// wokeUp will give us a rtc timeout. time out if gps serial is stalling
        DEBUG_PORT.println(lastDiagnosticMessage);
        DEBUG_PORT.println(millisTracked);
        millisTracked+= millis();
        if(millisTracked > lastDiagnosticMessage + DIAGNOSTIC_PERIOD && successfulLocks > 0 && locks > 0){
          diagnostics();
          lastDiagnosticMessage = millisTracked;
          clearReadingsBuffer();
        }
        DEBUG_PORT.println( "Sleeping" );
        DEBUG_PORT.flush();
        DEBUG_PORT.end();
        gpsPort.flush();
        Serial2.end();
        digitalWrite(GPS_POWER, LOW);
        // power down
       
        esp_sleep_enable_timer_wakeup(POWER_DOWN_PERIOD * 1000000);
        esp_deep_sleep_start();
      }else{
        watchdog --;
      }
      foundSignal = false; // reset until the next power on
    }
}
