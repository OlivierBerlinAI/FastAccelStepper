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

// Right stepper
#define RIGHT_STEP_PIN 4
#define RIGHT_DIR_PIN 2
#define RIGHT_ENABLE_PIN 15  // Optional, set to -1 if not used

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

  // Connect right stepper
  Serial.print("Connecting right stepper to pin ");
  Serial.println(RIGHT_STEP_PIN);
  rightStepper = engine.stepperConnectToPin(RIGHT_STEP_PIN);
  if (rightStepper) {
    rightStepper->setDirectionPin(RIGHT_DIR_PIN);
    if (RIGHT_ENABLE_PIN >= 0) {
      rightStepper->setEnablePin(RIGHT_ENABLE_PIN, LOW_ACTIVE_ENABLE);
      rightStepper->setAutoEnable(true);
      rightStepper->enableOutputs();
    }
    Serial.println("Right stepper initialized successfully");
  } else {
    Serial.println("ERROR: Failed to initialize right stepper!");
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

  /*   // Display path data
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
    Serial.println(); */

  // Add path segments to the queues
  Serial.println("Adding queue entries...");
  Serial.print("Total segments: ");
  Serial.println(pathLength);
  Serial.print("Queue capacity: 32 entries per stepper");
  Serial.println();

  bool queuesStarted = false;

  for (int i = 0; i < pathLength; i++) {
    // Calculate adjusted ticks with multiplier
    uint32_t leftTicksAdjusted =
        (uint32_t)tracedPath[i].leftTicks * TICK_MULTIPLIER;
    uint32_t rightTicksAdjusted =
        (uint32_t)tracedPath[i].rightTicks * TICK_MULTIPLIER;
    uint8_t leftSteps = (uint8_t)abs(tracedPath[i].leftSteps);
    uint8_t rightSteps = (uint8_t)abs(tracedPath[i].rightSteps);

    // Validate against MIN_CMD_TICKS
    uint32_t leftTotalTicks = leftTicksAdjusted * leftSteps;
    uint32_t rightTotalTicks = rightTicksAdjusted * rightSteps;

    if (leftTotalTicks < MIN_CMD_TICKS) {
      Serial.print("WARNING: Segment ");
      Serial.print(i);
      Serial.print(" left total ticks (");
      Serial.print(leftTotalTicks);
      Serial.print(") < MIN_CMD_TICKS (");
      Serial.print(MIN_CMD_TICKS);
      Serial.println("). Command may be rejected!");
    }

    if (rightTotalTicks < MIN_CMD_TICKS) {
      Serial.print("WARNING: Segment ");
      Serial.print(i);
      Serial.print(" right total ticks (");
      Serial.print(rightTotalTicks);
      Serial.print(") < MIN_CMD_TICKS (");
      Serial.print(MIN_CMD_TICKS);
      Serial.println("). Command may be rejected!");
    }

    // Cap ticks at uint16_t max
    if (leftTicksAdjusted > 65535) {
      Serial.print("WARNING: Segment ");
      Serial.print(i);
      Serial.print(" left ticks capped at 65535 (was ");
      Serial.print(leftTicksAdjusted);
      Serial.println(")");
      leftTicksAdjusted = 65535;
    }

    if (rightTicksAdjusted > 65535) {
      Serial.print("WARNING: Segment ");
      Serial.print(i);
      Serial.print(" right ticks capped at 65535 (was ");
      Serial.print(rightTicksAdjusted);
      Serial.println(")");
      rightTicksAdjusted = 65535;
    }

    // Prepare left stepper command
    struct stepper_command_s leftCmd = {
        .ticks = (uint16_t)leftTicksAdjusted,
        .steps = leftSteps,
        .count_up = tracedPath[i].leftSteps >= 0};

    // Prepare right stepper command
    struct stepper_command_s rightCmd = {
        .ticks = (uint16_t)rightTicksAdjusted,
        .steps = rightSteps,
        .count_up = tracedPath[i].rightSteps >= 0};

    // Add left stepper command - wait and retry if queue full
    bool leftAdded = false;
    int leftRetries = 0;
    while (!leftAdded) {
      AqeResultCode rc = leftStepper->addQueueEntry(&leftCmd, false);
      if (aqeIsOk(rc)) {
        leftAdded = true;
      } else if (aqeRetry(rc)) {
        // Queue full, wait for it to drain a bit
        leftRetries++;
        if (leftRetries > 5000) {
          Serial.print("\nERROR: Left queue stuck at segment ");
          Serial.println(i);
          Serial.print("Return code: ");
          Serial.println(toString(rc));
          Serial.print("Queue entries: ");
          Serial.println(leftStepper->queueEntries());
          Serial.print("Queue running: ");
          Serial.println(leftStepper->isQueueRunning());
          Serial.print("Queue empty: ");
          Serial.println(leftStepper->isQueueEmpty());
          return;
        }
        delay(1);
      } else {
        // Fatal error
        Serial.print("ERROR adding left stepper segment ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(toString(rc));
        Serial.print("Left total ticks: ");
        Serial.println(leftTotalTicks);
        return;
      }
    }

    // Add right stepper command - wait and retry if queue full
    bool rightAdded = false;
    int rightRetries = 0;
    while (!rightAdded) {
      AqeResultCode rc = rightStepper->addQueueEntry(&rightCmd, false);
      if (aqeIsOk(rc)) {
        rightAdded = true;
      } else if (aqeRetry(rc)) {
        // Queue full, wait for it to drain a bit
        rightRetries++;
        if (rightRetries > 5000) {
          Serial.print("\nERROR: Right queue stuck at segment ");
          Serial.println(i);
          Serial.print("Return code: ");
          Serial.println(toString(rc));
          Serial.print("Queue entries: ");
          Serial.println(rightStepper->queueEntries());
          Serial.print("Queue running: ");
          Serial.println(rightStepper->isQueueRunning());
          Serial.print("Queue empty: ");
          Serial.println(rightStepper->isQueueEmpty());
          return;
        }
        delay(1);
      } else {
        // Fatal error
        Serial.print("ERROR adding right stepper segment ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(toString(rc));
        Serial.print("Right total ticks: ");
        Serial.println(rightTotalTicks);
        return;
      }
    }

    // Print progress every 50 segments to avoid serial spam
    if (i % 50 == 0 || i == pathLength - 1) {
      Serial.print("Progress: ");
      Serial.print(i + 1);
      Serial.print("/");
      Serial.print(pathLength);
      Serial.print(" segments added (");
      Serial.print((i + 1) * 100 / pathLength);
      Serial.print("%) - Queue sizes: L=");
      Serial.print(leftStepper->queueEntries());
      Serial.print(" R=");
      Serial.println(rightStepper->queueEntries());
    }

    // Start queues after adding some entries so they can process while we add
    // more This prevents the queue from filling up completely
    if (!queuesStarted && (i >= 15 || i == pathLength - 1)) {
      Serial.println("\n>>> Starting queues to begin execution...");
      AqeResultCode leftStartResult = leftStepper->addQueueEntry(NULL, true);
      AqeResultCode rightStartResult = rightStepper->addQueueEntry(NULL, true);

      Serial.print("Left start result: ");
      Serial.println(toString(leftStartResult));
      Serial.print("Right start result: ");
      Serial.println(toString(rightStartResult));

      if (!aqeIsOk(leftStartResult) || !aqeIsOk(rightStartResult)) {
        Serial.println("ERROR: Failed to start queues!");
        return;
      }

      queuesStarted = true;
      Serial.println(">>> Queues started! Continuing to add entries...\n");
    }
  }

  Serial.println("\n====================================");
  Serial.println("All queue entries added successfully!");
  Serial.print("Left queue has ");
  Serial.print(leftStepper->queueEntries());
  Serial.println(" entries");
  Serial.print("Right queue has ");
  Serial.print(rightStepper->queueEntries());
  Serial.println(" entries");
  Serial.println("====================================");

  if (!queuesStarted) {
    // If we have 15 or fewer segments and haven't started yet, start now
    Serial.println("Starting queues now...");
    AqeResultCode leftStartResult = leftStepper->addQueueEntry(NULL, true);
    AqeResultCode rightStartResult = rightStepper->addQueueEntry(NULL, true);

    Serial.print("Left start result: ");
    Serial.println(toString(leftStartResult));
    Serial.print("Right start result: ");
    Serial.println(toString(rightStartResult));

    if (!aqeIsOk(leftStartResult) || !aqeIsOk(rightStartResult)) {
      Serial.println("ERROR: Failed to start queues!");
      return;
    }
    queuesStarted = true;
  }

  Serial.println("Monitoring execution...\n");

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
