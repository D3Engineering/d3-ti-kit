#pragma once

#include <common/config.h>

/**
 * @brief The Controller abstract class to implement a table controller
 */
class Controller
{
public:
    Controller(TableConfig& config);
    virtual ~Controller();

    virtual int sectionToTicks(double section);
    virtual double ticksToSection(double ticks);
    
    virtual double getSection(void) = 0;
    virtual bool isMoving() = 0;
    virtual void stop() = 0;

    virtual void advance(float value) = 0;
    virtual void setHome() = 0;
    virtual void goHome() = 0;

protected:
    TableConfig& _config;    // table config
    double _position;        /* the current position of the table in ticks */
    bool _moving;            /* whether the table is moving or not */
    double _velocity;        /* the current velocity (limit) of the table */
    double _ticksPerSection; /* number of ticks per section for the motor. */
};
