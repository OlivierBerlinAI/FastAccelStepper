/**
 * FastAccelStepper.js Example
 *
 * Demonstrates basic usage of the JavaScript FastAccelStepper implementation
 */

// For Node.js (uncomment if using Node):
// const FastAccelStepper = require('./FastAccelStepper.js');

// For browser (include FastAccelStepper.js in HTML first)

// Create a stepper instance
const stepper = new FastAccelStepper();

// Configure motion parameters
stepper.setSpeedInHz(1000);      // 1000 steps/second max speed
stepper.setAcceleration(500);     // 500 steps/s² acceleration

// Example 1: Run forward for a bit
console.log('Example 1: Running forward...');
stepper.runForward();

// Simulate polling position at regular intervals
function pollPosition(durationMs, intervalMs) {
  return new Promise((resolve) => {
    const startTime = Date.now();
    const interval = setInterval(() => {
      const position = stepper.getCurrentPosition();
      const speed = stepper.getCurrentSpeedInMilliHz();
      const accel = stepper.getCurrentAcceleration();

      console.log(
        `Position: ${position.toString().padStart(6)} steps, ` +
        `Speed: ${(speed / 1000).toFixed(2).padStart(8)} Hz, ` +
        `Accel: ${accel.toFixed(0).padStart(5)} steps/s²`
      );

      if (Date.now() - startTime >= durationMs) {
        clearInterval(interval);
        resolve();
      }
    }, intervalMs);
  });
}

// Run the example
async function runExample() {
  // Run forward for 2 seconds
  await pollPosition(2000, 100);

  console.log('\nExample 2: Stopping...');
  stepper.stopMove();
  await pollPosition(2000, 100);

  console.log('\nExample 3: Running backward...');
  stepper.runBackward();
  await pollPosition(2000, 100);

  console.log('\nExample 4: Stopping again...');
  stepper.stopMove();
  await pollPosition(2000, 100);

  console.log('\nExample complete!');
  console.log(`Final position: ${stepper.getCurrentPosition()} steps`);
}

// Run in Node.js
if (typeof module !== 'undefined' && module.exports) {
  runExample().catch(console.error);
}

// For browser, attach to button or call manually:
// runExample();
