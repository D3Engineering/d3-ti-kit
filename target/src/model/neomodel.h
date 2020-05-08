#pragma once

#include "abstractmodel.h"
#include <common/config.h>
#include "modelresult.h"

#include <string>
#include <dlr.h>

using namespace std;

class NeoModel : public AbstractModel
{
public:
    explicit NeoModel(ModelConfig& config);
    ~NeoModel();

    virtual void passSample(AbstractSample* sample);
    virtual bool isResultReady();
    virtual vector<ModelResult> PopResults();

protected:

    void infer();

    int64_t _input_shape[4];
    int64_t _output_shape[2];
    
    string _input_name;
    DLRModelHandle _model;

    bool _results_ready;

    vector<float> model_output;
    vector<ModelResult> _results;
    AbstractSample* _sample;
    float* _neo_results;
};
