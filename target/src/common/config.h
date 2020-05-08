#pragma once

#include <string>
#include <vector>
#include <map>

using namespace std;


struct ModelConfig
{
    enum ModelType
    {
        FAKE,
        REAL
    } model_type;

    enum InputFormat {
        ZERO_TO_ONE,
        ZERO_TO_255,
        NEG_ONE_TO_ONE,
    } input_format;

    string name;
    string path;

    vector<string> labelMap;
    
    int batch_size;
    int infer_width;
    int infer_height;
    int crop_x;
    int crop_y;
    int num_ch;
    string _input_name;

    size_t classes;
};

struct CaptureConfig
{
    string device;
    int capture_width;
    int capture_height;
    int crop_x;
    int crop_y;
    int zoom;
    int focus;
};

struct DisplayConfig
{
    int disp_width;
    int disp_height;

    string save_path;
};

struct VideoConfig
{
    string video_path;
};

struct TableConfig
{
    int ticks_per_rotation;
    int sections;
    float velocity;
    float acceleration;
    float current_limit;
};

struct GuiConfig
{
    enum Ctrls {
        TABLE_COARSE_FORWARD,
        TABLE_COARSE_BACKWARD,
        TABLE_FINE_FORWARD,
        TABLE_FINE_BACKWARD,

        MODEL_RUN_INFER,
        MODEL_ARM,
        MODEL_EVE,

        DEMO_TOGGLE
    };

    map<string, enum Ctrls> ui_key_map;
    float font_size;
    size_t pred_ring_size;
    float pred_cutoff;

    void useDefaults()
    {
        this->ui_key_map["n"] = TABLE_COARSE_FORWARD;
        this->ui_key_map["p"] = TABLE_COARSE_BACKWARD;
        this->ui_key_map["N"] = TABLE_FINE_FORWARD;
        this->ui_key_map["P"] = TABLE_FINE_BACKWARD;
        this->ui_key_map["i"] = MODEL_RUN_INFER;
        this->ui_key_map["a"] = MODEL_ARM;
        this->ui_key_map["e"] = MODEL_EVE;
        this->ui_key_map["d"] = DEMO_TOGGLE;
    }
    
};

enum class InputMode
{
    CAMERA,
    VIDEO
};

struct AutoConfig
{
    float infer_settle_time;
    float table_coarse_step;
    float table_fine_step;
    float table_move_settle_time;
    string save_path;
    bool no_infer;
    bool no_table;

    InputMode input_mode;
};

struct AppConfig
{
    ModelConfig model_config;
    CaptureConfig cap_config;
    DisplayConfig disp_config;
    VideoConfig video_config;
    TableConfig table_config;
    GuiConfig gui_config;
    AutoConfig auto_config;
};
