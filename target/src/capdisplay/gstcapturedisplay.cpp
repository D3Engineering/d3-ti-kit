#include "gstcapturedisplay.h"
#include <common/config.h>
#include <common/error.h>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>

#include <cairo.h>

using namespace std;

const string GST_PIPE_IN_OLD = string(" multifilesrc location=video/img%05d.png index=0 caps=image/png,framerate=30/1 ! pngdec ! videoconvert ! videorate ! appsink name=tvm-live-sink");

const string GstCaptureDisplay::GST_PIPE_IN = string("filesrc location=video.mkv ! matroskademux ! jpegdec ! alpha ! video/x-raw,format=RGBA,width=320,height=240,framerate=30/1 ! videoconvert ! videorate ! appsink name=tvm-live-sink");

const string GstCaptureDisplay::GST_PIPE_OUT = string("appsrc is-live=true stream-type=stream format=3 name=tvm-live-src ! queue ! video/x-raw,format=RGBA,width=320,height=240,framerate=30/1 ! videoconvert ! kmssink sync=false can-scale=true ");


GstCaptureDisplay::GstCaptureDisplay(VideoConfig& config, DisplayConfig& disp_config):
    _config(config),
    _disp_config(disp_config)
{
    gst_init(0,0);

    // Build input pipeline string from video_path argument
    stringstream pipe_ss;
    // pipe_ss << "filesrc location=" << config.video_path;
    // pipe_ss << GstCaptureDisplay::GST_PIPE_IN;

    pipe_ss << GST_PIPE_IN;

    DBG("gst-launch-1.0 %s", pipe_ss.str().c_str());
    DBG("gst-launch-1.0 %s", GST_PIPE_OUT.c_str());

    
    // Create pipelines
    // Input pipeline
    GError* err = NULL;
    this->_pipeline_in = gst_parse_launch(pipe_ss.str().c_str(), &err);
    if(err)
    {
	ERROR("failed to create gstreamer input pipeline: %s", err->message);

    }
    else if (this->_pipeline_in == NULL)
    {
	ERROR("failed to create gstreamer input pipeline");
	exit(1);
    }
    DBG("Created gstreamer input pipeline");

    // Ouput pipeline
    this->_pipeline_out = gst_parse_launch(GST_PIPE_OUT.c_str(), &err);
    if(err)
    {
	ERROR("failed to create gstreamer output pipeline: %s", err->message);
    }
    else if (this->_pipeline_out == NULL)
    {
	ERROR("failed to create gstreamer output pipeline");
	exit(1);
    }
    DBG("Created gstreamer output pipeline");

    
    // Get relevant elements from pipeline
    // Get appsink from input pipeline
    GstElement* appsink_ele = gst_bin_get_by_name((GstBin*)this->_pipeline_in, "tvm-live-sink");
    if (appsink_ele == NULL)
    {
	ERROR("failed to get appsink element");
	exit(1);
    }
    this->_appsink = (GstAppSink*)appsink_ele;
    DBG("Got appsink element");

    // Get appsrc from output pipeline
    GstElement* appsrc_ele = gst_bin_get_by_name((GstBin*)this->_pipeline_out, "tvm-live-src");
    if (appsrc_ele == NULL)
    {
	ERROR("failed to get appsrc element");
	exit(1);
    }
    this->_appsrc = (GstAppSrc*)appsrc_ele;
    DBG("Got appsrc element");


    // Read in stops file, so the demo knows when to run inference
    // within the video feed.
    stringstream stops_path;
    stops_path << "stops.txt";
    this->_load_stops(stops_path.str());
    this->_stop_idx = 0;
    this->_frame_idx = 0;

    this->_first_sample = true;
}

GstCaptureDisplay::~GstCaptureDisplay()
{
    gst_element_set_state(this->_pipeline_in, GST_STATE_NULL);
    gst_element_set_state(this->_pipeline_out, GST_STATE_NULL);
    gst_object_unref(this->_pipeline_in);
    gst_object_unref(this->_pipeline_out);
}

void GstCaptureDisplay::_load_stops(string path)
{
    ifstream stop_file(path.c_str());
    string line;

    DBG("_load_stops: open: %s", path.c_str());
    
    if (stop_file.is_open())
    {
	while(getline(stop_file, line))
	{
	    this->_stops.push_back(stoi(line));
	}
    }
    else
    {
	ERROR("Failed to open stops file: %s", path);
	exit(1);
    }

    MSG("loaded in %i stops", this->_stops.size());
}

bool GstCaptureDisplay::start()
{
    GstStateChangeReturn ret;
    
    DBG("start pipeline: try");
    ret = gst_element_set_state(this->_pipeline_in, GST_STATE_PLAYING);
    DBG("pipeline_in -> Playing: %i", (int)ret);
    
    ret = gst_element_set_state(this->_pipeline_out, GST_STATE_PLAYING);
    DBG("pipeline_out -> Playing: %i", (int)ret);
    DBG("start pipeline: success");

    return true;
}

void GstCaptureDisplay::stop()
{
    gst_element_set_state(this->_pipeline_in, GST_STATE_PAUSED);
    gst_element_set_state(this->_pipeline_out, GST_STATE_PAUSED);
}

void* GstCaptureDisplay::getSample()
{
    // DBG("get sample: try");
	
    GstSample* sample = gst_app_sink_pull_sample(this->_appsink);
    if (!sample) {
	bool eos = gst_app_sink_is_eos(this->_appsink);

	if (eos) {
	    MSG("End of Stream");
	    exit(1);
	}
	else
	{
	    ERROR("Failed to get sample");
	}
	
	return 0;
    }
    
    // DBG("get sample: pulled sample from appsink");
    
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer)
    {
	ERROR("failed to get buffer from sample");
	return 0;
    }
    this->_frame_idx = buffer->offset;
    // DBG("buffer offset: %i", buffer->offset);
    // DBG("get sample: got sample buffer");
	
    GstMapInfo info;
    
    bool map_success = gst_buffer_map(buffer, &info, GST_MAP_READ);

    if (!map_success)
    {
	ERROR("Failed to map gstreamer sample");
	return 0;
    }
    // DBG("get sample: mapped buffer");

    struct GstSampleRecord rec;
    rec.sample = sample;
    rec.buffer = buffer;
    rec.info = info;
    rec.ctx = NULL;

    this->_active_samples[rec.info.data] = rec;

    // DBG("get sample: success");

    this->_last_frame = rec.info.data;
    return (void*)rec.info.data;
}

void* GstCaptureDisplay::getOverlay()
{
    GstSampleRecord& record = this->_active_samples[this->_last_frame];
    unsigned char* img = (unsigned char*)record.info.data;
    
    int width = this->_disp_config.disp_width;
    int height = this->_disp_config.disp_height;
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    
    cairo_surface_t* surf = cairo_image_surface_create_for_data(img,
								CAIRO_FORMAT_ARGB32,
								width, height,
								stride);
    if (surf == NULL)
    {
	ERROR("Failed to create cairo surface");
	exit(1);
    }

    cairo_t* ctx = cairo_create(surf);
    cairo_surface_destroy(surf);

    if (ctx == NULL)
    {
	ERROR("Failed to create cairo context");
	exit(1);
    }

    record.ctx = ctx;
    
    return (void*)record.ctx;
}

void GstCaptureDisplay::returnSample(void* img)
{
    if (this->_active_samples.find((guint8*)img) ==
	this->_active_samples.end())
    {
	return;
    }
    else {
	if (this->_first_sample)
	{
	    this->_gst_sample_timer = std::chrono::steady_clock::now();
	    this->_first_sample = false;
	}

	struct GstSampleRecord& rec = this->_active_samples[(guint8*)img];
	gst_buffer_unmap(rec.buffer, &rec.info);

	GstFlowReturn ret;
	
	// appsrc takes ownership of buffer
	gst_buffer_ref(rec.buffer);  // ref so that buffer survives sample unref

	auto pts = chrono::duration_cast<chrono::nanoseconds>(
	    chrono::steady_clock::now() - this->_gst_sample_timer).count();
	if (pts > 0)
	    rec.buffer->pts = (GstClockTime)pts;
	else
	    ERROR("Calculated Negative timestamp");
	
	ret = gst_app_src_push_buffer(this->_appsrc, rec.buffer);
	if (ret != GST_FLOW_OK)
	{
	    ERROR("Failed to push sample to appsrc");
	}

	// buffer is ref-ed above, so unref-ing the sample won't cause problems
	gst_sample_unref(rec.sample);
	cairo_destroy(rec.ctx);

	this->_active_samples.erase((guint8*)img);
    }
}

void GstCaptureDisplay::dispFrame()
{
    
}

void GstCaptureDisplay::loop()
{
}

bool GstCaptureDisplay::inferStop()
{
    // return false;
    if (this->_frame_idx > this->_stops[this->_stop_idx]) {
	this->_stop_idx += 1;

	if (this->_stop_idx >= this->_stops.size())
	{
	    this->_stop_idx = this->_stops.size() - 1;
	}
    }
    
    // stops come in pairs, if even the table is moving
    if (this->_stop_idx % 2 == 0)
    {
	// DBG("don't infer");
	return false;
    }

    // DBG("infer");
    return true;
}
