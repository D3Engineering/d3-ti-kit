#include "camgui.h"

#include <model/modelresult.h>
#include <common/error.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

#include <sstream>
#include <string>
#include <iomanip>
#include <vector>
#include <numeric>

#include <ncurses.h>

using namespace cv;
using namespace std;

const Scalar CamGui::WHITE = Scalar(255,255,255,255);
const Scalar CamGui::GREEN = Scalar(0,255,0,255);
const Scalar CamGui::RED = Scalar(0,0,255,255);

CamGui::CamGui(GuiConfig& config, DisplayConfig& disp_config, cv::Rect roi) :
    AbstractGui(config, disp_config, roi)
{
}

void CamGui::drawLine(Mat frame, string str, TextBox& tb)
{
    this->drawLine(frame, str, tb, CamGui::WHITE);
}

void CamGui::drawLine(Mat frame, string str, TextBox& tb, Scalar color)
{
    cv::putText(frame, str.c_str(), cv::Point(tb.left, tb.top),
                FONT_HERSHEY_DUPLEX, tb.scale,
                color,1);
    tb.nextLine();
}

void CamGui::drawRoi(Mat frame, int thickness, Scalar color)
{
    int line_type = 8;
    int shift = 0;

    // DBG("roi: x: %i, y: %i, width: %i, height: %i\n", this->_roi.x, this->_roi.y, this->_roi.width, this->_roi.height);
  
    rectangle(frame, this->_roi, color, thickness, line_type, shift);
}

void CamGui::loop(void* overlay, Metrics& metrics)
{
    int height = this->_disp_config.disp_height;
    int width = this->_disp_config.disp_width;

    TextBox stats_tb(20, 237, 0.5);
    TextBox infer_tb(30, 15, 0.5);
  
    Mat frame = cv::Mat(height, width, CV_8UC4, overlay);
    frame = Scalar(0,0,0,0);

    stringstream fps_ss;
    fps_ss.precision(3);
    fps_ss << "fps: ";
    fps_ss << metrics.fps;

    stringstream ips_ss;
    ips_ss.precision(3);
    ips_ss << "ips: ";
    ips_ss << metrics.ips;
  
    this->drawLine(frame, fps_ss.str(), stats_tb);
    this->drawLine(frame, ips_ss.str(), stats_tb);

    if (this->_disp_infer) {
        
        if (this->_pred == "Pass") {
            this->drawLine(frame, this->_pred, infer_tb, CamGui::GREEN);
            this->drawRoi(frame, 1, CamGui::GREEN);
        }
        else if (this->_pred == "Fail") {
            this->drawLine(frame, this->_pred, infer_tb, CamGui::RED);
            this->drawRoi(frame, 1, CamGui::RED);
        }
        else
        {
            this->drawLine(frame, this->_pred, infer_tb, CamGui::WHITE);
            this->drawRoi(frame, 1, CamGui::WHITE);
        }
    }
    else {
        this->drawRoi(frame, 1, CamGui::WHITE);
    }
    
}


