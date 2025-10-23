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

    // Direction pin configuration
    this._directionPinDefined = false;
    this._dirHighCountsUp = true;    // If true, HIGH on dir pin means count up

    // Target state
    this._targetDirection = 0;       // Target direction (1 or -1)
    this._isRunning = false;         // Is the motor running?
    this._targetPosition = null;     // Target position for move() command (null = continuous)

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
   * Configure direction pin behavior (for simulation only - no actual pin control)
   * @param {number} dirPin - Direction pin number (ignored in simulation)
   * @param {boolean} dirHighCountsUp - If true, HIGH means forward/count up
   * @note In this simulation, dirHighCountsUp only affects visual/physical interpretation.
   *       Position coordinates always work the same way for consistent coordinate systems.
   */
  setDirectionPin(dirPin, dirHighCountsUp = true) {
    this._directionPinDefined = true;
    this._dirHighCountsUp = dirHighCountsUp;
  }

  /**
   * Get the direction pin configuration
   * @returns {boolean} True if HIGH means forward, false if HIGH means backward
   */
  getDirectionPinConfig() {
    return this._dirHighCountsUp;
  }

  /**
   * Set the current position without moving
   * @param {number} position - New position value
   */
  setCurrentPosition(position) {
    this._update();
    this._position = position;
    if (this._targetPosition !== null) {
      // Adjust target position to maintain relative movement
      const relativeMove = this._targetPosition - this._position;
      this._targetPosition = position + relativeMove;
    }
  }

  /**
   * Get the configured speed in milliHz (not current speed, but set speed)
   * @returns {number} Configured speed in milliHz
   */
  getSpeedInMilliHz() {
    return Math.round(this._maxSpeedHz * 1000);
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
    this._targetPosition = null;  // Continuous movement
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
    this._targetPosition = null;  // Continuous movement
    this._isRunning = true;
    this._lastUpdateTime = performance.now();
    return 0;
  }

  /**
   * Move a relative number of steps from current position
   * @param {number} steps - Number of steps to move (positive=forward, negative=backward)
   */
  move(steps) {
    if (this._maxSpeedHz === 0) {
      console.warn('Speed not set');
      return -1;
    }
    if (this._acceleration === 0) {
      console.warn('Acceleration not set');
      return -1;
    }
    if (steps === 0) {
      return 0;
    }

    // Update current position first
    this._update();

    // Calculate target position
    this._targetPosition = this._position + steps;
    this._targetDirection = steps > 0 ? 1 : -1;
    this._isRunning = true;
    this._lastUpdateTime = performance.now();
    return 0;
  }

  /**
   * Move to an absolute position
   * @param {number} position - Target position in steps
   */
  moveTo(position) {
    if (this._maxSpeedHz === 0) {
      console.warn('Speed not set');
      return -1;
    }
    if (this._acceleration === 0) {
      console.warn('Acceleration not set');
      return -1;
    }

    // Update current position first
    this._update();

    const steps = position - this._position;
    if (steps === 0) {
      return 0;
    }

    this._targetPosition = position;
    this._targetDirection = steps > 0 ? 1 : -1;
    this._isRunning = true;
    this._lastUpdateTime = performance.now();
    return 0;
  }

  /**
   * Stop the motor with deceleration
   */
  stopMove() {
    this._isRunning = false;
    this._targetPosition = null;
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

    const currentSignedSpeed = this._currentSpeedHz * this._direction;

    // Determine target speed based on whether we have a target position
    let targetSpeedHz;

    if (this._targetPosition !== null) {
      // Position-based movement
      const distanceRemaining = this._targetPosition - this._position;
      const distanceRemainingAbs = Math.abs(distanceRemaining);

      // Calculate deceleration distance needed from current speed
      // Using: d = v² / (2*a)
      const decelDistance = (this._currentSpeedHz * this._currentSpeedHz) / (2 * this._acceleration);

      // Check if we've reached target
      if (distanceRemainingAbs < 0.5) {
        // Reached target
        this._position = this._targetPosition;
        this._currentSpeedHz = 0;
        this._direction = 0;
        this._isRunning = false;
        this._rampState = this.RAMP_STATE_IDLE;
        this._targetPosition = null;
        return;
      }

      // Check if we need to start decelerating
      const distanceDirection = Math.sign(distanceRemaining);
      const movingInCorrectDirection = (this._direction === distanceDirection || this._direction === 0);

      if (!movingInCorrectDirection) {
        // Moving in wrong direction, need to stop and reverse
        targetSpeedHz = 0;
      } else if (decelDistance >= distanceRemainingAbs * 0.95) {
        // Need to decelerate now
        targetSpeedHz = 0;
      } else {
        // Can still accelerate or coast
        targetSpeedHz = this._maxSpeedHz * distanceDirection;
      }
    } else {
      // Continuous movement
      targetSpeedHz = this._isRunning ?
        this._maxSpeedHz * this._targetDirection : 0;
    }

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

      if (targetSpeedHz === 0 || Math.abs(targetSpeedHz) < this._currentSpeedHz) {
        this._rampState = this.RAMP_STATE_DECELERATE;
      } else {
        this._rampState = this.RAMP_STATE_ACCELERATE;
      }
    }

    // Update position based on average speed during this interval
    // Use trapezoidal integration for better accuracy
    const avgSpeedHz = (currentSignedSpeed + this._currentSpeedHz * this._direction) / 2;
    const deltaSteps = avgSpeedHz * deltaTime;

    // Note: dirHighCountsUp affects physical motor direction, not position counting
    // Position coordinates always work the same way regardless of direction pin setting
    // This allows consistent coordinate systems across multiple steppers
    const newPosition = this._position + deltaSteps;

    // If we have a target position, don't overshoot it
    if (this._targetPosition !== null) {
      const passedTarget = (this._direction > 0 && newPosition >= this._targetPosition) ||
                          (this._direction < 0 && newPosition <= this._targetPosition);

      if (passedTarget) {
        this._position = this._targetPosition;
        this._currentSpeedHz = 0;
        this._direction = 0;
        this._isRunning = false;
        this._rampState = this.RAMP_STATE_IDLE;
        this._targetPosition = null;
        return;
      }
    }

    this._position = newPosition;
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

  /**
   * Get the target position for the current move
   * Returns null if running continuously or idle
   * @returns {number|null} Target position or null
   */
  targetPos() {
    return this._targetPosition;
  }

  /**
   * Get the position after all commands are completed
   * For position-based moves, returns the target position
   * For continuous running or idle, returns current position
   * @returns {number} Future position in steps
   */
  getPositionAfterCommandsCompleted() {
    this._update();
    if (this._targetPosition !== null) {
      return Math.round(this._targetPosition);
    }
    return Math.round(this._position);
  }
}

// Export for Node.js and browser
if (typeof module !== 'undefined' && module.exports) {
  module.exports = FastAccelStepper;
}
