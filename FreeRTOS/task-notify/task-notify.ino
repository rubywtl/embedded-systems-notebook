#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ==============================================
// Pin Definitions
// ==============================================
#define LED_PIN 5   // connect LED + resistor to GND
#define stacksize 4096

// ==============================================
// Task Handle(s)
// ==============================================
// Declare a global TaskHandle_t for the receiving task (BlinkTask)
// so the sender can notify it.
TaskHandle_t blink_handle;
TaskHandle_t value_handle;

// ==============================================
// Task A: BlinkTask (Receiver)
// ==============================================
//  - Configure LED pin as OUTPUT once.
//  - Block with ulTaskNotifyTake(pdTRUE, portMAX_DELAY).
//  - When a notification is received, toggle LED once.
//  - Then loop back and wait again.
void BlinkTask(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);
  while(1){
    // wait till we get the notification to toggle the led
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
    digitalWrite(LED_PIN, LOW);
  }  
}


// ==============================================
// Task B: NotifierTask (Sender)
// ==============================================
//  - Every 1000 ms, call xTaskNotifyGive(<blink task handle>).
//  - Use vTaskDelay(pdMS_TO_TICKS(1000)) for timing (no delay()).
void NotifierTask(void *pvParameters) {
  uint32_t received_value = 1000;
  while(1){
    // get new value for the delay time from config task
    xTaskNotifyWait(0, 0, &received_value, pdMS_TO_TICKS(100));

    xTaskNotifyGive(blink_handle);

    // take the new delay value
    vTaskDelay(pdMS_TO_TICKS(received_value));
  }
}

// ==============================================
// Task C: ConfigTask
// ==============================================
void ConfigTask(void *pvParameters){
  while(1){
    uint32_t my_value = random(500, 1500); // generate rand value

    // send the new delay value to task b
    xTaskNotify(value_handle, my_value, eSetValueWithOverwrite);
    printf("new delay %d\n", my_value);
    vTaskDelay(pdMS_TO_TICKS(5000)); // generate value every 5 seconds
  }
}

// ==============================================
// Setup
// ==============================================
void setup() {
  Serial.begin(115200);

  // Create BlinkTask FIRST and capture its handle.
  xTaskCreate(BlinkTask, "BlinkTask", stacksize, NULL, 1, &blink_handle);

  // Create NotifierTask SECOND.
  xTaskCreate(NotifierTask, "NotifierTask", stacksize, NULL, 1, &value_handle);

  // Create ConfigTask Third
  xTaskCreate(ConfigTask, "ConfigTask", stacksize, NULL, 1, NULL);

  // brief print that the demo started.
  Serial.println("Task Notification demo started");
}


// ==============================================
// Loop
// ==============================================
void loop() {
  // Leave empty. FreeRTOS handles scheduling.
}