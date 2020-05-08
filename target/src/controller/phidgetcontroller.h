#pragma once

#include "controller.h"
#include <phidget22.h>

/**
 * @brief The PhidgetController class that is a subclass of the abstract Class controller. Communicates with a phidget stepper controller
 */
class PhidgetController : public Controller
{
public:
    PhidgetController(TableConfig& config);
    ~PhidgetController();

    double getSection(void);
    bool isMoving();
    void stop();
    void initStepMode();

    void advance(float value);
    void setHome();
    void goHome();

private:
    double _currentLimit;          /* electric current limit of the controller */
    double _acceleration;          /* acceleration for the table */
    PhidgetStepperHandle _stepper; /* handle for the controller */
};
