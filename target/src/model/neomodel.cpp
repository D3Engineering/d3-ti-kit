#include <model/neomodel.h>
#include <common/config.h>
#include <model/modelresult.h>
#include <common/error.h>
#include <dlr.h>

#include <stdio.h>


NeoModel::NeoModel(ModelConfig& config) :
AbstractModel(config)
{
    // NCHW
    this->_input_shape[0] = config.batch_size;
    this->_input_shape[2] = config.infer_height;
    this->_input_shape[3] = config.infer_width;
    this->_input_shape[1] = config.num_ch;
    this->_input_name = config._input_name;

    this->_output_shape[0] = config.batch_size;
    this->_output_shape[1] = config.classes;

    MSG("loading model from: %s... ", config.path.c_str());

    // 0 -> cpu
    int dev_type = 1; // not sure
    int device = 0; // cpu
    int err = CreateDLRModel(&this->_model, config.path.c_str(), dev_type, device);

    if (err) {
        ERROR("Failed\n");
        exit(1);
    }
    else {
        MSG("Success\n");
    }

    this->_neo_results = new float[this->_output_shape[0]*this->_output_shape[1]];
}


void NeoModel::passSample(AbstractSample* sample)
{
    this->_sample = sample;
    this->infer();
}


bool NeoModel::isResultReady()
{
    return this->_results_ready;
}

vector<ModelResult> NeoModel::PopResults()
{
    if (this->_results_ready) {
        this->_results_ready = false;
        return this->_results;
    }
    else
    {
        vector<ModelResult> empty;
        return empty;
    }
}


void NeoModel::infer()
{
    float* frame;
    int err;
    int dim = 4;  // this is 4 because NCHW equates to 4 dimensions

    // DBG("setting %i samples", (int)this->_input_shape[0]);

    frame = this->_sample->getSample();
    
    // DBG("Frame: %p, input: (%i,%i,%i,%i)",
    //        frame,
    //        (int)this->_input_shape[0],
    //        (int)this->_input_shape[1],
    //        (int)this->_input_shape[2],
    //        (int)this->_input_shape[3]);

    
    // Set model input
    err = SetDLRInput(&this->_model,
                      this->_input_name.c_str(),
                      this->_input_shape, 
                      frame, dim);
    if (err) {
        ERROR("Error: Failed to set input\n");
        return;
    }

    // Run inference
    err = RunDLRModel(&this->_model);
    if (err) {
        ERROR("ERROR: failed to run model\n");
        return;
    }

    this->_results.clear();
    
    ModelResult result;

    err = GetDLROutput(&this->_model, 0, this->_neo_results);
    if (err) {
        ERROR("ERROR: failed to get model output");
    }

    // For each batch
    for(int i = 0; i < this->_output_shape[0]; i++)
    {
        int base_idx = i * this->_output_shape[1];
        // For each class
        for(int j = 0; j < this->_output_shape[1]; j++) {
            result.output[this->_config.labelMap[j]] = this->_neo_results[base_idx + j];
        }
        this->_results.push_back(result);
    }

    this->_results_ready = true;
}
