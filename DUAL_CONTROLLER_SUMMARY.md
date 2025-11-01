# FastAccelStepper Dual Controller Setup - Complete Summary

## Your Setup

✅ **Two independent ESP32 microcontrollers**
✅ **One motor per controller**
✅ **Hardware sync signal for simultaneous start**
✅ **External motion planning software**
✅ **No communication between controllers during execution**

## What I've Created For You

### 1. **DualControllerSync.ino**
Complete Arduino sketch that:
- Takes a unified command list (both motors' data)
- Extracts only the relevant motor's steps based on LEFT/RIGHT flag
- Handles drift compensation automatically
- Monitors queue health and provides detailed status
- Waits for hardware sync signal before starting

### 2. **Optimal Command List Format**

```cpp
struct MotionCommand {
  int16_t steps_left;      // Steps for LEFT motor (-32767 to +32767)
  int16_t steps_right;     // Steps for RIGHT motor (-32767 to +32767)
  uint32_t duration_ticks; // Duration in timer ticks (16,000,000 Hz for ESP32)
  uint32_t duration_us;    // Duration in microseconds (for debugging)
};
```

**Size:** 12 bytes per command
**Memory:** 12 KB for 1,000 commands, 120 KB for 10,000 commands

### 3. **Documentation**
- **README.md**: Hardware setup, pin configuration, API details
- **WORKFLOW.md**: Complete workflow from motion planning to execution
- **generate_motion.py**: Python script to generate and validate command lists

## Key Parameters for Your Motion Planning Software

### ESP32 Timer Configuration
```
Timer frequency: 16,000,000 Hz (16 MHz)
Tick duration:   0.0625 µs (1/16 microsecond)
Ticks per ms:    16,000
```

### Command Constraints
```
MIN_CMD_TICKS:   3,200 ticks (200 µs minimum duration)
MAX_CMD_TICKS:   65,535 ticks per queue entry (4.096 ms)
MAX_STEPS:       ±32,767 per command
QUEUE_DEPTH:     32 entries
```

### Critical Validation Rule
For every command:
```
duration_ticks ≥ 3,200

AND (for non-pause commands):

(duration_ticks / |steps|) × |steps| ≥ 3,200
```

## Quick Start Guide

### Step 1: Configure Each Controller

**Controller #1 (LEFT motor):**
```cpp
#define MOTOR_SIDE MOTOR_LEFT  // Line 21 in DualControllerSync.ino
```

**Controller #2 (RIGHT motor):**
```cpp
#define MOTOR_SIDE MOTOR_RIGHT  // Line 21 in DualControllerSync.ino
```

### Step 2: Generate Your Command List

From your motion planning software, generate data like this:

```cpp
const MotionCommand motionCommands[] = {
  // steps_left, steps_right, duration_ticks, duration_us
  {   100,   50,    160000,   10000 },  // LEFT:100, RIGHT:50 in 10ms
  {    80,   80,    128000,    8000 },  // Both:80 in 8ms
  {     0,    0,    160000,   10000 },  // Pause 10ms (synchronized)
  {   -50,  -50,    128000,    8000 },  // Reverse
};
```

### Step 3: Flash Both Controllers

1. Open `DualControllerSync.ino` in Arduino IDE
2. Set `MOTOR_SIDE` to `MOTOR_LEFT`
3. Upload to first ESP32
4. Change `MOTOR_SIDE` to `MOTOR_RIGHT`
5. Upload to second ESP32

### Step 4: Hardware Connections

```
Both ESP32s:
- GPIO 26 → Stepper Driver STEP
- GPIO 25 → Stepper Driver DIR
- GPIO 27 → Stepper Driver ENABLE
- GPIO 32 → Shared SYNC signal (10kΩ pull-down to GND)
- GND → Common ground
```

### Step 5: Start Execution

1. Power both controllers
2. Both will show: `Waiting for SYNC signal...`
3. Pull SYNC_PIN (GPIO 32) HIGH → Both start simultaneously
4. Monitor via serial (115200 baud)

## Command List Generation Example

### From Your Motion Planning Software

```python
# Pseudo-code for your external software
TICKS_PER_SECOND = 16_000_000

for each_motion_segment:
    # Calculate steps for each motor
    steps_left = calculate_left_motor_steps()
    steps_right = calculate_right_motor_steps()

    # Calculate duration
    duration_us = calculate_segment_duration()
    duration_ticks = int(duration_us * TICKS_PER_SECOND / 1_000_000)

    # Validate
    assert duration_ticks >= 3200, "Duration too short"
    if steps_left != 0:
        assert (duration_ticks // abs(steps_left)) * abs(steps_left) >= 3200
    if steps_right != 0:
        assert (duration_ticks // abs(steps_right)) * abs(steps_right) >= 3200

    # Export
    output_command(steps_left, steps_right, duration_ticks, duration_us)
```

## Expected Output

Both controllers will show synchronized status:

```
[LEFT] Cmd:42/100 Steps:8450 Time:521000us Drift:12 Busy:5 Empty:0 Err:0
[RIGHT] Cmd:42/100 Steps:4230 Time:521000us Drift:8 Busy:3 Empty:0 Err:0
```

On completion:

```
[LEFT] === All commands completed ===
[LEFT] Total steps: 12500
[LEFT] Total time: 2500000 us
[LEFT] Final drift: 5 ticks (0 us)
```

**✅ Time should match on both controllers** (proves synchronization)

## Files Location

All files are in: `examples/DualControllerSync/`

```
DualControllerSync/
├── DualControllerSync.ino    # Main sketch
├── README.md                  # Setup & API documentation
├── WORKFLOW.md                # Complete workflow guide
└── generate_motion.py         # Helper script (not committed)
```

## Integration Checklist

- [ ] Understand command format (12 bytes per command)
- [ ] Know ESP32 constraints (3,200 min ticks, 16 MHz timer)
- [ ] Configure motion planning software to output valid commands
- [ ] Validate all commands before flashing
- [ ] Set MOTOR_SIDE for each controller (LEFT/RIGHT)
- [ ] Flash same code to both ESP32s (only MOTOR_SIDE differs)
- [ ] Wire SYNC pins together with pull-down
- [ ] Connect motors to respective drivers
- [ ] Test with simple command list first
- [ ] Verify total time matches on both controllers
- [ ] Scale up to full motion sequences

## Committed & Pushed

All files have been committed to branch:
```
claude/fastaccelstepper-info-011CUgwqLx2cKouCJW39CWi5
```

And pushed to remote repository.

## Next Steps

1. **Review the example code** in `examples/DualControllerSync/DualControllerSync.ino`
2. **Read WORKFLOW.md** for complete integration guide
3. **Test with the example command list** included in the sketch
4. **Integrate with your motion planning software** using the format above
5. **Scale to your full motion sequences**

---

## Quick Reference Card

| Item | Value |
|------|-------|
| **Timer Frequency** | 16,000,000 Hz |
| **Min Duration** | 3,200 ticks (200 µs) |
| **Max Steps/Cmd** | ±32,767 |
| **Bytes/Command** | 12 bytes |
| **Queue Depth** | 32 entries |
| **Sync Pin** | GPIO 32 (configurable) |
| **Step Pin** | GPIO 26 (configurable) |
| **Dir Pin** | GPIO 25 (configurable) |
| **Serial Baud** | 115200 |

**Validation Formula:**
`duration_ticks ≥ 3200 AND (duration_ticks/steps)×steps ≥ 3200`

**Time Conversion:**
`duration_ticks = duration_us × 16`

---

Good luck with your synchronized dual-motor system! 🚀
