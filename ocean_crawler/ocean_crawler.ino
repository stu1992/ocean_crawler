#include <NMEAGPS.h>
#include "LowPower.h"
#include <avr/wdt.h>
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
const int WATCHDOG_RESET = 60;
int watchdog = WATCHDOG_RESET;
bool foundSignal = false;
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

  if (fix.valid.location && fix.satellites > 5) {
    DEBUG_PORT.println("found a signal");
    foundSignal = true;
    watchdog = WATCHDOG_RESET;
    DEBUG_PORT.print("20");
    DEBUG_PORT.print(fix.dateTime.year);
    DEBUG_PORT.print("/");
    if ( fix.dateTime.month < 10 )
      DEBUG_PORT.print( '0' );
    DEBUG_PORT.print(fix.dateTime.month);
    DEBUG_PORT.print("/");
    if ( fix.dateTime.date < 10 )
      DEBUG_PORT.print( '0' );
    DEBUG_PORT.print(fix.dateTime.date);
    DEBUG_PORT.print(" : ");
    DEBUG_PORT.print(fix.dateTime.hours);
    DEBUG_PORT.print(":");
    if ( fix.dateTime.minutes < 10 )
      DEBUG_PORT.print( '0' );
    DEBUG_PORT.print(fix.dateTime.minutes);
    DEBUG_PORT.print(":");
    if ( fix.dateTime.seconds < 10 )
      DEBUG_PORT.print( '0' );
    DEBUG_PORT.print(fix.dateTime.seconds);
    DEBUG_PORT.print(" -> ");

    // DEBUG_PORT.print( fix.latitude(), 6 ); // floating-point display
    // DEBUG_PORT.print( fix.latitudeL() ); // integer display
    printL( DEBUG_PORT, fix.latitudeL() ); // prints int like a float
    DEBUG_PORT.print( ',' );
    // DEBUG_PORT.print( fix.longitude(), 6 ); // floating-point display
    // DEBUG_PORT.print( fix.longitudeL() );  // integer display
    printL( DEBUG_PORT, fix.longitudeL() ); // prints int like a float
    
  } else {
    // No valid location data yet!
    DEBUG_PORT.print( "GPS available but no lock found" );
  }

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

const int GPS_POWER = 8;
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
}

//--------------------------

void loop()
{
  //GPSloop();
    while (gps.available( gpsPort )){
      while(foundSignal == false && gps.available( gpsPort )){
        doSomeWork( gps.read() );
      }
      int powerDownPeriod = 600;
      if(foundSignal || watchdog <= 0){
        DEBUG_PORT.flush();
        DEBUG_PORT.end();
        gpsPort.end();
        digitalWrite(GPS_POWER, LOW);
        while(powerDownPeriod > 0){
          LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
          powerDownPeriod -= 8;
        }
          digitalWrite(GPS_POWER, HIGH);
          delay(1000);
          DEBUG_PORT.begin(9600);
          while (!DEBUG_PORT)
          delay(1000);
          gpsPort.begin(9600);
          delay(1000);
      }else{
        watchdog --;
        DEBUG_PORT.print( F("Watchdog: ") );
        DEBUG_PORT.println(watchdog);
      }
      foundSignal = false; // reset until the next power on
    }

}
