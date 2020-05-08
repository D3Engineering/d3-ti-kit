#pragma once

#include <string>
#include <map>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <cairo.h>

#include <chrono>

#include "abstractcapturedisplay.h"
#include <common/config.h>

using namespace std;

class GstCaptureDisplay : public AbstractCaptureDisplay
{
public:

    GstCaptureDisplay(VideoConfig& config, DisplayConfig& disp_config);
    ~GstCaptureDisplay();
    
    virtual void* getSample();
    virtual void returnSample(void* sample);
    virtual void* getOverlay();
    virtual void dispFrame();

    virtual bool start();
    virtual void stop();
    virtual void loop();

    bool inferStop();

    static const string GST_PIPE_IN;
    static const string GST_PIPE_OUT;

protected:
    GstElement* _pipeline_in;
    GstElement* _pipeline_out;
    GstAppSink* _appsink;
    GstAppSrc* _appsrc;
    VideoConfig& _config;
    DisplayConfig& _disp_config;

    vector<int> _stops;
    int _frame_idx;
    int _stop_idx;
    void _load_stops(string path);

    chrono::time_point<chrono::steady_clock> _gst_sample_timer;
    bool _first_sample;

    guint8* _last_frame;

    struct GstSampleRecord
    {
	GstSample* sample;
	GstBuffer* buffer;
	GstMapInfo info;
	
	cairo_t* ctx;
    };

    map<guint8*, struct GstSampleRecord> _active_samples;
};
    

