// --------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

// --------------------------------------------------------------------
// Macros & Globals
// --------------------------------------------------------------------

// The command 0x80 sets the cursor to the beginning of the first line.
// The command 0x01 clears the LCD screen.
#define CLEARSCREEN 0x01
#define RESETCURSOR 0x80

// address of the LCD
#define LCD_I2C_ADDRESS 0x27

// pins for the wire of lcd
#define SDA_PIN 16
#define SCL_PIN 17

// --------------------------------------------------------------------
// Functions
// --------------------------------------------------------------------

LiquidCrystal_I2C lcd(0x27, 16, 2); // initialize the LCD

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  lcd.init();
  lcd.clear();
  lcd.setCursor(0, 0);
}

// name: set_command (assume the input is 8 bits)
// @param
// - command: an integer
// description: send a command to the lcd
void set_command(unsigned int command) {
  // split 8 bits command (2 hex values) into 2 packets

  // get the most sig 4 bits
  unsigned int high_nibble = (command & 0xF0); // 0b1101 1111 & 0b1111 0000 = 0b1101 0000
  // get the least sig 4 bits with shifting + mask
  unsigned int low_nibble = ((command << 4) & 0xF0);

  // set the second param to false because we are sending a command
  send_packet(high_nibble, false);
  send_packet(low_nibble, false);

  delay(2); // delay a bit for hardware purposes
}

// name: set_string
// @param
// - command: a string to be displayed
// description: send a string to be displayed on the screen
void set_string(String displaystr){
  // loop thorugh each character of the string we want to display
  for(char& c : displaystr){
    // lcd packet is only 4 bits of data and the other 4 is controls
    // char is 1 byte which is 8 bits, so we need two packets

    // Bits 4-7: This is 4-bits of the command or data we want to send.
    unsigned int high_nibble = (c & 0xF0);
    unsigned int low_nibble = ((c << 4) & 0xF0);

    // set the second param to true because we are sending data not command
    send_packet(high_nibble, true);
    send_packet(low_nibble, true);
  }
}

// name: send_packet
// description: send the 4-bit packet to the lcd via wire library
void send_packet(unsigned int data, bool is_data) {
  // CONTROL BITS:
  // Bit 0: This is the register select bit
  //      -> most sig figs are data (1)
  // Bit 1: This is the read/write(0) bit
  // Bit 2: This is the enable bit (1)
  // Bit 3: This is the backlight control bit (1)

  unsigned int control_bits = 0b00001000; // we are writing

  if (is_data) {
    control_bits |= 0b00000001; // set register select bit to 1 for data
  }

  Wire.beginTransmission(LCD_I2C_ADDRESS);
  Wire.write(data | control_bits| 0b00000100);  // enable high
  Wire.endTransmission();
  delayMicroseconds(50); // some delays for hardware purpose

  Wire.beginTransmission(LCD_I2C_ADDRESS);
  Wire.write(data | control_bits); // enable low to latch the input
  Wire.endTransmission();
  delayMicroseconds(50); // some delays for hardware purpose
}

void loop() {
  // if there is incoming inputs
  if (Serial.available() > 0) { // check if there is data available in the serial buffer
    String receivedString = Serial.readString(); // get the string

    // Step 1: clean screen ex: lcd_command(0x01);
    printf("[log] reset screen\n");
    set_command(CLEARSCREEN);

    // Step 2: set cursor ex: lcd_command(0x80);
    printf("[log] set cursor\n");
    set_command(RESETCURSOR);

    // Step 3: start printing the string
    printf("[log] send string\n");
  
    set_string(receivedString);
  }
}
