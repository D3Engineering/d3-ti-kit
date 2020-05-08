#include "stats.h"

#include <numeric>
#include <vector>
#include <stdio.h>

using namespace std;

Stats::Stats(int frame_ring_size, int infer_ring_size, int min_frame_period)
{
    this->_frame_ring_size = frame_ring_size;
    this->_infer_ring_size = infer_ring_size;
    this->_frame_timer = chrono::steady_clock::now();
    this->_infer_timer = chrono::steady_clock::now();
    this->_min_frame_period = min_frame_period;
    this->_current_infer_ms = 0;
}

Stats::~Stats()
{
}

void Stats::frame()
{
    chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();

    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->_frame_timer).count();
    this->_frame_timer = now;

    if (ms < this->_min_frame_period)
        ms = this->_min_frame_period;
    
    this->_frame_duration.insert(this->_frame_duration.begin(), ms);

    if (this->_frame_duration.size() >= (this->_frame_ring_size + 1)) {
	this->_frame_duration.pop_back();
    }

    this->metrics.fps = accumulate(this->_frame_duration.begin(),
				   this->_frame_duration.end(),
				   0);
    this->metrics.fps = 1000.0 / (this->metrics.fps / this->_frame_duration.size());
}

void Stats::resetFrame()
{
    chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
    this->_frame_timer = now;
}

void Stats::startInfer()
{
    chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
    this->_infer_timer = now;
}

void Stats::stopInfer()
{
    chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();

    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->_infer_timer).count();

    this->_current_infer_ms += ms;
    this->_infer_timer = now;
    
}

void Stats::infer(int num)
{
    this->_infer_duration.insert(this->_infer_duration.begin(), this->_current_infer_ms);

    if (this->_infer_duration.size() >= this->_infer_ring_size + 1) {
	this->_infer_duration.pop_back();
    }

    this->metrics.ips = accumulate(this->_infer_duration.begin(),
				   this->_infer_duration.end(),
				   0);

    
    this->metrics.ips = 1000.0 / (this->metrics.ips / this->_infer_duration.size());
    this->metrics.ips *= num;

    this->_current_infer_ms = 0;
}


void Stats::start(string name)
{
    this->_generic_timer_name = name;
    this->_generic_timer = chrono::high_resolution_clock::now();
}

void Stats::stop()
{
    chrono::time_point<chrono::high_resolution_clock> now = chrono::high_resolution_clock::now();
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->_generic_timer).count();
}
