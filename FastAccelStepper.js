/**
 * FastAccelStepper.js - JavaScript simulation of FastAccelStepper
 *
 * A simplified JavaScript implementation for virtual polargraph simulation.
 * Simulates stepper motor motion planning without hardware control.
 *
 * Usage:
 *   const stepper = new FastAccelStepper();
 *   stepper.setSpeedInHz(1000);
 *   stepper.setAcceleration(500);
 *   stepper.runForward();
 *
 *   // Later, poll position:
 *   console.log(stepper.getCurrentPosition());
 */

class FastAccelStepper {
  constructor() {
    // Motion parameters
    this._maxSpeedHz = 0;           // Maximum speed in steps/second
    this._acceleration = 0;          // Acceleration in steps/s²

    // Current state
    this._position = 0;              // Current position in steps
    this._currentSpeedHz = 0;        // Current speed in steps/second
    this._direction = 0;             // 0 = stopped, 1 = forward, -1 = backward

    // Target state
    this._targetDirection = 0;       // Target direction (1 or -1)
    this._isRunning = false;         // Is the motor running?

    // Timing
    this._lastUpdateTime = null;     // Last time we updated position

    // State machine
    this.RAMP_STATE_IDLE = 0;
    this.RAMP_STATE_ACCELERATE = 1;
    this.RAMP_STATE_COAST = 2;
    this.RAMP_STATE_DECELERATE = 3;

    this._rampState = this.RAMP_STATE_IDLE;
  }

  /**
   * Set the maximum speed in Hz (steps per second)
   * @param {number} speedHz - Speed in steps/second
   */
  setSpeedInHz(speedHz) {
    if (speedHz <= 0) {
      console.warn('Speed must be positive');
      return -1;
    }
    this._maxSpeedHz = speedHz;
    return 0;
  }

  /**
   * Set acceleration in steps/s²
   * @param {number} acceleration - Acceleration in steps/s²
   */
  setAcceleration(acceleration) {
    if (acceleration <= 0) {
      console.warn('Acceleration must be positive');
      return -1;
    }
    this._acceleration = acceleration;
    return 0;
  }

  /**
   * Start running forward continuously
   */
  runForward() {
    if (this._maxSpeedHz === 0) {
      console.warn('Speed not set');
      return -1;
    }
    if (this._acceleration === 0) {
      console.warn('Acceleration not set');
      return -1;
    }

    this._targetDirection = 1;
    this._isRunning = true;
    this._lastUpdateTime = performance.now();
    return 0;
  }

  /**
   * Start running backward continuously
   */
  runBackward() {
    if (this._maxSpeedHz === 0) {
      console.warn('Speed not set');
      return -1;
    }
    if (this._acceleration === 0) {
      console.warn('Acceleration not set');
      return -1;
    }

    this._targetDirection = -1;
    this._isRunning = true;
    this._lastUpdateTime = performance.now();
    return 0;
  }

  /**
   * Stop the motor with deceleration
   */
  stopMove() {
    this._isRunning = false;
  }

  /**
   * Update the internal state based on elapsed time
   * This should be called before reading position/speed
   * @private
   */
  _update() {
    if (this._lastUpdateTime === null) {
      this._lastUpdateTime = performance.now();
      return;
    }

    const now = performance.now();
    const deltaTimeMs = now - this._lastUpdateTime;
    const deltaTime = deltaTimeMs / 1000.0; // Convert to seconds

    if (deltaTime <= 0) {
      return;
    }

    this._lastUpdateTime = now;

    // If not running and current speed is 0, we're idle
    if (!this._isRunning && this._currentSpeedHz === 0) {
      this._rampState = this.RAMP_STATE_IDLE;
      this._direction = 0;
      return;
    }

    // Determine if we need to accelerate or decelerate
    const targetSpeedHz = this._isRunning ?
      this._maxSpeedHz * this._targetDirection : 0;

    const currentSignedSpeed = this._currentSpeedHz * this._direction;
    const speedDiff = targetSpeedHz - currentSignedSpeed;

    // Calculate acceleration direction
    let accelDirection = 0;
    if (Math.abs(speedDiff) > 0.001) {
      accelDirection = speedDiff > 0 ? 1 : -1;
    }

    // Calculate speed change due to acceleration
    const speedChange = accelDirection * this._acceleration * deltaTime;

    // Check if we'll overshoot target speed
    if (Math.abs(speedChange) >= Math.abs(speedDiff)) {
      // We'll reach target speed
      this._currentSpeedHz = Math.abs(targetSpeedHz);
      this._direction = targetSpeedHz === 0 ? 0 : Math.sign(targetSpeedHz);

      if (this._currentSpeedHz === 0) {
        this._rampState = this.RAMP_STATE_IDLE;
      } else if (this._currentSpeedHz >= this._maxSpeedHz - 0.001) {
        this._rampState = this.RAMP_STATE_COAST;
      }
    } else {
      // Continue accelerating/decelerating
      const newSignedSpeed = currentSignedSpeed + speedChange;
      this._currentSpeedHz = Math.abs(newSignedSpeed);
      this._direction = newSignedSpeed === 0 ? 0 : Math.sign(newSignedSpeed);

      if (!this._isRunning || Math.abs(targetSpeedHz) < this._currentSpeedHz) {
        this._rampState = this.RAMP_STATE_DECELERATE;
      } else {
        this._rampState = this.RAMP_STATE_ACCELERATE;
      }
    }

    // Update position based on average speed during this interval
    // Use trapezoidal integration for better accuracy
    const avgSpeedHz = (currentSignedSpeed + this._currentSpeedHz * this._direction) / 2;
    const deltaSteps = avgSpeedHz * deltaTime;
    this._position += deltaSteps;
  }

  /**
   * Get current position in steps
   * @returns {number} Current position
   */
  getCurrentPosition() {
    this._update();
    return Math.round(this._position);
  }

  /**
   * Get current speed in milliHz (steps per 1000 seconds)
   * Returns signed value: positive for forward, negative for backward
   * @returns {number} Current speed in milliHz
   */
  getCurrentSpeedInMilliHz() {
    this._update();
    return Math.round(this._currentSpeedHz * this._direction * 1000);
  }

  /**
   * Get current acceleration in steps/s²
   * Returns signed value: positive when accelerating forward or decelerating backward
   * @returns {number} Current acceleration
   */
  getCurrentAcceleration() {
    this._update();

    if (this._rampState === this.RAMP_STATE_IDLE ||
        this._rampState === this.RAMP_STATE_COAST) {
      return 0;
    }

    if (this._rampState === this.RAMP_STATE_ACCELERATE) {
      return this._acceleration * this._targetDirection;
    }

    if (this._rampState === this.RAMP_STATE_DECELERATE) {
      // Decelerating toward stop
      return -this._acceleration * this._direction;
    }

    return 0;
  }

  /**
   * Check if motor is currently running
   * @returns {boolean} True if running
   */
  isRunning() {
    this._update();
    return this._currentSpeedHz > 0;
  }

  /**
   * Get current ramp state
   * @returns {number} State constant
   */
  getRampState() {
    this._update();
    return this._rampState;
  }
}

// Export for Node.js and browser
if (typeof module !== 'undefined' && module.exports) {
  module.exports = FastAccelStepper;
}
