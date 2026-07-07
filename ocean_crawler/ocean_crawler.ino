#include <NMEAGPS.h>
#include "LowPower.h"
#include <avr/wdt.h>
#include <SSD1306.h>
#include <Wire.h>

// pull out serial pins for upload
// tx UNO pin 1 goes to rx gps pin
// rx UNO pin 0 goes to tx gps pin
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
// system globals
SSD1306 display;
const int WATCHDOG_RESET = 60;
int watchdog = WATCHDOG_RESET;
bool foundSignal = false;
const unsigned long POWER_DOWN_PERIOD = 600;
const unsigned long DIAGNOSTIC_PERIOD = 300000;
const int GPS_POWER = 8;
unsigned long lastDiagnosticMessage = 0;

// diagnostics

// retries
unsigned int tries = 0;
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

void diagnostics()
{
  averageTryCount =  (float)tries / (float)successfulLocks;
  averageSatelliteCount = (float)totalSatellites / (float)attempts;
  averageWakeupPerLock = (float)wakeups / (float)locks;
    DEBUG_PORT.println(tries);
    DEBUG_PORT.println(successfulLocks);
    DEBUG_PORT.println(attempts);
    DEBUG_PORT.println(totalSatellites);
    DEBUG_PORT.println(wakeups);
    DEBUG_PORT.println(locks);
    DEBUG_PORT.print("{\"diagnostics\" : {");
    DEBUG_PORT.print("\"averageTryCount\" : ");
    DEBUG_PORT.print(averageTryCount);
    DEBUG_PORT.print(", \"averageSatelliteCount\" : ");
    DEBUG_PORT.print(averageSatelliteCount);
    DEBUG_PORT.print(", \"averageWakeupPerLock\" : ");
    DEBUG_PORT.print(averageWakeupPerLock);
    DEBUG_PORT.println("}}");

  display.clear();
  display.setCaret(0, 0);
  display.print("{\"diagnostics\" : {");
  display.print("\"averageTryCount\" : ");
  display.print(averageTryCount);
  display.print(", \"averageSatelliteCount\" : ");
  display.print(averageSatelliteCount);
  display.print(", \"averageWakeupPerLock\" : ");
  display.print(averageWakeupPerLock);
  display.print("}}");
  display.update();
  
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
    // DEBUG_PORT.print("20");
    // DEBUG_PORT.print(fix.dateTime.year);
    // DEBUG_PORT.print("/");
    // if ( fix.dateTime.month < 10 )
    //   DEBUG_PORT.print( '0' );
    // DEBUG_PORT.print(fix.dateTime.month);
    // DEBUG_PORT.print("/");
    // if ( fix.dateTime.date < 10 )
    //   DEBUG_PORT.print( '0' );
    // DEBUG_PORT.print(fix.dateTime.date);
    // DEBUG_PORT.print(" : ");
    // DEBUG_PORT.print(fix.dateTime.hours);
    // DEBUG_PORT.print(":");
    // if ( fix.dateTime.minutes < 10 )
    //   DEBUG_PORT.print( '0' );
    // DEBUG_PORT.print(fix.dateTime.minutes);
    // DEBUG_PORT.print(":");
    // if ( fix.dateTime.seconds < 10 )
    //   DEBUG_PORT.print( '0' );
    // DEBUG_PORT.print(fix.dateTime.seconds);
    // DEBUG_PORT.print(" -> ");

    // printL( DEBUG_PORT, fix.latitudeL() ); // prints int like a float
    // DEBUG_PORT.print( ',' );
    // printL( DEBUG_PORT, fix.longitudeL() ); // prints int like a float
    
    // // create payload for post
    uint32_t timestamp = (NeoGPS::clock_t) fix.dateTime + 946684800UL;
    DEBUG_PORT.println("");
    DEBUG_PORT.print("{\"readings\" : [{\"timestamp\" : ");
    DEBUG_PORT.print(timestamp);
    DEBUG_PORT.print(", \"latitude\" : ");
    printL( DEBUG_PORT, fix.latitudeL() );
    DEBUG_PORT.print(", \"longitude\" : ");
    printL( DEBUG_PORT, fix.longitudeL() );
    DEBUG_PORT.println("}]}");

  display.clear();
  display.setCaret(0, 0);
  display.print("{\"readings\" : [{\"timestamp\" : ");
  display.print((int)timestamp);
  display.print(", \"latitude\" : ");
  display.print(fix.latitude(), 8);
  display.print(", \"longitude\" : ");
  display.print(fix.longitude(),8 );
  display.print("}]}");
  display.update();
  
    successfulLocks++; //diagnostics
    locks++; //diagnostics
  } else {
    // No valid location data yet!
    DEBUG_PORT.print(fix.satellites);
    DEBUG_PORT.print( " satellites" );
  }
  totalSatellites += fix.satellites; // diagnostics
  DEBUG_PORT.println();

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
  pinMode(GPS_POWER, OUTPUT);
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

  #ifdef NMEAGPS_NO_MERGING
    DEBUG_PORT.println( F("Only displaying data from xxRMC sentences.\n  Other sentences may be parsed, but their data will not be displayed.") );
  #endif

  DEBUG_PORT.flush();

  gpsPort.begin(9600);


  // display
   Wire.begin();

  display.init();
  display.clear();

  display.print('!');
  display.print(F("\"#$%&'()*+,-./"));
  display.setCaret(10, 10);
  display.setScale(2);
  display.print(F("0123456789"));
  display.setCaret(40, 28);
  display.setScale(1);
  display.print(F(":;<=>?@"));
  display.setCaret(0, 40);
  display.print(F("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  display.print(F("[\\]^_`"));
  display.print(F("abcdefghijklmnopqrstuvwxyz"));
  display.print(F("{|}~"));

  display.update();
}

//--------------------------

void loop()
{
  //GPSloop();
    while (gps.available( gpsPort )){
      while(foundSignal == false && gps.available( gpsPort )){
        doSomeWork( gps.read() );
        tries++; //diagnostics
        attempts++; //diagnostics
      }
      int powerDownPeriod = POWER_DOWN_PERIOD;
      if(foundSignal || watchdog <= 0){
        DEBUG_PORT.println( "Sleeping" );
        DEBUG_PORT.flush();
        DEBUG_PORT.end();
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
          //diagnostics();
          DEBUG_PORT.println(millis());
          DEBUG_PORT.println(lastDiagnosticMessage);
          DEBUG_PORT.println(DIAGNOSTIC_PERIOD);
          if(millis() > lastDiagnosticMessage + DIAGNOSTIC_PERIOD){
            diagnostics();
            lastDiagnosticMessage = millis();
          }
          watchdog = WATCHDOG_RESET;
          wakeups++; //diagnostics
      }else{
        watchdog --;
        //DEBUG_PORT.print( F("Watchdog: ") );
        //DEBUG_PORT.println(watchdog);
      }
      foundSignal = false; // reset until the next power on
    }

}
