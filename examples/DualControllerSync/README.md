# DualControllerSync Example

This example demonstrates synchronized dual-motor control using **two independent microcontrollers** with FastAccelStepper's `moveTimed()` function.

## Use Case

- Two separate ESP32 microcontrollers
- Each controls one stepper motor independently
- Motion planning done in external software
- Both controllers receive a hardware sync signal to start simultaneously
- Same command list flashed to both controllers
- Each controller configured as LEFT or RIGHT motor

## Hardware Setup

```
┌─────────────────┐         ┌─────────────────┐
│   ESP32 #1      │         │   ESP32 #2      │
│   (LEFT motor)  │         │   (RIGHT motor) │
│                 │         │                 │
│  STEP ──────────┼────────▶│  Stepper        │
│  DIR  ──────────┼────────▶│  Driver         │
│  EN   ──────────┼────────▶│  #1             │
│                 │         └─────────────────┘
│  SYNC ──┐       │
└─────────┼───────┘         ┌─────────────────┐
          │                 │   ESP32 #2      │
          │                 │   (RIGHT motor) │
          │                 │                 │
          │                 │  STEP ──────────┼────────▶│  Stepper
          │                 │  DIR  ──────────┼────────▶│  Driver
          │                 │  EN   ──────────┼────────▶│  #2
          │                 │                 │         └─────────────────┘
          │                 │  SYNC ──┐       │
          │                 └─────────┼───────┘
          │                           │
          └───────────┬───────────────┘
                      │
              (Sync Trigger Signal)
              Pull HIGH to start
```

## Configuration

### For LEFT motor controller:
```cpp
#define MOTOR_SIDE MOTOR_LEFT
```

### For RIGHT motor controller:
```cpp
#define MOTOR_SIDE MOTOR_RIGHT
```

## Command List Format

Each command contains motion data for **both motors**:

```cpp
struct MotionCommand {
  int16_t steps_left;      // Steps for LEFT motor (-32767 to +32767)
  int16_t steps_right;     // Steps for RIGHT motor (-32767 to +32767)
  uint32_t duration_ticks; // Duration in timer ticks (16,000,000 ticks/sec for ESP32)
  uint32_t duration_us;    // Duration in microseconds (for debugging)
};
```

### Example:
```cpp
const MotionCommand motionCommands[] = {
  // steps_left, steps_right, duration_ticks, duration_us
  {   100,   50,    160000,   10000 },  // LEFT:100, RIGHT:50, 10ms
  {    80,   80,    128000,    8000 },  // Both 80 steps, 8ms
  {     0,    0,    160000,   10000 },  // Pause 10ms (synchronized)
};
```

## Timing Constraints (ESP32)

| Parameter | Value | Notes |
|-----------|-------|-------|
| Timer frequency | 16,000,000 Hz | 16 MHz |
| Tick duration | 0.0625 µs | 1/16 µs |
| MIN_CMD_TICKS | 3,200 ticks | 200 µs minimum |
| Ticks per ms | 16,000 | For conversion |

### Validation Rules:
1. `duration_ticks ≥ 3200` (minimum 200µs)
2. For non-pause commands: `(duration_ticks / steps) × steps ≥ 3200`
3. Steps range: -32,767 to +32,767
4. Pause command: `steps_left = 0` and `steps_right = 0`

## Generating Command Lists

Use the included Python script to generate commands:

```bash
python generate_motion.py --output-format cpp > motion_data.h
```

Then include in your sketch:
```cpp
#include "motion_data.h"
```

## Synchronization Strategy

1. **Compile & Flash**: Flash the **same code** to both ESP32s
   - Only difference: `MOTOR_SIDE` define (LEFT vs RIGHT)

2. **Power On**: Both controllers boot and validate commands

3. **Wait for Sync**: Both wait at `digitalRead(SYNC_PIN) == LOW`

4. **Trigger Start**: Pull SYNC_PIN HIGH on both controllers simultaneously
   - Use a physical button, relay, or third microcontroller
   - Both start executing within microseconds

5. **Independent Execution**: Each controller executes its motor's steps
   - LEFT controller uses `steps_left` from each command
   - RIGHT controller uses `steps_right` from each command
   - Both use the same `duration_ticks` (synchronized timing)

## Drift Compensation

The example automatically compensates for timing quantization:

```cpp
uint32_t drift = requested_duration - actual_duration;
next_duration = base_duration + drift;
```

This ensures long-term timing accuracy across many commands.

## Status Output

Every second, each controller prints status:
```
[LEFT] Cmd:42/100 Steps:8450 Time:521000us Drift:12 Busy:5 Empty:0 Err:0
```

- **Cmd**: Current command index / total commands
- **Steps**: Total steps executed so far
- **Time**: Total elapsed time in microseconds
- **Drift**: Current timing drift in ticks
- **Busy**: Number of times queue was full
- **Empty**: Number of times queue ran dry (warning)
- **Err**: Number of errors

## Testing

### Simple Test Pattern

```cpp
const MotionCommand motionCommands[] = {
  {  100,  100,  160000,  10000 },  // Both forward 100 steps
  {    0,    0,  160000,  10000 },  // Pause
  { -100, -100,  160000,  10000 },  // Both reverse 100 steps
};
```

Flash to both controllers, connect SYNC_PIN to button, press to start.

### Verify Synchronization

- Both motors should start simultaneously (within microseconds)
- Both should complete at the same time
- Check serial output: total time should match on both controllers

## Troubleshooting

### Queue Empty Warnings
If you see frequent "Queue empty" messages:
- Commands are not being added fast enough
- Reduce command complexity
- Increase command durations
- Check for blocking code in loop()

### Timing Drift
Small drift is normal (quantization). If drift grows continuously:
- Check command validation
- Verify `duration_ticks` values are correct
- Ensure drift compensation is working

### Motors Don't Start Together
- Check SYNC_PIN connections
- Verify both controllers are waiting at sync point
- Use oscilloscope to verify sync signal timing
- Add debug output before sync wait

## Advanced: Loading Commands from SD Card

For large command lists (>1000 commands), load from SD card:

```cpp
#include <SD.h>

void loadCommandsFromSD() {
  File file = SD.open("/motion.bin", FILE_READ);
  for (int i = 0; i < COMMAND_COUNT; i++) {
    file.read((uint8_t*)&motionCommands[i], sizeof(MotionCommand));
  }
  file.close();
}
```

## Command List Generation

See `generate_motion.py` for examples of generating:
- Linear motions
- Circular interpolation
- Coordinated multi-axis moves
- Differential drive patterns
