#include <app/tvm_app.h>

#include <model/neomodel.h>
#include <model/neosample.h>

#include <gui/stats.h>
#include <gui/camgui.h>
#include <gui/gstgui.h>

#include <controller/phidgetcontroller.h>

#include <capdisplay/gstcapturedisplay.h>
#include <capdisplay/ticapturedisplay.h>


TvmApp::TvmApp(AppConfig& config):
_config(config)
{

    this->_config = config;

    this->_controller = NULL;

    this->_sample = new NeoSample(this->_config.disp_config, this->_config.model_config);

    if (this->_config.auto_config.input_mode == InputMode::CAMERA) {
        this->_captureDisplay = new TICaptureDisplay(this->_config.cap_config,
                                                     this->_config.disp_config);
        this->_controller = new PhidgetController(_config.table_config);
        
        this->_gui = new CamGui(this->_config.gui_config,
                                this->_config.disp_config,
                                ((NeoSample*)this->_sample)->roi);
        
        this->_stats = new Stats(100, 2, 0);
    }
    else if (this->_config.auto_config.input_mode == InputMode::VIDEO) {
        this->_captureDisplay = new GstCaptureDisplay(this->_config.video_config,
                                                      this->_config.disp_config);
        this->_gui = new GstGui(this->_config.gui_config,
                                this->_config.disp_config,
                                ((NeoSample*)this->_sample)->roi);

        this->_stats = new Stats(100, 2, 33);
    }
            
    this->_model = new NeoModel(this->_config.model_config);

    this->_auto = new Autonomous(_config.auto_config,
                                 this->_gui,
                                 this->_stats,
                                 this->_model,
                                 this->_captureDisplay,
                                 this->_sample,
                                 this->_controller);
}

/**
 * @brief TVM_APP::~TVM_APP destructor for the object
 */
TvmApp::~TvmApp()
{
    delete this->_model;
    delete this->_captureDisplay;
    delete this->_sample;
    delete this->_gui;
    delete this->_stats;
    delete this->_auto;

    if (this->_controller)
        delete this->_controller;
}

/**
 * @brief TVM_APP::start starts the various threads and gui
 * @return status of the exec function of the app
 */
int TvmApp::start()
{
    this->_captureDisplay->start();
    this->_auto->startDemo();

    return 0;
}

/**
 * @brief TVM_APP::stop stops the various threads and such
 */
void TvmApp::stop()
{
    this->_captureDisplay->stop();
}

bool TvmApp::loop()
{
    this->_auto->loop();

    return true;
}
