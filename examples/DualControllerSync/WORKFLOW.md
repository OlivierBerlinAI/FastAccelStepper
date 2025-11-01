# Complete Workflow Guide

This guide walks through the complete process of using DualControllerSync with externally-generated motion data.

## Overview

```
┌──────────────────────────────────────────────────────────┐
│ External Motion Planning Software (Your Software)       │
│ - Calculates trajectories, velocities, accelerations    │
│ - Outputs command list in proper format                 │
└────────────────────┬─────────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────────┐
│ Command List Format Validation                           │
│ - Verify timing constraints                              │
│ - Check step ranges                                      │
│ - Validate ticks × steps ≥ MIN_CMD_TICKS                │
└────────────────────┬─────────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────────┐
│ Generate C++ Header or Binary File                       │
│ - motion_data.h (C++ array) OR                          │
│ - motion_data.bin (binary file for SD card)             │
└────────────────────┬─────────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────────┐
│ Flash to Both ESP32 Controllers                          │
│ - ESP32 #1: Set MOTOR_SIDE = MOTOR_LEFT                 │
│ - ESP32 #2: Set MOTOR_SIDE = MOTOR_RIGHT                │
│ - Same command list on both                              │
└────────────────────┬─────────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────────┐
│ Hardware Setup                                            │
│ - Connect sync pins together                             │
│ - Connect motors to respective drivers                   │
│ - Power on both controllers                              │
└────────────────────┬─────────────────────────────────────┘
                     │
                     ▼
┌──────────────────────────────────────────────────────────┐
│ Synchronized Execution                                    │
│ - Both wait for SYNC_PIN = HIGH                          │
│ - Pull sync pin HIGH to start                            │
│ - Both execute their motor's commands                    │
│ - Monitor via serial output                              │
└──────────────────────────────────────────────────────────┘
```

## Step-by-Step Instructions

### Step 1: Generate Motion Data in Your Software

Your external motion planning software should output data in this format:

```cpp
struct MotionCommand {
  int16_t steps_left;      // -32767 to +32767
  int16_t steps_right;     // -32767 to +32767
  uint32_t duration_ticks; // Timer ticks (ESP32: 16MHz = 16,000,000 ticks/sec)
  uint32_t duration_us;    // Microseconds (for reference/debugging)
};
```

**Constraints to enforce:**
- `duration_ticks ≥ 3200` (200µs minimum for ESP32)
- `duration_us = duration_ticks / 16` (for ESP32)
- For each motor: if `steps ≠ 0`, then `(duration_ticks / |steps|) × |steps| ≥ 3200`

### Step 2: Export to C++ Header or Binary

#### Option A: C++ Header (for small command lists)

Use the provided Python script:

```bash
cd examples/DualControllerSync
python generate_motion.py --output-format cpp --pattern example > motion_data.h
```

Or create manually:

```cpp
// motion_data.h
#ifndef MOTION_DATA_H
#define MOTION_DATA_H

struct MotionCommand {
  int16_t steps_left;
  int16_t steps_right;
  uint32_t duration_ticks;
  uint32_t duration_us;
};

const uint16_t COMMAND_COUNT = 3;

const MotionCommand motionCommands[] = {
  // steps_left, steps_right, duration_ticks, duration_us
  {   100,   50,    160000,   10000 },
  {    80,   80,    128000,    8000 },
  {  -100,  -50,    160000,   10000 },
};

#endif
```

#### Option B: Binary File (for large command lists via SD card)

```bash
python generate_motion.py --output-format bin > motion_data.bin
```

Binary format:
```
Header (12 bytes):
  uint32_t magic = 0x4D534146 ("FASM")
  uint16_t version = 1
  uint16_t motor_count = 2
  uint32_t command_count

Commands (12 bytes each):
  int16_t steps_left
  int16_t steps_right
  uint32_t duration_ticks
  uint32_t duration_us
```

### Step 3: Integrate with Arduino Sketch

#### For C++ Header:

```cpp
// In DualControllerSync.ino, replace the example commands with:
#include "motion_data.h"
```

#### For Binary File (SD Card):

```cpp
#include <SD.h>

const int SD_CS_PIN = 5;  // SD card chip select pin
MotionCommand motionCommands[MAX_COMMANDS];
uint16_t COMMAND_COUNT = 0;

void loadCommandsFromSD() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }

  File file = SD.open("/motion_data.bin", FILE_READ);
  if (!file) {
    Serial.println("Failed to open motion_data.bin");
    return;
  }

  // Read header
  uint32_t magic;
  uint16_t version, motor_count;
  uint32_t cmd_count;

  file.read((uint8_t*)&magic, 4);
  file.read((uint8_t*)&version, 2);
  file.read((uint8_t*)&motor_count, 2);
  file.read((uint8_t*)&cmd_count, 4);

  if (magic != 0x4D534146) {
    Serial.println("Invalid file format!");
    file.close();
    return;
  }

  COMMAND_COUNT = min(cmd_count, MAX_COMMANDS);

  // Read commands
  for (int i = 0; i < COMMAND_COUNT; i++) {
    file.read((uint8_t*)&motionCommands[i], sizeof(MotionCommand));
  }

  file.close();
  Serial.printf("Loaded %u commands from SD card\n", COMMAND_COUNT);
}

void setup() {
  // ... other setup code ...
  loadCommandsFromSD();
  // ... rest of setup ...
}
```

### Step 4: Configure Each Controller

**Controller 1 (LEFT motor):**
```cpp
#define MOTOR_SIDE MOTOR_LEFT
```

**Controller 2 (RIGHT motor):**
```cpp
#define MOTOR_SIDE MOTOR_RIGHT
```

Flash the **same code** to both, just change this one line.

### Step 5: Hardware Wiring

```
LEFT Controller (ESP32 #1):
- GPIO 26 → LEFT Motor Driver STEP
- GPIO 25 → LEFT Motor Driver DIR
- GPIO 27 → LEFT Motor Driver ENABLE
- GPIO 32 → SYNC signal (shared)
- GND → Common ground

RIGHT Controller (ESP32 #2):
- GPIO 26 → RIGHT Motor Driver STEP
- GPIO 25 → RIGHT Motor Driver DIR
- GPIO 27 → RIGHT Motor Driver ENABLE
- GPIO 32 → SYNC signal (shared)
- GND → Common ground

SYNC Signal:
- Pull-down resistor (10kΩ to GND)
- Button or relay to pull HIGH
- When HIGH: both controllers start executing
```

### Step 6: Execution

1. **Power both controllers**
   - Both boot, validate commands, wait for sync

2. **Monitor serial output** (115200 baud):
   ```
   [LEFT] Waiting for SYNC signal...
   ```
   ```
   [RIGHT] Waiting for SYNC signal...
   ```

3. **Trigger start** (pull SYNC_PIN HIGH)
   - Press button / activate relay
   - Both controllers start simultaneously

4. **Watch execution**:
   ```
   [LEFT] Cmd:42/100 Steps:8450 Time:521000us Drift:12 Busy:5 Empty:0 Err:0
   [RIGHT] Cmd:42/100 Steps:4230 Time:521000us Drift:8 Busy:3 Empty:0 Err:0
   ```

5. **Verify completion**:
   ```
   [LEFT] === All commands completed ===
   [LEFT] Total steps: 12500
   [LEFT] Total time: 2500000 us
   [LEFT] Final drift: 5 ticks (0 us)

   [RIGHT] === All commands completed ===
   [RIGHT] Total steps: 6250
   [RIGHT] Total time: 2500000 us
   [RIGHT] Final drift: -3 ticks (0 us)
   ```

   ✅ **Total time should match** on both controllers (synchronized)

## Troubleshooting

### Commands Don't Match Expected Motion

**Check:** Command validation in your external software
- Verify `duration_ticks` calculation: `duration_us × 16`
- Ensure all commands pass validation

### One Motor Doesn't Move

**Check:** `MOTOR_SIDE` configuration
- LEFT controller uses `steps_left`
- RIGHT controller uses `steps_right`
- Print which motor side in setup(): `Serial.printf("Motor: %s\n", getMotorName());`

### Motors Start at Different Times

**Check:** Sync signal wiring
- Both SYNC_PIN connected to same signal?
- Pull-down resistor present?
- Both controllers showing "Waiting for SYNC signal..."?

### Queue Empty Warnings

**Cause:** Commands not added fast enough

**Solutions:**
- Increase command durations (more time per command)
- Reduce loop() overhead
- Remove unnecessary Serial.print() calls in loop
- Pre-fill more commands in setup()

### Large Timing Drift

**Check:** Quantization errors
- Drift of ±10-50 ticks is normal
- Growing drift indicates validation issue
- Verify: `(duration_ticks / steps) × steps ≥ 3200`

## Performance Tips

1. **Command Duration**: Keep in 1-20ms range for best results
2. **Queue Fullness**: Monitor "Busy" count - should be low
3. **Serial Output**: Disable or reduce frequency for faster loop execution
4. **Command Count**: No hard limit, tested with 10,000+ commands
5. **Memory**: Each command = 12 bytes (10,000 commands = 120 KB)

## Integration with Your Software

### Example: Python Motion Planner

```python
import struct

TICKS_PER_SECOND = 16_000_000

def export_command(f, steps_left, steps_right, duration_us):
    duration_ticks = int(duration_us * TICKS_PER_SECOND / 1_000_000)

    # Validate
    assert duration_ticks >= 3200, "Duration too low"
    if steps_left != 0:
        assert (duration_ticks // abs(steps_left)) * abs(steps_left) >= 3200
    if steps_right != 0:
        assert (duration_ticks // abs(steps_right)) * abs(steps_right) >= 3200

    # Write binary
    f.write(struct.pack('<hhII',
                       steps_left,
                       steps_right,
                       duration_ticks,
                       int(duration_us)))

# Write commands
with open("motion_data.bin", "wb") as f:
    # Header
    f.write(struct.pack('<IHHI', 0x4D534146, 1, 2, num_commands))

    # Commands
    for cmd in your_motion_plan:
        export_command(f, cmd.left_steps, cmd.right_steps, cmd.duration_us)
```

### Example: CSV to Binary Converter

If your software outputs CSV:

```csv
steps_left,steps_right,duration_us
100,50,10000
80,80,8000
-100,-50,10000
```

Convert with:

```python
import csv
import struct

TICKS_PER_SECOND = 16_000_000

with open("motion.csv") as csvfile:
    reader = csv.DictReader(csvfile)
    commands = list(reader)

with open("motion_data.bin", "wb") as f:
    # Header
    f.write(struct.pack('<IHHI', 0x4D534146, 1, 2, len(commands)))

    # Commands
    for cmd in commands:
        steps_left = int(cmd['steps_left'])
        steps_right = int(cmd['steps_right'])
        duration_us = float(cmd['duration_us'])
        duration_ticks = int(duration_us * TICKS_PER_SECOND / 1_000_000)

        f.write(struct.pack('<hhII',
                           steps_left, steps_right,
                           duration_ticks, int(duration_us)))
```

## Summary Checklist

- ✅ External software generates valid command list
- ✅ All commands validated (duration ≥ 3200 ticks)
- ✅ Commands exported to .h or .bin file
- ✅ DualControllerSync.ino configured for each motor (LEFT/RIGHT)
- ✅ Same code flashed to both ESP32s
- ✅ SYNC pins wired together with pull-down
- ✅ Motors connected to correct drivers
- ✅ Serial monitors running (115200 baud)
- ✅ Sync signal ready (button/relay)
- ✅ Start triggered, both controllers execute
- ✅ Total time matches on both controllers

You're ready to run synchronized dual-motor motion with external planning! 🚀
