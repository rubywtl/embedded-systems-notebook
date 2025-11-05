// Task Table Scheduler with TCB
// Description: Round robin scheduler with TCB, running two tasks sequentially
//              (and each task handles their own delay)

// --------------------------------------------------------
// INCLUDE & MACROS
// --------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"
#include "soc/timer_group_reg.h"

// Define GPIO pin number
#define GPIO_PIN 5
#define GPIO_PIN2 6

// Define toggle interval in timer ticks (e.g., 1 second)
#define BLINK_INTERVAL_1 500000
#define BLINK_INTERVAL_2 1000000

// Define TIMER_INCREMENT_MODE and TIMER_ENABLE macros
#define TIMER_INCREMENT_MODE (1<<30)
#define TIMER_ENABLE (1<<31)
#define TIME_DIVIDER_VAL 80

// Tasks initialization
#define MAX_TASKS 5
#define NUM_TASKS 2
// --------------------------------------------------------
// GLOBAL VARIABLES
// --------------------------------------------------------

// Defining the states of a task
typedef enum{
  READY,
  RUNNING,
  WAITING
} TaskState;

// Define Task Control Block (TCB) structure
typedef struct {
    int task_id;
    TaskState state;
    int priority;
    void (*task_function)(void);  // Pointer to the task's function
    char task_name[20];
} TCB;

// global task list
TCB task_list[MAX_TASKS];

// --------------------------------------------------------
// FUCNTIONS
// --------------------------------------------------------

// Name: Task1
// Description: blink one led at the frequency of ur choosing
void Task1(void){
  static uint32_t last_toggle_time = 0;

  uint32_t current_time = 0;
  current_time = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  //printf("current time: %d, last toggle time: %d", current_time, last_toggle_time);
  // --- Turn the LED ON --- //
  if((current_time - last_toggle_time) >= BLINK_INTERVAL_1){
    uint32_t gpio_out = 0;
    gpio_out = *((volatile uint32_t *)GPIO_OUT_REG);

    // Toggle GPIO_PIN using XOR
    *((volatile uint32_t *)GPIO_OUT_REG) = gpio_out ^ (1 << GPIO_PIN);

    // Update last_toggle_time
    last_toggle_time = current_time;
  }
  // Refresh timer counter value
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
}

// Name: Task2
// Description: Blink a second LED at a different frequency
void Task2(void){
  static uint32_t last_toggle_time = 0;

  uint32_t current_time = 0;
  current_time = *((volatile uint32_t *)TIMG_T0LO_REG(0));

  // --- Turn the LED ON --- //
  if((current_time - last_toggle_time) >= BLINK_INTERVAL_2){
    uint32_t gpio_out = 0;
    gpio_out = *((volatile uint32_t *)GPIO_OUT_REG);

    // Toggle GPIO_PIN using XOR
    *((volatile uint32_t *)GPIO_OUT_REG) = gpio_out ^ (1 << GPIO_PIN2);

    // Update last_toggle_time
    last_toggle_time = current_time;
  }
  // Refresh timer counter value
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
}

// Name: init_tasks
// Description: Initialize task 1 and task 2
void init_tasks(void) {
    task_list[0] = (TCB){ .task_id = 1, .state = READY, .priority = 1, .task_function = Task1, .task_name = "Task_1" };
    task_list[1] = (TCB){ .task_id = 2, .state = READY, .priority = 2, .task_function = Task2, .task_name = "Task_2" };
}

// Name: Scheduler
// Description: Simple round-robin scheduler simulation
// - check if it is ready first, and then start running it
// - after executing it, set it back to ready state
void scheduler(void) {
    for (int i = 0; i < NUM_TASKS; i++) {
        if (task_list[i].state == READY) {
            task_list[i].state = RUNNING;
            printf("Running %s...\n", task_list[i].task_name);
            task_list[i].task_function();   // Run the task
            task_list[i].state = READY;     // Back to ready state
        }
    }
}

void setup() {
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN], PIN_FUNC_GPIO);

  // enable GPIO_PIN as output
  *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN);

  // enable GPIO_PIN as output
  *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN2);

  // Configure timer with clock divider
  uint32_t timer_config = (TIME_DIVIDER_VAL) << 13;

  // Set increment mode and enable timer
  timer_config |= TIMER_INCREMENT_MODE;
  timer_config |= TIMER_ENABLE;

  // Write config to timer register
  *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) = timer_config;

  // Trigger a timer update to load settings
  *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;

  init_tasks();
}

void loop() {
  scheduler();
}

// Name: Ruby Lee, Feier Long
// - Task Table Scheduler with TCB + delays
// - Description: Round robin scheduler that only calls the tasks when time is up

// // --------------------------------------------------------
// // INCLUDE & MACROS
// // --------------------------------------------------------
// #include <stdio.h>
// #include <string.h>
// #include "driver/gpio.h"
// #include "soc/io_mux_reg.h"
// #include "soc/gpio_reg.h"
// #include "soc/gpio_periph.h"
// #include "soc/timer_group_reg.h"

// // Define GPIO pin number
// #define GPIO_PIN 5
// #define GPIO_PIN2 6

// // Define TIMER_INCREMENT_MODE and TIMER_ENABLE macros
// #define TIMER_INCREMENT_MODE (1<<30)
// #define TIMER_ENABLE (1<<31)
// #define TIME_DIVIDER_VAL 80

// // Tasks initialization
// #define MAX_TASKS 5

// // --------------------------------------------------------
// // GLOBAL VARIABLES
// // --------------------------------------------------------
// typedef enum{
//   READY,
//   RUNNING,
//   WAITING
// } TaskState;

// // Define Task Control Block (TCB) structure
// typedef struct {
//     int task_id;
//     TaskState state;
//     int priority;
//     void (*task_function)(void);  // Pointer to the task's function
//     char task_name[20];
//     int delay; // the time delay between each runs
//     uint32_t last_exec_time; // the last time the task got executed
// } TCB;

// // global task list
// TCB all_tasks[MAX_TASKS];
// int num_tasks = 0;

// // --------------------------------------------------------
// // FUCNTIONS
// // --------------------------------------------------------

// // Name: Task1
// // Description: blink one led at the frequency of ur choosing
// void Task1(void){
//     uint32_t gpio_out = 0;
//     gpio_out = *((volatile uint32_t *)GPIO_OUT_REG);

//     // Toggle GPIO_PIN using XOR
//     *((volatile uint32_t *)GPIO_OUT_REG) = gpio_out ^ (1 << GPIO_PIN);
// }

// // Name: Task2
// // Description: Blink a second LED at a different frequency
// void Task2(void){
//     uint32_t gpio_out = 0;
//     gpio_out = *((volatile uint32_t *)GPIO_OUT_REG);

//     // Toggle GPIO_PIN using XOR
//     *((volatile uint32_t *)GPIO_OUT_REG) = gpio_out ^ (1 << GPIO_PIN2);
// }

// // Initialize tasks
// void init_tasks(void) {
//     all_tasks[0] = (TCB){ .task_id = 1, .state = READY, .priority = 1, .task_function = Task1, .task_name = "Task_1", .delay = 250000, .last_exec_time = *((volatile uint32_t *)TIMG_T0LO_REG(0)) };
//     all_tasks[1] = (TCB){ .task_id = 2, .state = READY, .priority = 2, .task_function = Task2, .task_name = "Task_2", .delay = 500000, .last_exec_time = *((volatile uint32_t *)TIMG_T0LO_REG(0)) };
//     num_tasks = 2;
// }

// // Simple round-robin scheduler simulation
// void scheduler(void) {
//     for (int i = 0; i < num_tasks; i++) {
//         TCB *t = &all_tasks[i];

//         // get current time
//         *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
//         uint32_t current_time = 0;
//         current_time = *((volatile uint32_t *)TIMG_T0LO_REG(0));

//         // check if the wait time is up yet for this task
//         if (current_time - all_tasks[i].last_exec_time >= (all_tasks[i].delay)) {
//           // run task
//           t->state = RUNNING;
//           printf("[Scheduler] Running %s\n", t->task_name);
//           t->task_function();

//           // update the exec time after executing it
//           *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;
//           t->last_exec_time = *((volatile uint32_t *)TIMG_T0LO_REG(0));
          
//           // set the state back to ready again
//           t->state = READY;
//         }
//     }
// }

// void setup() {
//   PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_PIN], PIN_FUNC_GPIO);

//   // enable GPIO_PIN as output
//   *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN);

//   // enable GPIO_PIN as output
//   *((volatile uint32_t *)GPIO_ENABLE_REG) |= (1 << GPIO_PIN2);

//   // Configure timer with clock divider
//   uint32_t timer_config = (TIME_DIVIDER_VAL) << 13;

//   // Set increment mode and enable timer
//   timer_config |= TIMER_INCREMENT_MODE;
//   timer_config |= TIMER_ENABLE;

//   // Write config to timer register
//   *((volatile uint32_t *)TIMG_T0CONFIG_REG(0)) = timer_config;

//   // Trigger a timer update to load settings
//   *((volatile uint32_t *)TIMG_T0UPDATE_REG(0)) = 1;

//   init_tasks();
// }

// void loop() {
//     scheduler();
// }

