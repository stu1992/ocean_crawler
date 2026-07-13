#include <NMEAGPS.h>
#include "LowPower.h"
#include <avr/wdt.h>
#include <SPI.h>
#include <SD.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// pull out serial pins for upload
// tx UNO pin 1 goes to rx gps pin
// rx UNO pin 0 goes to tx gps pin

// 3.3v to pin 7 if you want to hold off writing locations to serial until you're connected home
// 9600 baud rate for serial
#include <GPSport.h>

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
// system globals
//SSD1306 display;
const short WATCHDOG_RESET = 60;
const short REST_PAYLOAD_SIZE = 10;
int watchdog = WATCHDOG_RESET;
bool foundSignal = false;
const unsigned long POWER_DOWN_PERIOD = 600;
const unsigned long DIAGNOSTIC_PERIOD = 500000;
const int GPS_POWER = 8;
const int SdchipSelect = 4;
const int DONT_CLEAR_BUFFER = 7;
unsigned long lastDiagnosticMessage = 0;

// diagnostics

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

void clearReadingsBuffer(){
  File dataFileIn = SD.open("readings.txt", FILE_READ);
  while (dataFileIn.available()) {
    bool hasMultipleElements = false;
    DEBUG_PORT.print("{\"readings\":[");
    for(int i = 0; i < REST_PAYLOAD_SIZE && dataFileIn.available() ; i ++ ){
      if(hasMultipleElements){
        DEBUG_PORT.print(","); // technically not correct JSON if we have a leading or trailing comma
      }
      DEBUG_PORT.print(dataFileIn.readStringUntil('\n'));
      hasMultipleElements = true;
    }
    DEBUG_PORT.println("]}");
  }
  bool everythingWorked = true; //TODO we need line by line confirmation so we don't lose data before wiping flash
  if(everythingWorked){
    SD.remove("readings.txt");
  }
}

void sendJsonPost() {
  EthernetClient client;
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  IPAddress server(192, 168, 0, 103); // Replace with your server's IP address
  int port = 5167;   
    if (Ethernet.begin(mac) == 0) {
    DEBUG_PORT.println("Failed to configure Ethernet using DHCP");
    while (true); // Halt execution
  }
  delay(1000); // Give the Ethernet shield a second to initialize

  if (client.connect(server, port)) {
    DEBUG_PORT.println("Connected to server. Sending POST...");

    // 1. Create the JSON payload
    StaticJsonDocument<200> doc;

    doc[""] = "temperature";
    doc["value"] = 24.5;
    doc["status"] = "OK";

    // 2. Measure the exact size of the payload string
    size_t contentLength = measureJson(doc);

    // 3. Send HTTP POST Headers
    client.println("POST /addDiagnostics HTTP/1.1"); // Update to your API path
    client.println("Host: 192.168.0.103");     // Update to your server Host/IP
    client.println("Content-Type: application/json; charset=UTF-8");
    client.print("Content-Length: ");
    client.println(contentLength);
    client.println("Connection: close");
    client.println(); // CRITICAL: Blank line separating headers from body

    // 4. Send the JSON Body
    serializeJson(doc, client);
    client.println();

    // Read response from server
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        DEBUG_PORT.println(c); // Print response to Serial Monitor
      }
    }
    client.stop();
    DEBUG_PORT.println("\nDisconnected.");
  } else {
    DEBUG_PORT.println("Connection failed.");
  }
}

void diagnostics()
{
  averageTryCount =  (float)tries / (float)successfulLocks;
  averageSatelliteCount = (float)totalSatellites / (float)attempts;
  averageWakeupPerLock = (float)wakeups / (float)locks;
    DEBUG_PORT.print("{\"diagnostics\" : {");
    DEBUG_PORT.print("\"averageTryCount\" : ");
    DEBUG_PORT.print(averageTryCount);
    DEBUG_PORT.print(", \"averageSatelliteCount\" : ");
    DEBUG_PORT.print(averageSatelliteCount);
    DEBUG_PORT.print(", \"averageWakeupPerLock\" : ");
    DEBUG_PORT.print(averageWakeupPerLock);
    DEBUG_PORT.print(", \"uptime\" : ");
    DEBUG_PORT.print(millis());
    DEBUG_PORT.println("}}");
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

    File dataFileOut = SD.open("readings.txt", FILE_WRITE | O_APPEND | O_CREAT);
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
      DEBUG_PORT.println("failed to open file");
    }
    successfulLocks++; //diagnostics
    locks++; //diagnostics
    tries += triesThisTime;
    triesThisTime = 0;
  } else {
    // No valid location data yet!
    // DEBUG_PORT.print(fix.satellites);
    // DEBUG_PORT.print( " satellites" );
    // not required for benchmarking
  }
  totalSatellites += fix.satellites; // diagnostics

} // doSomeWork

//------------------------------------

static void GPSloop();
static void GPSloop()
{

  while (gps.available( gpsPort ))
    doSomeWork( gps.read() );

} // GPSloop
  
//--------------------------


void setup()
{
                   // Replace with your server's port

  pinMode(GPS_POWER, OUTPUT);
  pinMode(DONT_CLEAR_BUFFER, INPUT);
  digitalWrite(GPS_POWER, HIGH);
  DEBUG_PORT.begin(9600);
  while (!DEBUG_PORT)
    ;

  DEBUG_PORT.print( F("NMEAloc.INO: started\n") );
  DEBUG_PORT.print( F("fix object size = ") );
  DEBUG_PORT.println( sizeof(gps.fix()) );
  DEBUG_PORT.print( F("NMEAGPS object size = ") );
  DEBUG_PORT.println( sizeof(gps) );
  DEBUG_PORT.println( F("Looking for GPS device on " GPS_PORT_NAME) );


  if (!SD.begin(SdchipSelect)) {
     DEBUG_PORT.println("initialization failed. Things to check:");
    while (true);
  }
  //SD.remove("readings.txt");
  DEBUG_PORT.println("SD card alive.");

  #ifdef NMEAGPS_NO_MERGING
    DEBUG_PORT.println( F("Only displaying data from xxRMC sentences.\n  Other sentences may be parsed, but their data will not be displayed.") );
  #endif

  DEBUG_PORT.flush();

  gpsPort.begin(9600);
}

//--------------------------

void loop()
{
  //GPSloop();
    while (gps.available( gpsPort )){
      while(foundSignal == false && gps.available( gpsPort )){
        doSomeWork( gps.read() );
        triesThisTime++; //diagnostics
        attempts++; //diagnostics
      }
      int powerDownPeriod = POWER_DOWN_PERIOD;
      if(foundSignal || watchdog <= 0){
        DEBUG_PORT.println( "Sleeping" );
        DEBUG_PORT.flush();
        DEBUG_PORT.end();
        gpsPort.flush();
        gpsPort.end();
        digitalWrite(GPS_POWER, LOW);
        while(powerDownPeriod > 0){
          LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
          powerDownPeriod -= 8;
        }
          digitalWrite(GPS_POWER, HIGH);
          delay(2000);
          DEBUG_PORT.begin(9600);
          while (!DEBUG_PORT)
          delay(2000);
          gpsPort.begin(9600);
          delay(2000);
          DEBUG_PORT.println( "Waking up" );
          if(millis() > lastDiagnosticMessage + DIAGNOSTIC_PERIOD && successfulLocks > 0 && locks > 0){
            diagnostics();
            lastDiagnosticMessage = millis();
            if(digitalRead(DONT_CLEAR_BUFFER) == LOW){ // temporary fix for me to have my readings outside on a battery and not trigger a serial port dump
              clearReadingsBuffer();
            }
          }
          
          watchdog = WATCHDOG_RESET;
          wakeups++; //diagnostics
      }else{
        watchdog --;
      }
      foundSignal = false; // reset until the next power on
    }

}
