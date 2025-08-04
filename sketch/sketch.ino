// sketch.ino ////////////////////////////////////////////////////////////////
// an unPhone example with a bit of everything... ////////////////////////////

#include <Adafruit_EPD.h>       // for pio LDF
#include "unPhone.h"

unPhone u = unPhone("everything");

static uint32_t loopIter = 0;   // time slicing iterator

void setup() { ///////////////////////////////////////////////////////////////
  // say hi, init, blink etc.
  Serial.begin(115200);
  Serial.printf("Starting build from: %s\n", u.buildTime);
  u.begin();
  u.store(u.buildTime);

  // if all three buttons pressed, go into factory test mode
#if UNPHONE_FACTORY_MODE == 1
  if(u.button1() && u.button2() && u.button3()) {
    u.factoryTestMode(true);
    u.factoryTestSetup();
    return;
  }
#endif

  // power management
  u.printWakeupReason();        // what woke us up?
  u.checkPowerSwitch();         // if power switch is off, shutdown
  Serial.printf("battery voltage = %3.3f\n", u.batteryVoltage());
  Serial.printf("enabling expander power\n");
  u.expanderPower(false);        // turn expander power on

  // flash the internal RGB LED and then the IR_LEDs
  // u.ir(true); u.rgb(0, 0, 0);
  // u.rgb(1, 0, 0); delay(300); u.rgb(0, 1, 0); delay(300);
  // u.expanderPower(false);       // expander power off
  // u.rgb(0, 0, 1); delay(300); u.rgb(1, 0, 0); delay(300);
  // u.expanderPower(true);        // expander power on
  // u.ir(false);
  // u.rgb(0, 1, 0); delay(300); u.rgb(0, 0, 1); delay(300);
  // for(uint8_t i = 0; i<4; i++) {
  //   u.ir(true);  delay(300);
  //   u.expanderPower(false);     // expander power off
  //   u.ir(false); delay(300);
  //   u.expanderPower(true);      // expander power on
  // }
  u.rgb(0, 0, 0);

  // buzz a bit
  for(int i = 0; i < 3; i++) {
    u.vibe(true);  delay(150);
    u.vibe(false); delay(150);
  }
  u.printStore();               // print out stored messages

  u.provisioned();              // redisplay the UI
  // ledTest(); // TODO on spin 7 LEDs don't work when UI0 is enabled
  Serial.println("done with setup()");
}

void loop() { ////////////////////////////////////////////////////////////////
#if UNPHONE_FACTORY_MODE == 1
  if(u.factoryTestMode()) { u.factoryTestLoop(); return; }
#endif
  if(loopIter % 25000 == 0) // allow IDLE; 100 is min to allow it to fire:
    delay(100);  // https://github.com/espressif/arduino-esp32/issues/6946
}