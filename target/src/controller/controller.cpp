#include <controller/controller.h>

#include <common/config.h>

/**
 * @brief Controller::Controller constructor for the abstract class of Controller
 * @param config the JSON config object for the app (a pointer to it)
 * @param parent
 */
Controller::Controller(TableConfig& config):
    _config(config)
{
    this->_ticksPerSection = _config.ticks_per_rotation / _config.sections;
    this->_velocity = _config.velocity;
}

/**
 * @brief Controller::~Controller destructor for the Controller
 */
Controller::~Controller() {}

/**
 * @brief Controller::sectionToTicks converts a given section to the number of ticks for the motor
 * @param section the section number to be converted
 * @return the number of ticks in the section
 */
int Controller::sectionToTicks(double section)
{
    return (int) (section * this->_ticksPerSection);
}

/**
 * @brief Controller::ticksToSection the opposite of sectionToTicks, returns the section that the number of ticks is in
 * @param ticks number of ticks to be converted
 * @return the section number associated with the given ticks
 */
double Controller::ticksToSection(double ticks)
{
    return (ticks) / (this->_ticksPerSection);
}
