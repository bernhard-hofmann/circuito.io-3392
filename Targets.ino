// Include Libraries
#include "Arduino.h"
#include "LiquidCrystal_PCF8574.h"
#include "Button.h"
#include "Relay.h"

// Pin Definitions
#define PIEZOVIBRATION1_1_PIN_NEG	A3
#define PIEZOVIBRATION2_2_PIN_NEG	A1
#define ARCADEBUTTON_PIN_NO	2
#define RELAYMODULE1_1_PIN_SIGNAL	3
#define RELAYMODULE2_2_PIN_SIGNAL	4

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
Relay relayModule1_1(RELAYMODULE1_1_PIN_SIGNAL);
Relay relayModule2_2(RELAYMODULE2_2_PIN_SIGNAL);


// define vars for testing menu
const int timeout = 10000;       //define timeout of 10 sec
long time0;

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

    // LCD 16x2 I2C - Test Code
    // The LCD Screen will display the text of your choice.
    lcdI2C.clear();                          // Clear LCD screen.
    lcdI2C.print("Ready");                   // Print print String to LCD on first line
    lcdI2C.selectLine(2);                    // Set cursor at the begining of line 2
    lcdI2C.print("");                        // Print print String to LCD on second line
}

// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop() 
{
    // Concave Arcade Button - Red (without LED) - Test Code
    //Read Arcade pushbutton (without LED)  state. 
    //if button is pressed function will return HIGH (1). if not function will return LOW (0). 
    //for debounce funtionality try also ArcadeButton.onPress(), .onRelease() and .onChange().
    //if debounce is not working properly try changing 'debounceDelay' variable in Button.h
    bool ArcadeButtonVal = ArcadeButton.read();

    // Relay Module #1 - Test Code
    // The relay will turn on and off for 500ms (0.5 sec)
    relayModule1_1.on();
    relayModule2_2.on();
    delay(500);
    relayModule1_1.off();
    relayModule2_2.off();
    delay(500);

    delay(1000);
}
