# FastAccelStepper.js

A JavaScript port of FastAccelStepper for virtual/simulated stepper motor control. This implementation focuses on motion planning and simulation without hardware control, making it ideal for virtual polargraph projects and motion simulation.

## Features

- Time-based position simulation
- Realistic acceleration and deceleration
- Simple API matching the original FastAccelStepper
- Works in both Node.js and browser environments
- No dependencies

## Quick Start

### Browser Usage

```html
<!DOCTYPE html>
<html>
<head>
    <script src="FastAccelStepper.js"></script>
</head>
<body>
    <script>
        const stepper = new FastAccelStepper();
        stepper.setSpeedInHz(1000);
        stepper.setAcceleration(500);
        stepper.runForward();

        // Poll position
        setInterval(() => {
            console.log('Position:', stepper.getCurrentPosition());
        }, 100);
    </script>
</body>
</html>
```

### Node.js Usage

```javascript
const FastAccelStepper = require('./FastAccelStepper.js');

const stepper = new FastAccelStepper();
stepper.setSpeedInHz(1000);
stepper.setAcceleration(500);
stepper.runForward();

// Poll position
setInterval(() => {
    console.log('Position:', stepper.getCurrentPosition());
}, 100);
```

## API Reference

### Configuration Methods

#### `setSpeedInHz(speedHz)`
Set the maximum speed in steps per second.
- **Parameters:** `speedHz` - Speed in Hz (steps/second)
- **Returns:** `0` on success, `-1` on error

#### `setAcceleration(acceleration)`
Set the acceleration in steps/s².
- **Parameters:** `acceleration` - Acceleration in steps/s²
- **Returns:** `0` on success, `-1` on error

#### `setDirectionPin(dirPin, dirHighCountsUp)`
Configure direction pin behavior (for simulation only - no actual pin control).
- **Parameters:**
  - `dirPin` - Direction pin number (ignored in simulation)
  - `dirHighCountsUp` - If true, HIGH means forward/count up (default: true)
- **Note:** In simulation, this doesn't control hardware but can be used to invert direction logic

#### `setCurrentPosition(position)`
Set the current position without moving the motor.
- **Parameters:** `position` - New position value
- **Note:** If a move is in progress, adjusts the target to maintain relative movement

#### `getSpeedInMilliHz()`
Get the configured maximum speed (not the current speed).
- **Returns:** Configured speed in milliHz (steps per 1000 seconds)

### Motion Control Methods

#### `runForward()`
Start running forward continuously at the configured speed.
- **Returns:** `0` on success, `-1` on error

#### `runBackward()`
Start running backward continuously at the configured speed.
- **Returns:** `0` on success, `-1` on error

#### `move(steps)`
Move a relative number of steps from the current position. The motor will accelerate, optionally coast, then decelerate to stop exactly at the target position.
- **Parameters:** `steps` - Number of steps to move (positive=forward, negative=backward)
- **Returns:** `0` on success, `-1` on error
- **Example:** `stepper.move(1000)` moves 1000 steps forward

#### `moveTo(position)`
Move to an absolute position. The motor will accelerate, optionally coast, then decelerate to stop exactly at the target position.
- **Parameters:** `position` - Target position in steps
- **Returns:** `0` on success, `-1` on error
- **Example:** `stepper.moveTo(5000)` moves to absolute position 5000

#### `stopMove()`
Stop the motor with deceleration.

### Query Methods

#### `getCurrentPosition()`
Get the current simulated position in steps.
- **Returns:** Current position (integer)

#### `getCurrentSpeedInMilliHz()`
Get the current speed in milliHz (steps per 1000 seconds).
- **Returns:** Current speed in milliHz (signed: positive=forward, negative=backward)

#### `getCurrentAcceleration()`
Get the current acceleration in steps/s².
- **Returns:** Current acceleration (signed: positive=accelerating forward, negative=decelerating)

#### `isRunning()`
Check if the motor is currently moving.
- **Returns:** `true` if running, `false` if stopped

#### `getRampState()`
Get the current motion state.
- **Returns:** State constant:
  - `0` - IDLE
  - `1` - ACCELERATE
  - `2` - COAST
  - `3` - DECELERATE

#### `targetPos()`
Get the target position for the current move operation.
- **Returns:** Target position (number) if a move is in progress, `null` if running continuously or idle

#### `getPositionAfterCommandsCompleted()`
Get the position where the stepper will be after all current commands are completed.
- **Returns:** Future position in steps
- **Note:** For position-based moves (move/moveTo), returns the target position. For continuous running or idle, returns current position

## Examples

See the included example files:
- **example.js** - Node.js/console example with detailed output showing move() commands
- **example.html** - Interactive browser demo with dual steppers showing:
  - Two steppers with different direction pin configurations
  - Stepper 1: Normal direction (dirHighCountsUp = true)
  - Stepper 2: Inverted direction (dirHighCountsUp = false)
  - Visual demonstration of both relative (move) and absolute (moveTo) positioning
  - Real-time position, speed, and acceleration display

### Running the Examples

**Node.js:**
```bash
node example.js
```

**Browser:**
Open `example.html` in a web browser.

## How It Works

The implementation uses time-based simulation:

1. **Position Tracking:** Position is calculated based on elapsed time and current speed
2. **Acceleration:** Speed changes over time according to the configured acceleration
3. **State Machine:** Tracks whether the motor is accelerating, coasting, or decelerating
4. **Continuous Updates:** Each call to query methods (position, speed, etc.) triggers an internal update based on elapsed time

### Implementation Details

- Uses `performance.now()` for high-resolution timing
- Trapezoidal integration for accurate position calculation during acceleration
- No dependencies on hardware or external libraries
- Lightweight (~200 lines of code)

## Differences from C++ Version

This JavaScript implementation is simplified for simulation:

- **No hardware control** - No pin management, timers, or interrupts
- **Time-based simulation** - Position is calculated from elapsed time, not step-by-step
- **No command queue** - The C++ version uses a command queue for hardware timing; this version simulates continuously
- **Simplified motion planning** - Uses basic trapezoidal motion profiles without the advanced optimization of the C++ version

## Use Cases

Perfect for:
- Virtual polargraph simulation
- Motion planning visualization
- Testing control algorithms without hardware
- Educational purposes
- Prototyping motion control applications

## Limitations

- No hardware pin control (simulation only)
- No enable pin functionality
- Timing accuracy depends on JavaScript's event loop and timer resolution
- No linear acceleration (cubic jerk) support yet
- No command queue like the C++ version (motion is calculated in real-time)

## Future Enhancements

Potential additions:
- Enable pin simulation
- Position limits and soft stops
- Multiple stepper coordination with synchronized movement
- Linear acceleration (cubic jerk) support
- Move queue for planning multiple moves in advance
- Event callbacks (onMoveComplete, onTargetReached, etc.)

## License

This JavaScript port follows the same licensing as the original FastAccelStepper library.

## Original Project

Based on [FastAccelStepper](https://github.com/gin66/FastAccelStepper) by gin66.

## Contributing

Feel free to submit issues or pull requests for improvements!
