// Purpose: We explored the concept of interrupts, ISRs, and BLE. 
// -----------------------------------------------------------------------------

// ====== Includes =============================================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include "esp_timer.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ====== Hardware Config ======================================================
#define LCD_I2C_ADDRESS 0x27
#define SDA_PIN 16
#define SCL_PIN 17
#define BUTTON_PIN  9

#define TOGGLE_INTERVAL_US 1000000 


// ====== BLE UUIDs ============================================================
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-123456789abc"

// ====== Globals ==============================================================
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);
esp_timer_handle_t periodic_timer;

// Flags/counters shared with ISR
volatile bool tick_flag = false;
volatile bool btn_flag  = false;
volatile bool ble_flag  = false;
volatile uint32_t counter = 0;

volatile uint64_t pause_until_us = 0;
volatile unsigned long lastInterruptTime = 0;

const unsigned long debounceDelay = 200; // 200 ms

// ====== FUNCTIONS ========
// name: BLECharacteristicCallbacks
// -  turn on the ble flag and pasue for 2 seconds
class MyCallbacks: public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *pCharacteristic) {
      unsigned long current  = millis();
      ble_flag = true;
      pause_until_us = current + 2000;
   }
};

// name: onTimer
// sets the tick flag to true
void IRAM_ATTR onTimer(void* arg) {
  tick_flag = true;
}

// name: handleButtonInterrupt
// show button pressed and pause counting for 2 seconds
void IRAM_ATTR handleButtonInterrupt() {
  unsigned long current  = millis();

  if(current - lastInterruptTime > debounceDelay){
    btn_flag = true;  
    Serial.println("Interrupted");
    pause_until_us = current + 2000; // pause for 2s
    lastInterruptTime = current;
  }
}

// ====== HELPERS ========
// name: lcdShowCounter
// display a counter on the LCD screen
void lcdShowCounter(uint32_t value) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Counter:");
  lcd.setCursor(0, 1);
  lcd.print(value);
}

// name: lcdShowMessage
// display "New Message!" on the LCD screen
void lcdShowMessage(const char* msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
}

// ====== Arduino setup/loop ===================================================
void setup() {
  Serial.begin(115200);
  delay(100); // for hardware

  // LCD init
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);

  // Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);

  // periodic timer
  const esp_timer_create_args_t targs = {
    .callback = &onTimer,
    .arg = nullptr
  };

  esp_timer_create(&targs, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, TOGGLE_INTERVAL_US);

  lcdShowCounter(counter);

  // BLE
  BLEDevice::init("RF-474"); // Change this to an unique name
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                      );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  unsigned long current  = millis();
  bool paused = (current < pause_until_us);

  // Handle button event 
  if (btn_flag) {
    btn_flag = false;
    lcdShowMessage("Button Pressed");
  }

  // Handle BLE event
  if (ble_flag) {
    ble_flag = false;
    lcdShowMessage("New Message!");
  }

  // Handle the 1 Hz tick only if not paused
  if (tick_flag) {
    tick_flag = false;         // consume the tick

    if (!paused) {
      counter++;
      lcdShowCounter(counter);
    }
  }

  // light idle
  delay(5);
}