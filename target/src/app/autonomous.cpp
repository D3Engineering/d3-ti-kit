#include "autonomous.h"
#include <common/config.h>
#include <capdisplay/abstractinfertimer.h>
#include <capdisplay/gstcapturedisplay.h>

#include <gui/abstractgui.h>
#include <gui/camgui.h>
#include <gui/gstgui.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <thread>
#include <chrono>


Autonomous::Autonomous(AutoConfig& auto_config,
		       AbstractGui* gui,
		       Stats* stats,
		       AbstractModel* model,
		       AbstractCaptureDisplay* cap_display,
		       AbstractSample* sample,
		       Controller* controller) :
    _config(auto_config)
{
    this->_gui = gui;
    this->_stats = stats;
    this->_model = model;
    this->_cap_display = cap_display;
    this->_controller = controller;
    this->_sample = sample;

    this->_state = States::IDLE;
    this->_save_path = auto_config.save_path;
    this->_save_imgs = (this->_save_path != "");
    this->_skip_infer = this->_save_imgs || auto_config.no_infer;
    this->_save_idx = 0;
    
    this->stopTimer();
}

Autonomous::~Autonomous()
{}

struct StateVars& Autonomous::getStateVars()
{
    if (this->_config.input_mode == InputMode::CAMERA) {
	this->_state_vars.table_moving = this->_controller->isMoving();
    }
    else if (this->_config.input_mode == InputMode::VIDEO) {
	this->_state_vars.table_moving = !((GstCaptureDisplay*)this->_cap_display)->inferStop();
    }

    if (this->_state == States::IDLE) {
	if (this->_state_vars.mode == StateVars::Modes::CAM_DEMO) {
	    this->_state_vars.move_delta = this->_config.table_coarse_step;
	}
    }

    return this->_state_vars;
}

enum States Autonomous::nextState()
{
    enum States next_state;
    struct StateVars& state_vars = this->getStateVars();
  
    if (this->_state == States::IDLE) {
		
	if (state_vars.mode == StateVars::Modes::CAM_DEMO)
	    next_state = States::MOVING;

	else if (state_vars.mode == StateVars::Modes::VID_DEMO)
	    if (state_vars.table_moving)
		next_state = States::IDLE;
	    else
		next_state = States::INFER;
	
	else if (state_vars.mode == StateVars::Modes::INFER)
	    next_state = States::INFER;
	else
	    next_state = States::IDLE;

	if (next_state == States::MOVING && this->_config.no_table)
	    next_state = States::INFER;
	
    }
    else if (this->_state == States::MOVING) {
	bool done = this->tickTimer(this->_config.table_move_settle_time * MS_PER_SEC);

	if (!done)
	    next_state = States::MOVING;
	else if (state_vars.table_moving)
	    next_state = States::MOVING;
	
	else if (state_vars.mode == StateVars::Modes::CAM_DEMO) {
	    next_state = States::INFER;
	}
	
	else if (state_vars.mode == StateVars::Modes::INFER) {
	    next_state = States::INFER;
	}
	
	else // State::NONE
	    next_state = States::IDLE;

	if (next_state != States::MOVING) {
	    this->stopTimer();
	}
    }
    else if (this->_state == States::INFER) {

	bool done = this->tickTimer(this->_config.infer_settle_time * MS_PER_SEC);

	// If in a video demo, either stay in infer, or go back to idle
	if (state_vars.mode == StateVars::Modes::VID_DEMO)
	    if (state_vars.table_moving)
		next_state = States::IDLE;
	    else
		next_state = States::INFER;
		
	// if in a camera demo, infer for the specified time
	else if (!done)
	    next_state = States::INFER;
	else if (state_vars.mode == StateVars::Modes::CAM_DEMO)
	    next_state = States::IDLE;
	else if (state_vars.mode == StateVars::Modes::INFER)
	    if (state_vars.move_delta > 0.0)
		next_state = States::MOVING;
	    else
		next_state = States::INFER;
	else
	    next_state = States::IDLE;

	if (next_state != States::INFER)
	{
	    this->stopTimer();
	    this->_gui->resetInfer();
	    this->_sample->reset();
	}
    
    }
    else {
	ERROR("This state shouldn't exist");
    }

    return next_state;
}

void Autonomous::procInput()
{
    string str;
  
    GuiConfig& gui_config = this->_gui->getConfig();
    enum GuiConfig::Ctrls ctrl;
  
    if (gui_config.ui_key_map.find(str) == gui_config.ui_key_map.end()) {
	return;
    }
    
    ctrl = this->_gui->getConfig().ui_key_map[str];

    switch (ctrl){
    case GuiConfig::Ctrls::TABLE_COARSE_FORWARD:
	this->_state_vars.move_delta = this->_config.table_coarse_step;
	break;
		
    case GuiConfig::Ctrls::TABLE_COARSE_BACKWARD:
	this->_state_vars.move_delta = -this->_config.table_coarse_step;
	break;
		
    case GuiConfig::Ctrls::TABLE_FINE_FORWARD:
	this->_state_vars.move_delta = this->_config.table_fine_step;
	break;
		
    case GuiConfig::Ctrls::TABLE_FINE_BACKWARD:
	this->_state_vars.move_delta = -this->_config.table_fine_step;
	break;
    }
}


void Autonomous::startDemo()
{
    if (this->_config.input_mode == InputMode::CAMERA) {
	this->_state_vars.mode = StateVars::Modes::CAM_DEMO;
    }
    else if (this->_config.input_mode == InputMode::VIDEO) {
	this->_state_vars.mode = StateVars::Modes::VID_DEMO;
    }
    else
    {
	ERROR("Unsuppored Input Mode");
	exit(1);
    }

    this->_stats->resetFrame();
}
  

void Autonomous::loop()
{
    this->_stats->start("get sample & overlay");
    void* img = this->_cap_display->getSample();
    void* overlay = this->_cap_display->getOverlay();
    this->_stats->stop();
    
    this->_stats->frame();

    if (this->_save_imgs)
	this->_save_img(img);

    this->_stats->start("State setup");
    enum States next_state = this->nextState();
    struct StateVars& state_vars = this->getStateVars();
    this->_stats->stop();

    if (this->_state == States::IDLE) {
	DBG("IDLE");
    }
    else if (this->_state == States::MOVING) {
	DBG("MOVING: %i", (int)state_vars.table_moving);
	if (state_vars.first_iter) {
	    this->_controller->advance(state_vars.move_delta);
	    state_vars.move_delta = 0.0;
	}
    }
    else if (this->_state == States::INFER && !this->_skip_infer) {
	DBG("INFER");
	bool ran_infer = false;

	this->_stats->start("Pass Image");
	this->_sample->passImage(img);
	this->_stats->stop();
	if (this->_sample->isReady()) {
	    this->_stats->startInfer();
	    this->_stats->start("Pass Sample");
	    this->_model->passSample(this->_sample);
	    this->_stats->stop();
	    this->_stats->stopInfer();
	}

	if (this->_model->isResultReady()) {
	    this->_stats->start("pass results to gui");
	    vector<ModelResult> results = this->_model->PopResults();
	    this->_gui->passResults(results);
	    ran_infer = true;
	    this->_gui->dispInfer();
	    this->_stats->stop();
	}
	
	// Inference Timing
	if (ran_infer) {
	    this->_stats->infer(this->_model->getConfig().batch_size);
	}
    }
    else if (!this->_skip_infer)
	ERROR("This state shouldn't exist");

	
    if (this->_state != next_state) {
	DBG("First Iter");
	state_vars.first_iter = true;
    }
    else {
	state_vars.first_iter = false;
    }

    this->_state = next_state;

    this->_stats->start("gui loop");
    this->_gui->loop(overlay, this->_stats->metrics);
    this->_stats->stop();

    this->_stats->start("disp frame & return sample");
    this->_cap_display->dispFrame();
    this->_cap_display->returnSample(img);
    this->_stats->stop();
}


bool Autonomous::tickTimer(int ms)
{
    chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
  
    if (this->_timer_active) {
	int ms_passed = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->_timer).count();

	if (ms_passed >= ms) {
	    return true;
	}
    }
    else {
	this->_timer_active = true;
	this->_timer = now;
    }

    return false;
}

void Autonomous::stopTimer()
{
    this->_timer_active = false;
}

void Autonomous::_save_img(void* img)
{
    int width = this->_cap_display->out_width;
    int height = this->_cap_display->out_height;
    
    Mat frame(cvSize(width, height),
              CV_8UC4, (char*) img);

    stringstream number_ss;
    stringstream file_ss;
    
    number_ss << this->_save_idx;

    file_ss << this->_save_path << "img";
    file_ss << string(5 - number_ss.str().length(), '0') << number_ss.str();
    file_ss << ".png";
    
    imwrite(file_ss.str(), frame);
    this->_save_idx++;
}

