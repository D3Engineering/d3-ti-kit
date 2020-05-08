#pragma once

#include <model/abstractmodel.h>

#include <capdisplay/abstractcapturedisplay.h>
#include <capdisplay/ticapturedisplay.h>

#include <model/abstractsample.h>

#include <gui/abstractgui.h>
#include <gui/stats.h>

#include <common/config.h>
#include "autonomous.h"

#include <map>
#include <string>
#include <chrono>

using namespace std;

/**
 * @brief The TVM_APP class wrapper class to handle the application
 *
 * This is a class to wrap all of the functions of the application.
 * This handles all of the threads and instances of the different modules. This is effectively a glue layer between everything.
 */
class TvmApp
{
public:
    explicit TvmApp(AppConfig& config);
    ~TvmApp();

    int start();
    void stop();
    bool loop();

private:

    AbstractCaptureDisplay *_captureDisplay;        /* instance of the capture display, gst and overlay */
    AppConfig& _config;
    // Controller *_controller;                /* table controller */
    AbstractModel * _model; /* all of the models */
    AbstractSample* _sample;
    AbstractGui* _gui;
    Stats* _stats;
    Controller* _controller;
    Autonomous* _auto;

    std::chrono::time_point<std::chrono::steady_clock> _lastIPSClock;
};
