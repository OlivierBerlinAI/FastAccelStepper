/*
 * FluidTest - Queue Instructions Test with Traced Path Data
 *
 * This example demonstrates using FastAccelStepper queue instructions
 * with traced path data for synchronized dual-stepper control.
 *
 * Tested on: ESP32
 */

#include "FastAccelStepper.h"

// Pin definitions for ESP32
// Left stepper
#define LEFT_STEP_PIN    26
#define LEFT_DIR_PIN     25
#define LEFT_ENABLE_PIN  27  // Optional, set to -1 if not used

// Right stepper
#define RIGHT_STEP_PIN   14
#define RIGHT_DIR_PIN    12
#define RIGHT_ENABLE_PIN 13  // Optional, set to -1 if not used

// Create engine and stepper objects
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *leftStepper = NULL;
FastAccelStepper *rightStepper = NULL;

// Traced path data structure
struct PathData {
  int16_t leftSteps;
  uint16_t leftTicks;
  int16_t rightSteps;
  uint16_t rightTicks;
};

// Your traced path data - expand this array with more samples as needed
const PathData tracedPath[] = {
  {-20, 153, 26, 198},
  {-21, 209, 25, 249},
  {-21, 248, 25, 295},
  {-20, 268, 25, 335},
  {-21, 311, 26, 385},
  // Add more path data here as needed
};

const int pathLength = sizeof(tracedPath) / sizeof(tracedPath[0]);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("FluidTest - Queue Instructions Test");
  Serial.println("====================================");

  // Initialize the stepper engine
  engine.init();

  // Connect left stepper
  Serial.print("Connecting left stepper to pin ");
  Serial.println(LEFT_STEP_PIN);
  leftStepper = engine.stepperConnectToPin(LEFT_STEP_PIN);
  if (leftStepper) {
    leftStepper->setDirectionPin(LEFT_DIR_PIN);
    if (LEFT_ENABLE_PIN >= 0) {
      leftStepper->setEnablePin(LEFT_ENABLE_PIN);
      leftStepper->setAutoEnable(true);
    }
    Serial.println("Left stepper initialized successfully");
  } else {
    Serial.println("ERROR: Failed to initialize left stepper!");
    while(1);
  }

  // Connect right stepper
  Serial.print("Connecting right stepper to pin ");
  Serial.println(RIGHT_STEP_PIN);
  rightStepper = engine.stepperConnectToPin(RIGHT_STEP_PIN);
  if (rightStepper) {
    rightStepper->setDirectionPin(RIGHT_DIR_PIN);
    if (RIGHT_ENABLE_PIN >= 0) {
      rightStepper->setEnablePin(RIGHT_ENABLE_PIN);
      rightStepper->setAutoEnable(true);
    }
    Serial.println("Right stepper initialized successfully");
  } else {
    Serial.println("ERROR: Failed to initialize right stepper!");
    while(1);
  }

  Serial.println("\nSystem Information:");
  Serial.print("TICKS_PER_S: ");
  Serial.println(TICKS_PER_S);
  Serial.print("MIN_CMD_TICKS: ");
  Serial.println(MIN_CMD_TICKS);
  Serial.print("Path length: ");
  Serial.print(pathLength);
  Serial.println(" segments");

  Serial.println("\n====================================");
  Serial.println("Press any key to start the path execution...");

  // Wait for user input
  while (!Serial.available()) {
    delay(100);
  }

  // Clear the serial buffer
  while (Serial.available()) {
    Serial.read();
  }

  // Execute the traced path
  executeTracedPath();
}

void executeTracedPath() {
  Serial.println("\nStarting path execution...");
  Serial.println("====================================");

  // Display path data
  Serial.println("Path Data:");
  Serial.println("Seg | Left Steps | Left Ticks | Right Steps | Right Ticks");
  Serial.println("----|------------|------------|-------------|------------");
  for (int i = 0; i < pathLength; i++) {
    Serial.print(i);
    Serial.print("   | ");
    Serial.print(tracedPath[i].leftSteps);
    Serial.print("         | ");
    Serial.print(tracedPath[i].leftTicks);
    Serial.print("        | ");
    Serial.print(tracedPath[i].rightSteps);
    Serial.print("          | ");
    Serial.println(tracedPath[i].rightTicks);
  }
  Serial.println();

  // Add all path segments to the queues
  Serial.println("Adding queue entries...");

  for (int i = 0; i < pathLength; i++) {
    // Prepare left stepper command
    struct stepper_command_s leftCmd = {
      .ticks = tracedPath[i].leftTicks,
      .steps = (uint8_t)abs(tracedPath[i].leftSteps),
      .count_up = tracedPath[i].leftSteps >= 0
    };

    // Prepare right stepper command
    struct stepper_command_s rightCmd = {
      .ticks = tracedPath[i].rightTicks,
      .steps = (uint8_t)abs(tracedPath[i].rightSteps),
      .count_up = tracedPath[i].rightSteps >= 0
    };

    // Add left stepper command with retry logic
    bool leftAdded = false;
    while (!leftAdded) {
      AqeResultCode rc = leftStepper->addQueueEntry(&leftCmd, false);
      if (aqeIsOk(rc)) {
        leftAdded = true;
      } else if (aqeRetry(rc)) {
        // Queue full, wait a bit and retry
        delayMicroseconds(100);
      } else {
        // Fatal error
        Serial.print("ERROR adding left stepper segment ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(toString(rc));
        return;
      }
    }

    // Add right stepper command with retry logic
    bool rightAdded = false;
    while (!rightAdded) {
      AqeResultCode rc = rightStepper->addQueueEntry(&rightCmd, false);
      if (aqeIsOk(rc)) {
        rightAdded = true;
      } else if (aqeRetry(rc)) {
        // Queue full, wait a bit and retry
        delayMicroseconds(100);
      } else {
        // Fatal error
        Serial.print("ERROR adding right stepper segment ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(toString(rc));
        return;
      }
    }

    Serial.print("Added segment ");
    Serial.print(i);
    Serial.print(" - Left: ");
    Serial.print(tracedPath[i].leftSteps);
    Serial.print(" steps @ ");
    Serial.print(tracedPath[i].leftTicks);
    Serial.print(" ticks, Right: ");
    Serial.print(tracedPath[i].rightSteps);
    Serial.print(" steps @ ");
    Serial.print(tracedPath[i].rightTicks);
    Serial.println(" ticks");
  }

  Serial.println("\nAll queue entries added successfully!");
  Serial.println("Starting synchronized execution...\n");

  // Start both steppers
  leftStepper->startQueue();
  rightStepper->startQueue();

  // Monitor execution
  unsigned long startTime = millis();
  bool leftRunning = true;
  bool rightRunning = true;

  while (leftRunning || rightRunning) {
    leftRunning = leftStepper->isQueueRunning();
    rightRunning = rightStepper->isQueueRunning();

    // Print status every 100ms
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 100) {
      lastPrint = millis();

      Serial.print("Time: ");
      Serial.print(millis() - startTime);
      Serial.print(" ms | Left: ");
      Serial.print(leftStepper->getCurrentPosition());
      Serial.print(" (");
      Serial.print(leftStepper->queueEntries());
      Serial.print(" in queue) | Right: ");
      Serial.print(rightStepper->getCurrentPosition());
      Serial.print(" (");
      Serial.print(rightStepper->queueEntries());
      Serial.println(" in queue)");
    }

    delay(10);
  }

  unsigned long executionTime = millis() - startTime;

  Serial.println("\n====================================");
  Serial.println("Path execution completed!");
  Serial.println("====================================");
  Serial.print("Total execution time: ");
  Serial.print(executionTime);
  Serial.println(" ms");
  Serial.print("Final left position: ");
  Serial.println(leftStepper->getCurrentPosition());
  Serial.print("Final right position: ");
  Serial.println(rightStepper->getCurrentPosition());

  // Calculate expected positions
  int32_t expectedLeft = 0;
  int32_t expectedRight = 0;
  for (int i = 0; i < pathLength; i++) {
    expectedLeft += tracedPath[i].leftSteps;
    expectedRight += tracedPath[i].rightSteps;
  }

  Serial.print("Expected left position: ");
  Serial.println(expectedLeft);
  Serial.print("Expected right position: ");
  Serial.println(expectedRight);

  if (leftStepper->getCurrentPosition() == expectedLeft &&
      rightStepper->getCurrentPosition() == expectedRight) {
    Serial.println("\nSUCCESS: Positions match expected values!");
  } else {
    Serial.println("\nWARNING: Position mismatch detected!");
  }
}

void loop() {
  // Path execution is done in setup()
  // You can add code here to repeat the path or do other tasks
  delay(1000);
}
