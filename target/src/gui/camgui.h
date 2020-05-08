#pragma once

#include <common/config.h>
#include <gui/stats.h>
#include <model/modelresult.h>

#include <opencv2/core/types.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>

#include <string>
#include <vector>
#include <map>

#include "abstractgui.h"


using namespace cv;
using namespace std;

struct TextBox
{
  int top, left;
  float scale;

  TextBox(int top, int left, float scale)
  {
    this->top = top;
    this->left = left;
    this->scale = scale;
  }

  void nextLine()
  {
    this->top += scale * 40;
  }
};

class CamGui : public AbstractGui
{
 public:
  CamGui(GuiConfig& config, DisplayConfig& disp_config, cv::Rect roi);
  ~CamGui();

  virtual void start() {}
  virtual void stop() {}
  virtual void loop(void* overlay, Metrics& metrics);

  static const Scalar WHITE;
  static const Scalar GREEN;
  static const Scalar RED;

 protected:

  void drawLine(Mat frame, string sr, TextBox& tb);
  void drawLine(Mat frame, string sr, TextBox& tb, Scalar color);
  void drawRoi(Mat frame, int thickness, Scalar color);
};
