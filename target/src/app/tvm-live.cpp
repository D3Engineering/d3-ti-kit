
#include <stdlib.h>
#include <stdio.h>

#include <app/tvm_app.h>
#include <common/config.h>
#include <capdisplay/ticapturedisplay.h>
#include <capdisplay/gstcapturedisplay.h>

#include "cxxopts.hpp"

using namespace std;

struct ProgramArgs
{
    InputMode in_mode;
    string model_path;
    string video_path;
    string img_path;
    bool no_infer;
    bool no_table;
};

struct ProgramArgs parse_args(int argc, char** argv)
{
    struct ProgramArgs prog_args;

    prog_args.no_infer = false;
    prog_args.no_table = false;
    
    cxxopts::Options options("tvm-live", "Defect Detection Demo");
    options.add_options()
        ("s,source", "Image Data Source (Required)", cxxopts::value<string>())
        ("f,file", "Video File (Required if source=video)", cxxopts::value<string>())
        ("h,help", "Print Help") // boolean
        ("m,model", "Compiled Model Path (Required)", cxxopts::value<string>())
        ("v,noinfer", "No inference", cxxopts::value<bool>()->default_value("false"))
        ("n,notable", "Don't move the table", cxxopts::value<bool>()->default_value("false"))
        ("i,imgcap", "Run normally, but write images to disk", cxxopts::value<string>());

    auto args = options.parse(argc, argv);

    if (args.count("help"))
    {
        cout << options.help() << endl;
        exit(0);
    }
                   
    if (args.count("source"))
    {
        prog_args.in_mode = InputMode::CAMERA;
        if (args["source"].as<string>() == "video")
        {
            prog_args.in_mode = InputMode::VIDEO;
        }
    }
    else
    {
        cout << "Missing s[ource]" << endl << endl;
        cout << options.help() << endl;
        exit(1);
    }

    if (args.count("model"))
    {
        prog_args.model_path = args["model"].as<string>();
    }
    else
    {
        ERROR("A model must be specified");
        exit(1);
    }

    if (args.count("file"))
    {
        prog_args.video_path = args["file"].as<string>();
    }
    else if (prog_args.in_mode == InputMode::VIDEO)
    {
        ERROR("if source=video then file must be specified");
        exit(1);
    }

    if (args.count("imgcap"))
    {
        prog_args.img_path = args["imgcap"].as<string>();
    }
    else
    {
        prog_args.img_path = "";
    }

    if (args.count("noinfer"))
    {
        prog_args.no_infer = args["noinfer"].as<bool>();
    }

    if (args.count("notable"))
    {
        prog_args.no_table = args["notable"].as<bool>();
    }

    return prog_args;
}
    
int main(int argc, char* argv[])
{
    AppConfig config;

    struct ProgramArgs args = parse_args(argc, argv);

    config.auto_config.input_mode = args.in_mode;


    config.disp_config.disp_width = 320;
    config.disp_config.disp_height = 240;
        
    if (config.auto_config.input_mode == InputMode::CAMERA)
    {
        // Capture config setup
        config.cap_config.device = "/dev/video1";
        config.cap_config.capture_width = 640;
        config.cap_config.capture_height = 480;

        // Center crop
        config.cap_config.crop_x = (config.cap_config.capture_width -
                                    config.disp_config.disp_width) / 2;
        
        config.cap_config.crop_y = (config.cap_config.capture_height -
                                    config.disp_config.disp_height) / 2;    
    
        config.cap_config.zoom = 300;
        config.cap_config.focus = 30;
    }
    else if (config.auto_config.input_mode == InputMode::VIDEO)
    {
        config.video_config.video_path = args.video_path;
        DBG("got video path: %s", config.video_config.video_path.c_str());
    }

    // Model config setup
    config.model_config.name = "test";
    config.model_config.path = args.model_path;
    config.model_config.labelMap.push_back("Pass");
    config.model_config.labelMap.push_back("Fail");
    config.model_config.batch_size = 4;
    config.model_config.infer_width = 224;
    config.model_config.infer_height = 224;
    config.model_config.crop_x = 10;
    config.model_config.crop_y = 10;
    config.model_config.num_ch = 3;
    config.model_config._input_name = "input_eval";
    config.model_config.classes = 2;

    // GUI Config
    config.gui_config.useDefaults();
    config.gui_config.font_size = 12.0;
    config.gui_config.pred_ring_size = 1;
    config.gui_config.pred_cutoff = 0.7;

    // Table config
    config.table_config.ticks_per_rotation = 230408.0;
    config.table_config.velocity = 40000;
    config.table_config.sections = 5;
    config.table_config.acceleration = 80000 * 0.75;
    config.table_config.current_limit = 2.5;

    // Autonomous config
    config.auto_config.infer_settle_time = 1.5;
    config.auto_config.table_coarse_step = 1.0;
    config.auto_config.table_fine_step = 0.2;
    config.auto_config.save_path = args.img_path;
    config.auto_config.no_infer = args.no_infer;
    config.auto_config.no_table = args.no_table;

    config.auto_config.table_move_settle_time =
        config.table_config.ticks_per_rotation /
        config.table_config.sections /
        config.table_config.velocity;
        

    TvmApp app(config);

    app.start();

    while (true) {
        if(!app.loop())
        {
            exit(1);
        }
    }

    return 0;
}
