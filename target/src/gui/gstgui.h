#pragma once

#include "abstractgui.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include <cairo.h>

class GstGui : public AbstractGui
{
public:
    GstGui(GuiConfig& config, DisplayConfig& disp_config, cv::Rect roi);
    ~GstGui();

    virtual void loop(void* overlay, Metrics& metrics);

    virtual void start() {}
    virtual void stop() {}

    void draw(cairo_t * cr);
    
protected:
    Metrics _cur_metric;
};
