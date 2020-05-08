#pragma once

#include <chrono>
#include <vector>

#include <string>

// #include <ti-acc/utils/perf_stats/include/app_perf_stats.h>

using namespace std;

struct Metrics
{
    float fps;
    float ips;
};

class Stats
{
public:
    Stats(int frame_ring_size, int infer_ring_size, int min_frame_period);
    ~Stats();

    void frame();
    void resetFrame();

    void startInfer();
    void stopInfer();
    void infer(int num);

    void start(string name);
    void stop();

    struct Metrics metrics;

protected:
    chrono::time_point<chrono::steady_clock> _frame_timer;
    chrono::time_point<chrono::steady_clock> _infer_timer;

    vector<int> _frame_duration;
    vector<int> _infer_duration;
    size_t _frame_ring_size;
    size_t _infer_ring_size;
    
    int _head;
    int _min_frame_period;

    int _current_infer_ms;

    string _generic_timer_name;
    chrono::time_point<chrono::high_resolution_clock> _generic_timer;

};
