/*
 * Includes
 */
#include "Arduino.h"
#include <malloc.h>       // for free memory computation
#include <Wire.h>

#include "neocampus_debug.h"
#include "neocampus_utils.h"
#include "neocampus_i2c.h"
#include "can_stm32duino.h"

/*
 * Global variables
 */
 
bool _need2reboot = false;              // flag to tell a reboot is requested
HardwareTimer timer(TIM4); 

// --- Functions ---------------------------------------------------------------

// stm32 memory computation
extern "C" char *sbrk(int i);

// ---
// Function to send back the available memory (bytes)
unsigned int getFreeMemory( void ) {
  /* Use linker definition */
  extern char _end;
  extern char _sdata;
  extern char _estack;
  extern char _Min_Stack_Size;

  static char *ramstart = &_sdata;
  static char *ramend = &_estack;
  static char *minSP = (char*)(ramend - &_Min_Stack_Size);

  char *heapend = (char*)sbrk(0);
  char * stack_ptr = (char*)__get_MSP();
  struct mallinfo mi = mallinfo();

  return (unsigned int)(((stack_ptr < minSP) ? stack_ptr : minSP) - heapend + mi.fordblks);
}

// ---
// Setup serial link
void setupSerial( void ) {
#ifdef SERIAL_BAUDRATE
  delay(3000);  // time for USB serial link to come up anew
  Serial.begin(SERIAL_BAUDRATE); // Start serial for output

  char tmp[96];
  snprintf(tmp,sizeof(tmp),"\n\n\n\n# %s firmware rev %d for VACOP is starting ... ",getBoardName(),getFirmwareRev());
  log_info(tmp);
  log_info(F("\n#\tlog level is ")); log_info(LOG_LEVEL,DEC);
  log_info(F("\n"));
  log_flush();
#endif
}

// ---
// Setup system led
void setupSysLed( void ) {
  if( SYS_LED != INVALID_GPIO ) {  
    pinMode(SYS_LED, OUTPUT);
  }
  else {
    log_info(F("\nNO SYS_LED defined.")); log_flush();
  }
}


// ---
// Blink system led
inline void blinkSysLed( void ) {

  uint8_t _led = SYS_LED;
  if( _led != INVALID_GPIO ) {
    digitalWrite( _led, LOW);
    delay(50);
    digitalWrite( _led, HIGH);
  }
}

// ---
// process end of main loop: specific functions executed every seconds
void endLoop( void ) {
  static unsigned long _lastCheck = 0;    // elapsed ms since last check

#if 0
  // ONY FOR DEBUGGING
  static unsigned long _lastJSONdisplay = 0;    // elapsed ms since last displying shared JSON
  // 90s second elapsed ?
  if( ((millis() - _lastJSONdisplay) >= (unsigned long)90*1000UL) == true ) {
    _lastJSONdisplay = millis();
    log_debug(F("\nGlobal sharedJSON:\n")); log_flush();
    serializeJsonPretty( sharedRoot, Serial );
  }
#endif /* 0 */

  // check if a reboot has been requested ...
  if( _need2reboot ) {
    log_info(F("\n[REBOOT] a reboot has been asked ..."));log_flush();

    /*
     * stop CANopen features here
     */

    board_reboot();
    delay(5000); // to avoid an infinite loop
  }

  // a second elapsed ?
  if( ((millis() - _lastCheck) >= (unsigned long)1000UL) == true ) {
    _lastCheck = millis();

    /* blink SYS_LED */
    blinkSysLed();

    // check Heap available
    if( getFreeMemory() < 4096 ) {
      log_error(F("\n[SYS] CRTICAL available memory very low!!!")); log_flush();
    }

    // reload watchdog ?
    if( IWatchdog.isEnabled() ) {
      IWatchdog.reload();
    }

    // serial link activity marker ...
    log_debug(F("."));
  }
}



void setup() {
  /*
   * Serial link for debug ...
   */
  setupSerial();

  /*
   * Setup system led (mostly ESP8266 with blue led on GPIO2)
   */
  setupSysLed();


}

void loop() {
  

  /*
   * 
   * CANopen processing here ... unless interrupt driven :D
   *
   */

  
  /* 
   * end of main loop
   */
  // call endLoop system management level
  endLoop();
  
    // waiting a bit
  #if defined(MAIN_LOOP_DELAY_MS) and MAIN_LOOP_DELAY_MS!=0
    delay( MAIN_LOOP_DELAY_MS );
  #endif


}
