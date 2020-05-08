#pragma once

#include <common/error.h>
#include <common/config.h>
#include <gui/abstractgui.h>
#include <capdisplay/abstractcapturedisplay.h>
#include <controller/controller.h>
#include <gui/stats.h>
#include <model/abstractmodel.h>
#include <model/abstractsample.h>

#include <stdio.h>
#include <chrono>

#define MS_PER_SEC (1000)

using namespace std;

struct StateVars
{
    enum Modes {
	NONE,
	CAM_DEMO,
	VID_DEMO,
	INFER
    } mode;

    bool table_moving;
    bool no_table;
    float move_delta;

    bool first_iter;
};

enum States {
    IDLE,
    MOVING,
    INFER
};

class Autonomous
{
public:
    Autonomous(AutoConfig& auto_config,
	       AbstractGui* gui,
	       Stats* stats,
	       AbstractModel* model,
	       AbstractCaptureDisplay* cap_display,
	       AbstractSample* sample,
	       Controller* controller);

    ~Autonomous();

    void loop();
    void startDemo();

private:
    AbstractGui* _gui;
    Stats* _stats;
    AbstractModel* _model;
    AbstractCaptureDisplay* _cap_display;
    Controller* _controller;
    AutoConfig& _config;
    AbstractSample* _sample;

    enum States _state;
    struct StateVars _state_vars;

    chrono::time_point<chrono::steady_clock> _timer;
    bool _timer_active;
    bool tickTimer(int ms);
    void stopTimer();

    void procInput();
    enum States nextState();
    struct StateVars& getStateVars();

    bool _skip_infer;
    
    void _save_img(void* img);
    bool _save_imgs;
    string _save_path;
    int _save_idx;
};
