#pragma once

#include <common/config.h>
#include <gui/stats.h>
#include <model/modelresult.h>
#include <opencv2/core/types.hpp>

class AbstractGui
{
public:
    AbstractGui(GuiConfig& config, DisplayConfig& disp_config, cv::Rect roi);

    GuiConfig& getConfig() { return this->_config; }

    virtual void start() = 0;
    virtual void stop() = 0;
    
    virtual void loop(void* overlay, Metrics& metrics) = 0;
    
    void passResults(vector<ModelResult> results);
    void dispInfer();
    void resetInfer();


protected:
    GuiConfig& _config;
    DisplayConfig& _disp_config;
    cv::Rect _roi;
    string _pred;
    bool _disp_infer;

    map<string, vector<float>> _preds;
};
