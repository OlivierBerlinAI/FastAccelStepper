/*
 * FluidTest - Queue Instructions Test with Traced Path Data
 *
 * This example demonstrates using FastAccelStepper queue instructions
 * with traced path data for synchronized dual-stepper control.
 *
 * Tested on: ESP32
 */

#include "FastAccelStepper.h"
#include "PathData.h"

// Pin definitions for ESP32
// Left stepper
#define LEFT_STEP_PIN 19
#define LEFT_DIR_PIN 18
#define LEFT_ENABLE_PIN 23  // Optional, set to -1 if not used

#define MOTOR_MS1 22
#define MOTOR_MS2 21

// Enable pin logic - set to true if your driver needs LOW to enable (most
// common) Most stepper drivers (A4988, DRV8825, TMC2208) use LOW to enable Set
// to false if your driver needs HIGH to enable (less common)
#define LOW_ACTIVE_ENABLE true

// Tick multiplier - increase this if your ticks are too low
// ESP32 MIN_CMD_TICKS = 3200, so multiply ticks to meet this requirement
#define TICK_MULTIPLIER 1

// Create engine and stepper objects
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper* leftStepper = NULL;
FastAccelStepper* rightStepper = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Press any key to start the execution...");

  // Wait for user input
  while (!Serial.available()) {
    delay(100);
  }

  // Clear the serial buffer
  while (Serial.available()) {
    Serial.read();
  }

  Serial.println("FluidTest - Queue Instructions Test");
  Serial.println("====================================");

  pinMode(LEFT_ENABLE_PIN, OUTPUT);
  pinMode(MOTOR_MS1, OUTPUT);
  pinMode(MOTOR_MS2, OUTPUT);

  // MOTOR_ENABLED LOW = Motor on
  digitalWrite(LEFT_ENABLE_PIN, LOW);

  // Initialize the stepper engine
  engine.init();

  // Connect left stepper
  Serial.print("Connecting left stepper to pin ");
  Serial.println(LEFT_STEP_PIN);
  leftStepper = engine.stepperConnectToPin(LEFT_STEP_PIN);
  if (leftStepper) {
    leftStepper->setDirectionPin(LEFT_DIR_PIN);
    if (LEFT_ENABLE_PIN >= 0) {
      leftStepper->setEnablePin(LEFT_ENABLE_PIN, LOW_ACTIVE_ENABLE);
      leftStepper->setAutoEnable(true);
      leftStepper->enableOutputs();
    }
    Serial.println("Left stepper initialized successfully");
  } else {
    Serial.println("ERROR: Failed to initialize left stepper!");
    while (1);
  }

  Serial.println("\nSystem Information:");
  Serial.print("TICKS_PER_S: ");
  Serial.println(TICKS_PER_S);
  Serial.print("MIN_CMD_TICKS: ");
  Serial.println(MIN_CMD_TICKS);
  Serial.print("TICK_MULTIPLIER: ");
  Serial.println(TICK_MULTIPLIER);
  Serial.print("Enable pin logic: ");
  Serial.println(LOW_ACTIVE_ENABLE ? "LOW to enable (common)"
                                   : "HIGH to enable");
  Serial.print("Path length: ");
  Serial.print(pathLength);
  Serial.println(" segments");

  Serial.println("\n====================================");
}

uint32_t currentPathIndex = 0;
bool wasPathStarted = false;

void addCommandsToQueue(int numberOfCommandsToAdd) {
  uint32_t previousPathIndex = currentPathIndex;

  while (true) {
    if (currentPathIndex - previousPathIndex >= numberOfCommandsToAdd) {
      Serial.println("Added enough commands");
      return;
    }

    if (currentPathIndex >= pathLength) {
      Serial.println("Stopped adding elements early, no elements left");

      return;
    }

    // Prepare left stepper command
    struct stepper_command_s leftCmd = {
        .ticks = (uint16_t)tracedPath[currentPathIndex].leftTicks,
        .steps = tracedPath[currentPathIndex].leftSteps,
        .count_up = tracedPath[currentPathIndex].leftSteps >= 0};

    AqeResultCode rc = leftStepper->addQueueEntry(&leftCmd, false);
    if (!aqeIsOk(rc)) {
      Serial.println("Unable to add command to queue");
      return;
    } else {
      Serial.print("Added currentPathIndex ");
      Serial.print(currentPathIndex);
      Serial.print(" with ticks ");
      Serial.print((uint16_t)tracedPath[currentPathIndex].leftTicks);
      Serial.print(" with steps ");
      Serial.println(tracedPath[currentPathIndex].leftSteps);
    }

    currentPathIndex++;
  }
}

uint8_t prevQueueCount = 0;
ulong prevMillis = 0;
ulong currentMillis = 0;
ulong lastStallCheck = 0;

void loop() {
  if (!wasPathStarted) {
    addCommandsToQueue(30);
    leftStepper->addQueueEntry(NULL, true);
    wasPathStarted = true;
    prevMillis = millis();

    Serial.println("Started left stepper");
  }

  // Check for queue stalling every 100ms
  currentMillis = millis();
  if (currentMillis - lastStallCheck >= 100) {
    lastStallCheck = currentMillis;
    if (!leftStepper->isQueueEmpty() && !leftStepper->isQueueRunning()) {
      Serial.println("WARNING: Queue stalled, restarting...");
      leftStepper->addQueueEntry(NULL, true);  // Restart queue
    }
  }

  if (leftStepper->queueEntries() < 20 && currentPathIndex < pathLength) {
    Serial.print("Was at ");
    Serial.print(leftStepper->queueEntries());
    Serial.print(" queueEntries and at currentPathIndex ");
    Serial.println(currentPathIndex);
    addCommandsToQueue(10);
    Serial.print("Added 10 more commands to queue, now at ");
    Serial.println(leftStepper->queueEntries());
  }

  uint8_t queueEntries = leftStepper->queueEntries();
  if (prevQueueCount != queueEntries) {
    prevQueueCount = queueEntries;

    Serial.print("Queue is now at ");
    Serial.print(queueEntries);
    Serial.print(" at ");
    Serial.print(currentMillis);
    Serial.print(" with DT ");
    Serial.print(currentMillis - prevMillis);
    Serial.println(" ms");

    prevMillis = currentMillis;
  }

  if (currentPathIndex >= pathLength && queueEntries == 0) {
    // Wait for user input
    while (!Serial.available()) {
      delay(100);
    }

    // Clear the serial buffer
    while (Serial.available()) {
      Serial.read();
    }

    wasPathStarted = false;
    currentPathIndex = 0;
    prevMillis = millis();
  }

  // delayMicroseconds(10);
}
