#pragma once

#include "abstractsample.h"
#include <common/config.h>

#include <stdlib.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>

using namespace cv;

class NeoSample : public AbstractSample
{
public:
    NeoSample(DisplayConfig& disp_config, ModelConfig& config);
    ~NeoSample();

    virtual float* getSample();
    virtual void passImage(void* img);
    virtual bool isReady();
    virtual void reset();

    cv::Rect roi;

protected:
    size_t _batch;
    size_t _width;
    size_t _height;
    size_t _channels;

    size_t _in_width;
    size_t _in_height;

    float* _buffer;
    size_t _buffer_size;
    int _head;
    size_t _frame_size;
  
    bool _is_ready;

    bool _save_samples;
    string _save_path;
    int _save_idx;

    void _move_pixels(Mat& img, float* buffer, size_t buffer_size);

    ModelConfig& _config;
    DisplayConfig& _disp_config;
};
