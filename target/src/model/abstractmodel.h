#pragma once

#include "modelresult.h"
#include <common/config.h>
#include "abstractsample.h"

#include <vector>

using namespace std;

/**
 * @brief The AbstractModel class abstract class for a model. Real and Fake 
 * models inherit from this
 */
class AbstractModel
{
public:
    AbstractModel(ModelConfig& config);
    
    virtual void passSample(AbstractSample* sample) = 0;
    virtual bool isResultReady() = 0;
    virtual vector<ModelResult> PopResults() = 0;
    virtual ModelConfig& getConfig() { return this->_config; }
    
protected:
    ModelConfig& _config;
};
