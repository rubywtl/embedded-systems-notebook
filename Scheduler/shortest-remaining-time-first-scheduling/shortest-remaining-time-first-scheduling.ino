#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------

/** @brief LED output pin */
#define LED_PIN 5
/** @brief I2C SDA pin */
#define SDA_PIN 16
/** @brief I2C SCL pin */
#define SCL_PIN 17
/** @brief I2C address of LCD */
#define LCD_I2C_ADDRESS 0x27

/** @brief FreeRTOS tick conversion for 50ms quantum */
const TickType_t QUANTUM = pdMS_TO_TICKS(250);

/** @brief Total execution times for tasks (in ticks) */
const TickType_t ledTaskExecutionTime      = 500 / portTICK_PERIOD_MS;   ///< 500 ms
const TickType_t counterTaskExecutionTime  = 2000 / portTICK_PERIOD_MS;  ///< 2 s
const TickType_t alphabetTaskExecutionTime = 13000 / portTICK_PERIOD_MS; ///< 13 s

/** @brief Remaining execution times (volatile since modified in tasks) */
volatile TickType_t remainingLedTime      = ledTaskExecutionTime;
volatile TickType_t remainingCounterTime  = counterTaskExecutionTime;
volatile TickType_t remainingAlphabetTime = alphabetTaskExecutionTime;

/** @brief Quantum in ticks for decrementing */
const TickType_t QUANTUM_TICKS = 50 / portTICK_PERIOD_MS;

/** @brief Task state tracking */
volatile bool ledState = false;
volatile int counterValue = 1;
volatile char alphabetChar = 'A';

/** @brief Track when each task last completed for staggered arrivals */
volatile TickType_t ledLastCompletion      = 0;
volatile TickType_t counterLastCompletion  = 0;
volatile TickType_t alphabetLastCompletion = 0;

/** @brief Arrival periods for each task */
const TickType_t LED_PERIOD      = 1250 / portTICK_PERIOD_MS; ///< LED arrives every 1.25 s
const TickType_t COUNTER_PERIOD  = 5000 / portTICK_PERIOD_MS; ///< Counter every 5 s
const TickType_t ALPHABET_PERIOD = 7500 / portTICK_PERIOD_MS; ///< Alphabet every 7.5 s

/** @brief FreeRTOS task handles */
TaskHandle_t ledTaskHandle     = NULL;
TaskHandle_t counterTaskHandle = NULL;
TaskHandle_t alphaTaskHandle   = NULL;
TaskHandle_t schedulerHandle   = NULL;

/** @brief LCD object */
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);

// -------------------------------------------------------------------
// Task Functions
// -------------------------------------------------------------------

/**
 * @brief LED task toggles the LED state.
 *
 * This task flips the LED state each time it runs and then waits to be
 * suspended by the scheduler.
 *
 * @param parameter Unused FreeRTOS parameter
 */
void ledTask(void *parameter) {
  pinMode(LED_PIN, OUTPUT);

  while (1) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);

    vTaskDelay(portMAX_DELAY); // wait to be suspended
  }
}

/**
 * @brief Counter task updates LCD and prints counter value.
 *
 * Increments counterValue and wraps after 20. Waits to be suspended by scheduler.
 *
 * @param parameter Unused
 */
void counterTask(void *parameter) {
  while (1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Counter: ");
    lcd.print(counterValue);
    // Serial.printf(">>> Counter: %d\n", counterValue);

    counterValue++;
    if (counterValue > 20) counterValue = 1;

    vTaskDelay(portMAX_DELAY); // wait to be suspended
  }
}

/**
 * @brief Alphabet task prints letters cyclically.
 *
 * Increments alphabetChar from 'A' to 'Z' and wraps. Waits to be suspended.
 *
 * @param parameter Unused
 */
void alphaTask(void *parameter) {
  while (1) {
    Serial.printf(">>> Alphabet: %c\n", alphabetChar);

    alphabetChar++;
    if (alphabetChar > 'Z') alphabetChar = 'A';

    vTaskDelay(portMAX_DELAY); // wait to be suspended
  }
}

/**
 * @brief Scheduler task implementing Shortest Remaining Time First (SRTF).
 *
 * Checks task arrivals, selects the task with the shortest remaining time,
 * runs it for one quantum, and then suspends it again.
 *
 * @param parameter Unused
 */
void schedulerTask(void *parameter) {
  // Start with all tasks suspended
  vTaskSuspend(ledTaskHandle);
  vTaskSuspend(counterTaskHandle);
  vTaskSuspend(alphaTaskHandle);

  TickType_t currentTime = 0;

  while (1) {
    // Check if tasks should "arrive" (become ready)
    if (remainingLedTime == 0 && (currentTime - ledLastCompletion >= LED_PERIOD)) {
      remainingLedTime = ledTaskExecutionTime;
      ledLastCompletion = currentTime;
      Serial.println("\n*** LED TASK ARRIVED ***");
    }

    if (remainingCounterTime == 0 && (currentTime - counterLastCompletion >= COUNTER_PERIOD)) {
      remainingCounterTime = counterTaskExecutionTime;
      counterLastCompletion = currentTime;
      Serial.println("\n*** COUNTER TASK ARRIVED ***");
    }

    if (remainingAlphabetTime == 0 && (currentTime - alphabetLastCompletion >= ALPHABET_PERIOD)) {
      remainingAlphabetTime = alphabetTaskExecutionTime;
      alphabetLastCompletion = currentTime;
      Serial.println("\n*** ALPHABET TASK ARRIVED ***");
    }

    // ---- SRTF selection ----
    TickType_t shortest = 999999;
    TaskHandle_t nextTask = NULL;

    if (remainingLedTime > 0 && remainingLedTime < shortest) {
      shortest = remainingLedTime;
      nextTask = ledTaskHandle;
    }
    if (remainingCounterTime > 0 && remainingCounterTime < shortest) {
      shortest = remainingCounterTime;
      nextTask = counterTaskHandle;
    }
    if (remainingAlphabetTime > 0 && remainingAlphabetTime < shortest) {
      shortest = remainingAlphabetTime;
      nextTask = alphaTaskHandle;
    }

    // Run selected task
    if (nextTask != NULL) {
      vTaskResume(nextTask);
      vTaskDelay(QUANTUM);
      vTaskSuspend(nextTask);

      // Decrement remaining time
      if (nextTask == ledTaskHandle) {
        remainingLedTime -= QUANTUM_TICKS;
        if (remainingLedTime < 0) remainingLedTime = 0;
      } else if (nextTask == counterTaskHandle) {
        remainingCounterTime -= QUANTUM_TICKS;
        if (remainingCounterTime < 0) remainingCounterTime = 0;
      } else if (nextTask == alphaTaskHandle) {
        remainingAlphabetTime -= QUANTUM_TICKS;
        if (remainingAlphabetTime < 0) remainingAlphabetTime = 0;
      }
    } else {
      // Idle state
      Serial.printf("Time:%4d | LED:%3d CNT:%4d ALPHA:%4d | IDLE\n",
                    currentTime, remainingLedTime, remainingCounterTime, remainingAlphabetTime);
      vTaskDelay(QUANTUM);
    }

    currentTime += QUANTUM_TICKS;
  }
}

// -------------------------------------------------------------------
// Setup & Loop
// -------------------------------------------------------------------

/**
 * @brief Initializes hardware, LCD, I2C, and creates FreeRTOS tasks.
 */
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  Serial.println("\n=== Starting SRTF Scheduler ===");

  xTaskCreatePinnedToCore(ledTask, "LED", 2048, NULL, 1, &ledTaskHandle, 0);
  xTaskCreatePinnedToCore(counterTask, "Counter", 4096, NULL, 1, &counterTaskHandle, 0);
  xTaskCreatePinnedToCore(alphaTask, "Alphabet", 2048, NULL, 1, &alphaTaskHandle, 0);
  xTaskCreatePinnedToCore(schedulerTask, "Scheduler", 4096, NULL, 2, &schedulerHandle, 0);
}

/**
 * @brief Empty loop as all work is handled by FreeRTOS tasks.
 */
void loop() {}