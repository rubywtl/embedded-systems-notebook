# Embedded-Systems Notebook 🚀

## Content

### Schedulers ⏱️
How tasks are scheduled and selected to run next.
- **Round-Robin Scheduler:** Each task gets equal time in rotation. The order is determined by the initially assigned priorities.
- **Shortest-Remaining-Time (SRT):** The task with the shortest remaining time runs first. A running task can be preempted if a shorter task enters the queue.

### Interrupts ⚡
Temporarily halt the current task to quickly handle a high-priority event.
- **Examples:** Button interrupt, Bluetooth signal interrupt.

### FreeRTOS 🧵
A lightweight real-time OS used to create and manage tasks on bare metal.
- Task creation and scheduling  
- Dual-core task execution  
- Delays and timing  
- Semaphores, mutexes, queues for synchronization  
- Task Notifications: Lightweight, fast signaling between tasks or from ISRs to tasks. Often used instead of semaphores for simple one-to-one event or data signaling.

### Hardware Components 🔧
Examples of basic peripheral control:
- LEDs with GPIO  
- Buzzers with PWM  
- Analog input components  
- LCD communication over serial interfaces
