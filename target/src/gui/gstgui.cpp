
#include "gstgui.h"
#include <gui/stats.h>

#include <gst/gst.h>
#include <gst/video/video.h>

#include <sstream>
#include <cairo.h>

using namespace std;

GstGui::GstGui(GuiConfig& config, DisplayConfig& disp_config, cv::Rect roi) :
    AbstractGui(config, disp_config, roi)
{
}

void GstGui::loop(void* overlay, Metrics& metrics)
{
    this->_cur_metric = metrics;

    draw((cairo_t*)overlay);
}

void GstGui::draw(cairo_t * cr)
{
    stringstream fps_ss;
    fps_ss.precision(3);
    fps_ss << "fps: ";
    fps_ss << _cur_metric.fps;

    stringstream ips_ss;
    ips_ss.precision(3);
    ips_ss << "ips: ";
    ips_ss << _cur_metric.ips;

    cairo_set_line_width (cr, 0.5);
    cairo_set_source_rgb (cr, 255, 255, 255);

    if (this->_disp_infer) {
	if (this->_pred == "Pass") {
	    cairo_set_source_rgb(cr, 0, 255, 0);
	}
	else if (this->_pred == "Fail") {
	    cairo_set_source_rgb(cr, 0, 0, 255);
	}
    }
    
    cairo_rectangle(cr,
		   this->_roi.x, this->_roi.y,
		   this->_roi.width, this->_roi.height);

    cairo_move_to(cr, 15, 30);
    if (this->_disp_infer) {
	cairo_show_text(cr, this->_pred.c_str());
    }
    else {
	cairo_show_text(cr, this->_pred.c_str());
    }
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 255, 255, 255);

    cairo_move_to(cr, 237, 20);
    cairo_show_text(cr, fps_ss.str().c_str());

    cairo_move_to(cr, 237, 40);
    cairo_show_text(cr, ips_ss.str().c_str());

    cairo_stroke(cr);
}
