
#include "neosample.h"

#include <common/error.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/photo.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

NeoSample::NeoSample(DisplayConfig& disp_config, ModelConfig& config):
    _disp_config(disp_config),
    _config(config)
{
    this->_in_width = (size_t)disp_config.disp_width;
    this->_in_height = (size_t)disp_config.disp_height;
  
    this->_width = (size_t)config.infer_width;
    this->_height = (size_t)config.infer_height;
    this->_channels = (size_t)config.num_ch;
    this->_batch = (size_t)config.batch_size;
    this->_head = 0;

    this->roi.x = config.crop_x;
    this->roi.y = config.crop_y;
    this->roi.width = this->_width;
    this->roi.height = this->_height;

    this->_frame_size = this->_channels * this->_height * this->_width;
    size_t buffer_size = this->_frame_size * this->_batch;
    this->_buffer = new float[buffer_size];
    this->_buffer_size = buffer_size;
}

NeoSample::~NeoSample()
{
    delete this->_buffer;
}

void NeoSample::_move_pixels(Mat& img, float* buffer, size_t buffer_size)
{
    size_t frame_size_one_channel = img.rows*img.cols;
    size_t row_size_one_channel = img.cols;
    cv::Mat channels[4];

    if (frame_size_one_channel * 3 > buffer_size)
    {
        ERROR("Insufficent space in buffer");
        return;
    }

    // map BGR in Mat to RGB in buffer
    float* ch_ptr_map[3] = {
        buffer + 2*frame_size_one_channel,
        buffer + frame_size_one_channel,
        buffer
    };

    // split channels
    cv::split(img, channels);

    // copy data from split channels into buffer
    for (int i = 0; i < 3; i++)
    {
        cv::Mat& channel = channels[i];
        float* buff_ptr = ch_ptr_map[i];

        // if channel Mat is continuous use memcopy
        if (channel.isContinuous()) {
            memcpy((uchar*)buff_ptr, channel.ptr(), frame_size_one_channel*4);
        }
        // else use memcopy on each row
        else
        {
            DBG("using per-row memcopy\n");
            for (int j = 0; j < channel.rows; j++)
            {
                memcpy((uchar*)buff_ptr, channel.ptr(j), row_size_one_channel*4);
                buff_ptr += row_size_one_channel;
            }
        }
    }
}

void NeoSample::passImage(void* img)
{
    // Crop frame
    Mat frame(cvSize(this->_in_width, this->_in_height),
              CV_8UC4, (char*) img);
  
    Mat pic;
    Mat pic_float;
    
    pic = frame(this->roi);
    pic.convertTo(pic_float, CV_32FC4, 1.0/255.0);

    float* buffer = this->_buffer + (this->_head * this->_frame_size);

    this->_move_pixels(pic_float, buffer, this->_buffer_size);

    this->_head = (this->_head + 1) % this->_batch;

    if (this->_head == 0)
        this->_is_ready = true;
    else
        this->_is_ready = false;
}

void NeoSample::reset()
{
    this->_head = 0;
}

float* NeoSample::getSample()
{
    return this->_buffer;
}

bool NeoSample::isReady()
{
    return this->_is_ready;
}
