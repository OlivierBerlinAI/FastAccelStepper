# FluidTest - Queue Instructions Test

This test demonstrates using FastAccelStepper queue instructions with traced path data for synchronized dual-stepper control on ESP32.

## Hardware Setup

### Pin Connections (ESP32)

**Left Stepper:**
- STEP: GPIO 26
- DIR: GPIO 25
- ENABLE: GPIO 27 (optional)

**Right Stepper:**
- STEP: GPIO 14
- DIR: GPIO 12
- ENABLE: GPIO 13 (optional)

You can modify these pins in the `FluidTest.ino` file to match your hardware.

## Compiling and Uploading

### Option 1: Using PlatformIO (Recommended)

1. Install [PlatformIO](https://platformio.org/install) if you haven't already
2. Navigate to the FluidTest directory:
   ```bash
   cd FluidTest
   ```
3. Connect your ESP32 to your computer
4. Build and upload:
   ```bash
   pio run --target upload
   ```
5. Open serial monitor:
   ```bash
   pio device monitor
   ```

### Option 2: Using Arduino IDE

1. Open Arduino IDE
2. Install the ESP32 board support:
   - Go to File > Preferences
   - Add to "Additional Board Manager URLs": `https://dl.espressif.com/dl/package_esp32_index.json`
   - Go to Tools > Board > Boards Manager
   - Search for "esp32" and install "esp32 by Espressif Systems"
3. Install FastAccelStepper library:
   - Since this is a local development, add the parent directory to your Arduino libraries folder, or use Sketch > Add File to add the library
4. Open `FluidTest.ino`
5. Select your board: Tools > Board > ESP32 Arduino > ESP32 Dev Module
6. Select the correct port: Tools > Port
7. Click Upload

## Running the Test

1. After uploading, open the Serial Monitor at 115200 baud
2. The test will initialize both steppers and display system information
3. Press any key when prompted to start the path execution
4. The test will:
   - Display the traced path data
   - Add all queue entries to both steppers
   - Execute the path with synchronized control
   - Monitor progress and display position updates
   - Verify final positions match expected values

## Adding More Path Data

To add more traced path segments, modify the `tracedPath[]` array in `FluidTest.ino`:

```cpp
const PathData tracedPath[] = {
  {-20, 153, 26, 198},
  {-21, 209, 25, 249},
  {-21, 248, 25, 295},
  {-20, 268, 25, 335},
  {-21, 311, 26, 385},
  // Add your new data here:
  // {leftSteps, leftTicks, rightSteps, rightTicks},
};
```

## Understanding the Output

The serial output shows:
- System information (TICKS_PER_S, MIN_CMD_TICKS)
- Path data table showing all segments
- Real-time position updates during execution
- Final positions and verification

Example output:
```
Time: 150 ms | Left: -42 (3 in queue) | Right: 52 (3 in queue)
```

This shows:
- Elapsed time: 150 ms
- Left stepper position: -42 steps
- Left stepper queue entries: 3 remaining
- Right stepper position: 52 steps
- Right stepper queue entries: 3 remaining

## Troubleshooting

**Issue: Steppers not moving**
- Check wiring and power supply to stepper drivers
- Verify pin definitions match your hardware
- Check if enable pins are active (may need to invert logic)

**Issue: Position mismatch**
- Check if ticks values are above MIN_CMD_TICKS (3200 for ESP32 @ 16MHz)
- Verify stepper driver microstepping settings
- Check mechanical coupling and belt tension

**Issue: Compilation errors**
- Ensure FastAccelStepper library is properly installed
- Verify ESP32 board support is installed
- Check that platformio.ini or Arduino IDE is configured for ESP32

## Technical Details

- **Platform**: ESP32
- **TICKS_PER_S**: 16,000,000
- **MIN_CMD_TICKS**: 3,200 (200 µs minimum command duration)
- **Queue Length**: 32 entries per stepper
- **Driver**: RMT or MCPWM/PCNT (auto-selected by FastAccelStepper)

## Customization

You can customize this test by:
1. Changing pin definitions for your hardware
2. Adding more path data segments
3. Modifying the monitoring interval (currently 100ms)
4. Adding custom logic after path execution in `loop()`
5. Implementing different path patterns or shapes
