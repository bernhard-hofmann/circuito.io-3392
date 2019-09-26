// Include Libraries
#include <Wire.h>
#include "Arduino.h"
#include "LiquidCrystal_PCF8574.h"
#include "Button.h"
#include "Relay.h"

// Pin Definitions
#define PIEZOVIBRATION1_PIN_NEG A3
#define PIEZOVIBRATION2_PIN_NEG A1
#define ARCADEBUTTON_PIN_NO       2
#define relayModule1_PIN_SIGNAL 3
#define relayModule2_PIN_SIGNAL 4

// Global variables and defines

// There are several different versions of the LCD I2C adapter, each might have a different address.
// Try the given addresses by Un/commenting the following rows until LCD works follow the serial monitor prints.
// To find your LCD address go to: http://playground.arduino.cc/Main/I2cScanner and run example.
#define LCD_ADDRESS 0x3F
//#define LCD_ADDRESS 0x27

// Define LCD characteristics
#define LCD_ROWS 2
#define LCD_COLUMNS 16
#define SCROLL_DELAY 150
#define BACKLIGHT 255

// object initialization
LiquidCrystal_PCF8574 lcdI2C;
Button ArcadeButton(ARCADEBUTTON_PIN_NO);
Relay relayModule1(relayModule1_PIN_SIGNAL);
Relay relayModule2(relayModule2_PIN_SIGNAL);

/* Original requirements description:
 Build a pair of nerf quickdraw targets.
 I want to be able to press a button and then have the Arduino wait a random 2- 5 seconds
 then turn a 12v LED on each target on using a relay. The lit LED means that each player
 may draw their blaster and shoot the target. once this happens I want to have a piezo
 sensor on each target to sense how fast each person was and then have the faster person’s
 target turn solid while the other person’s turns off. And to make it a little more
 competitive I would also like to add an LCD to display the time of each player.
 */

/* Requirements translated to pseudo-code
 When the button is pressed
 Reset target "hit" statuses
 Turn off both relays/LEDs (flash them a few times before turning off?)
 Show "Get Ready!" on the LCD
 Set a timer for between 2~5 seconds from now (random value)
 When the timer event occurs
 Turn on both relays/LEDs
 Mark the current time (to be used later to measure reaction time)
 When a target is hit
 Check whether the timer has been reached
 If not
 show false start for player 1|2 on the LCD
 turn off the relay/LED for this target
 otherwise
 turn off the relay/LED of the other target (this one stays on to indicate the winner)
 show time difference from turning on until being hit
 
 Suggested LCD output:
 when player 2 hit the target before it's enabled:
 PLAYER 1    0.2s
 PLAYER 2   CHEAT
 when player 2 beats player 1
 PLAYER 1    1.2s
 PLAYER 2****0.8s
 1234567890123456
 */

bool isTarget1Hit;
bool isTarget2Hit;
unsigned long startTime;
unsigned long delayTime;
bool targetsActive;
bool gameOver = false;

// The values for the targets will be read using analogRead to get a value from 0 through 1023.
int target1Value = 0;
int target2Value = 0;

// Target values can be from 0 through 1023. To avoid false "hit" detection, modify the threshold that the value must exceed to be recognised as a hit.
// These are likely to be the same, but the reality is that one target might be more sensitive than another so we use two values for flexibility.
int target1Threshold = 512;
int target2Threshold = 512;

unsigned long player1HitTime;
unsigned long player2HitTime;

bool player1FalseStart = false;
bool player2FalseStart = false;

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup()
{
  // Setup Serial which is useful for debugging
  // Use the Serial Monitor to view printed messages
  Serial.begin(9600);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB
  Serial.println("start");

  // initialize the lcd
  lcdI2C.begin(LCD_COLUMNS, LCD_ROWS, LCD_ADDRESS, BACKLIGHT);
  ArcadeButton.init();

  lcdI2C.clear();
  lcdI2C.print("Startup...");
  delay(800);

  // Enter a diagnostics / test process if the button is pressed during startup
  bool arcadeButtonVal = ArcadeButton.read();
  if (arcadeButtonVal == HIGH) {
    lcdI2C.clear();
    lcdI2C.print("TEST MODE");
    lcdI2C.selectLine(2);
    lcdI2C.print("Release button");
    while (arcadeButtonVal == HIGH) {
      delay(10);
      arcadeButtonVal = ArcadeButton.read();
    }

    lcdI2C.selectLine(2);
    lcdI2C.print("Relay 1 ON      ");
    relayModule1.on();
    delay(2000);

    lcdI2C.selectLine(2);
    lcdI2C.print("Relay 2 ON      ");
    relayModule2.on();
    delay(2000);

    lcdI2C.selectLine(2);
    lcdI2C.print("Relay 1 OFF     ");
    relayModule1.off();
    delay(2000);

    lcdI2C.selectLine(2);
    lcdI2C.print("Relay 2 OFF     ");
    relayModule2.off();
    delay(2000);

    target1Value = analogRead(PIEZOVIBRATION1_PIN_NEG);
    unsigned long start = millis();
    while (millis() < start + 5000) {
      delay(50);
      target1Value = analogRead(PIEZOVIBRATION1_PIN_NEG);
      char buf[21];
      sprintf(buf,"PV1: %04d       ",target1Value);
      lcdI2C.selectLine(2);
      lcdI2C.print(buf);
    }

    target2Value = analogRead(PIEZOVIBRATION2_PIN_NEG);
    unsigned long start = millis();
    while (millis() < start + 5000) {
      delay(50);
      target2Value = analogRead(PIEZOVIBRATION2_PIN_NEG);
      char buf[21];
      sprintf(buf,"PV2: %04d       ",target2Value);
      lcdI2C.selectLine(2);
      lcdI2C.print(buf);
    }

  } // End diagnostics

  // LCD 16x2 I2C - Test Code
  // The LCD Screen will display the text of your choice.
  lcdI2C.clear();                          // Clear LCD screen.
  lcdI2C.print("Ready");                   // Print print String to LCD on first line
  lcdI2C.selectLine(2);                    // Set cursor at the begining of line 2
  lcdI2C.print("Press the button");        // Print print String to LCD on second line

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
}

// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop()
{
  // Concave Arcade Button - Red (without LED) - Test Code
  //Read Arcade pushbutton (without LED)  state.
  //if button is pressed function will return HIGH (1). if not function will return LOW (0).
  //for debounce funtionality try also ArcadeButton.onPress(), .onRelease() and .onChange().
  //if debounce is not working properly try changing 'debounceDelay' variable in Button.h
  bool arcadeButtonVal = ArcadeButton.read();

  if (arcadeButtonVal == HIGH) {
    // When the button is pressed
    // Reset target "hit" statuses
    isTarget1Hit = false;
    isTarget2Hit = false;

    // Turn off both relays/LEDs (flash them a few times before turning off?)
    relayModule1.off();
    relayModule2.off();

    player1HitTime = 0;
    player2HitTime = 0;

    // Show "Get Ready!" on the LCD
    lcdI2C.clear();
    lcdI2C.print("Get Ready!");

    // Set a timer for between 2~5 seconds from now (random value)
    startTime = millis(); // Returns the number of milliseconds passed since the Arduino board began running the current program. This number will overflow (go back to zero), after approximately 50 days.
    Serial.print("Button pressed at: "); 
    Serial.println(String(startTime));
    startTime += random(2000, 5000);
    Serial.print("Random StartTime : "); 
    Serial.println(String(startTime));
    targetsActive = false;
    gameOver = false;
  }

  // Temporary test code - turn on the relays when the start time is reached and the targets are not already active
  if (!targetsActive && millis() >= startTime) {
    targetsActive = true;
    relayModule1.on();
    relayModule2.on();
  }

  // I don't have a Piezo Vibration Sensor but the documentation says that it is suitable for measurements of flexibility, vibration, impact and touch.
  // When the sensor moves back and forth, a certain voltage will be created by the voltage comparator inside of it. 
  // Returned values: from 0 (no strain) to 1023 (maximal strain).
  target1Value = analogRead(PIEZOVIBRATION1_PIN_NEG);
  if (player1HitTime == 0 && target1Value > target1Threshold) {
    player1HitTime = millis();
    if (targetsActive) {
      player1FalseStart = false;
    } 
    else {
      player1FalseStart = true;
    }
  }

  // The exact same code for target 2
  target2Value = analogRead(PIEZOVIBRATION2_PIN_NEG);
  if (player2HitTime == 0 && target2Value > target2Threshold) {
    player2HitTime = millis();
    if (targetsActive) {
      player2FalseStart = false;
    } 
    else {
      player2FalseStart = true;
    }
  }

  // Update the display if a target has been hit
  if (!gameOver && (player1HitTime > 0 || player2HitTime > 0)) {
    if (player1HitTime > 0 && player2HitTime > 0) {
      gameOver = true;
    }

    lcdI2C.clear();
    lcdI2C.print("PLAYER 1 ");
    if (player1FalseStart) {
      lcdI2C.print("    !!!");
    } 
    else {
      if (player1HitTime == 0) {
        lcdI2C.print("    ...");
      }
      else {
        delayTime = player1HitTime - startTime;
        int s = delayTime/1000;
        int ms = delayTime - s*1000;
        char buf[21];
        sprintf(buf,"%02d.%03d",s, ms);
        lcdI2C.print(buf);
      }
    }

    lcdI2C.selectLine(2);
    lcdI2C.print("PLAYER 2 ");
    if (player2FalseStart) {
      lcdI2C.print("    !!!");
    } 
    else {
      if (player2HitTime == 0) {
        lcdI2C.print("    ...");
      }
      else {
        delayTime = player2HitTime - startTime;
        int s = delayTime/1000;
        int ms = delayTime - s*1000;
        char buf[21];
        sprintf(buf,"%02d.%03d",s, ms);
        lcdI2C.print(buf);
      }
    }
  }

  // Turn off the losing LED and blink the other - assumes setting a relay to the state it's already in has no effect
  if (gameOver) {
    if (player1FalseStart && player2FalseStart) {
      // No winner, both LEDs off
      relayModule1.off();
      relayModule2.off();
    }
    else {
      // We're going to ignore draws because of the extremely low chance both targets are hit at the same millisecond
      if (player1FalseStart || player1HitTime > player2HitTime) {
        relayModule1.off();
        // Blink the other relay simply by turning it off/on based on the current time
        if ((millis()/500) % 2 < 1) {
          relayModule2.off();
        }
        else {
          relayModule2.on();
        }
      }

      if (player2FalseStart || player2HitTime > player1HitTime) {
        relayModule2.off();
        // Blink the other relay simply by turning it off/on based on the current time
        if ((millis()/500) % 2 < 1) {
          relayModule1.off();
        }
        else {
          relayModule1.on();
        }
      }
    }
  }

  // A short delay prevents erroneous button press readings and saves a little processing power
  delay(50);
}

