# FreeRTOS Architecture Documentation

## System Overview
This FreeRTOS implementation consists of the following six tasks:
1. **serialTask**: Handles serial communication with connected devices.
2. **mqttTask**: Manages the MQTT protocol for IoT communications.
3. **apiTask**: Interfaces with external APIs for data fetching and actions.
4. **relayTask**: Controls relays based on incoming commands.
5. **rgbLedTask**: Manages RGB LED states and effects.
6. **watchdogTask**: Implements a watchdog timer to ensure system reliability.

## Command Queue Structure
The command queue is structured to allow efficient communication between tasks. Each task can send and receive commands, ensuring a responsive and scalable architecture.

## Task Priorities and Stack Sizes
- **serialTask**: Priority 2, Stack Size: 150
- **mqttTask**: Priority 3, Stack Size: 200
- **apiTask**: Priority 1, Stack Size: 100
- **relayTask**: Priority 4, Stack Size: 150
- **rgbLedTask**: Priority 2, Stack Size: 120
- **watchdogTask**: Priority 5, Stack Size: 100

## Architecture Diagram
![FreeRTOS Architecture Diagram](link_to_architecture_diagram)

## File Structure
- `/src`: Source code for tasks and utilities.
- `/include`: Header files for tasks and common definitions.
- `/docs`: Documentation and architecture overviews.

## Memory Usage Statistics
Memory usage is optimized to ensure that all tasks operate smoothly within the available resources. Detailed statistics can be found in the `memory_usage.txt` file.

## Migration Notes from Classic Version
- Tasks have been refactored to leverage FreeRTOS features for better performance.
- Improved task communication through integrated queues.
- Migration from static memory allocation to dynamic allocation for flexibility.

## Benefits of FreeRTOS Implementation
- Improved task management and scheduling.
- Better resource utilization through dynamic allocation.
- Enhanced system stability and reliability with the watchdog task.

## Debugging Guide
- Use the built-in FreeRTOS debugging tools to monitor task states.
- Log data to monitor command queues and received messages.
- Analyze task stack usage and task states through the FreeRTOS monitoring system.

## Changelog
### Version v1.1-rtos
- Integrated FreeRTOS with improved task handling.
- Added watchdog implementation for reliability.
- Refactored existing tasks to comply with FreeRTOS usage. 

## Future Improvements
- Explore Advanced features in FreeRTOS to enhance the capabilities of tasks.
- Optimize memory management practices further for critical tasks.