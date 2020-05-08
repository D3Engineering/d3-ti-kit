#include "phidgetcontroller.h"

#include <common/config.h>
#include <common/error.h>

/**
 * @brief PhidgetController::PhidgetController constructor for the phidget controller class
 * @param config pointer to the JSON config
 */
PhidgetController::PhidgetController(TableConfig& config)
    : Controller(config)
{
    this->_acceleration = _config.acceleration;
    this->_currentLimit = _config.current_limit;
    
    PhidgetReturnCode res = PhidgetStepper_create(&this->_stepper);
    if (EPHIDGET_OK != res) {
        WARN("Failed to create Stepper controller instance");
    }
    // TODO: Add appropriate error handling
    res = Phidget_openWaitForAttachment((PhidgetHandle) this->_stepper, 5000);
    if (EPHIDGET_OK != res) {
        WARN("Failed to connect to Stepper controller");
    }
    this->initStepMode();
    res = PhidgetStepper_setEngaged(this->_stepper, 1);
}

/**
 * @brief PhidgetController::~PhidgetController destructor for the object
 */
PhidgetController::~PhidgetController()
{
    PhidgetReturnCode res = PhidgetStepper_delete(&this->_stepper);
    if (EPHIDGET_OK != res) {
        WARN("Failed to delete Stepper controller instance");
    }
}

/**
 * @brief PhidgetController::getSection gets the current section that the table controller is on
 * @return current section
 */
double PhidgetController::getSection()
{
    /* gets current position in ticks */
    PhidgetStepper_getPosition(this->_stepper, &this->_position);
    return this->ticksToSection(this->_position); /* returns the converted section number */
}

/**
 * @brief PhidgetController::isMoving checks if the motor controller is moving
 * @return if the table is moving
 */
bool PhidgetController::isMoving()
{
    PhidgetStepper_getIsMoving(this->_stepper, (int *) &this->_moving);
    return this->_moving;
}

/**
 * @brief PhidgetController::setHome set the home position to the current position
 */
void PhidgetController::setHome()
{
    double position;
    PhidgetStepper_getPosition(this->_stepper, &position);
    PhidgetStepper_addPositionOffset(this->_stepper, -1 * position); /* zeroes out the position */
}

/**
 * @brief PhidgetController::goHome sets the position to the 0 position
 */
void PhidgetController::goHome()
{
    PhidgetStepper_setTargetPosition(this->_stepper, 0);
}

void PhidgetController::stop() {}

/**
 * @brief PhidgetController::initStepMode sets up the various control modes
 */
void PhidgetController::initStepMode()
{
    PhidgetStepper_setControlMode(this->_stepper, CONTROL_MODE_STEP);
    PhidgetStepper_setAcceleration(this->_stepper, this->_acceleration);
    PhidgetStepper_setCurrentLimit(this->_stepper, this->_currentLimit);
    PhidgetStepper_setVelocityLimit(this->_stepper, this->_velocity);
    PhidgetStepper_setRescaleFactor(this->_stepper, 1);
}

/**
 * @brief PhidgetController::advance slot to advance the motor
 * @param value
 */
void PhidgetController::advance(float value)
{
    double currentPosition;
    PhidgetReturnCode res;
    res = PhidgetStepper_getPosition(this->_stepper, &currentPosition);
    if (EPHIDGET_OK != res) {
        WARN("Failed to current position of Stepper controller");
    }
    double sec = this->sectionToTicks(value);
    currentPosition += sec;
    res = PhidgetStepper_setTargetPosition(this->_stepper, currentPosition);
    if (EPHIDGET_OK != res) {
        WARN("Failed to set target of Stepper controller");
    }
}
