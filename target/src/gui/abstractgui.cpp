
#include "abstractgui.h"

#include <common/config.h>
#include <common/error.h>
#include <opencv2/core/types.hpp>

#include <numeric>

using namespace std;

AbstractGui::AbstractGui(GuiConfig& config,
			 DisplayConfig& disp_config,
			 cv::Rect roi):
    _config(config),
    _disp_config(disp_config)
{
    this->_roi = roi;
    this->_disp_infer = false;
}

void AbstractGui::dispInfer()
{
    this->_disp_infer = true;
}

void AbstractGui::resetInfer()
{
    this->_disp_infer = false;
    this->_pred = "";
}

void AbstractGui::passResults(vector<ModelResult> results)
{
    // Compute the average probability of the batch
    ModelResult avg_result;
    string pred = "";
    float max_pred = 0.0;

    float pred_cutoff = this->_config.pred_cutoff;
    
    // DBG("Results: %i: ", results.size());
    for (auto const& r : results) {
        for (auto const& m : r.output) {
            // DBG("%s: %4.2f, ", m.first.c_str(), m.second);

            vector<float>& pred_vector = _preds[m.first];

            if (pred_vector.size() >= _config.pred_ring_size) {
                pred_vector.pop_back();
            }
            pred_vector.insert(pred_vector.begin(), m.second);
        }
    }

    for (auto const& m : _preds) {
        const vector<float>& pred_vector = m.second;
        float pred_sum = std::accumulate(pred_vector.begin(), pred_vector.end(), 0.0);
        
        if (pred_sum > max_pred) {
            // pred_sum >= (pred_cutoff * pred_vector.size())) {
            
            max_pred = pred_sum;
            pred = m.first;
        }
    }
    
    // DBG("Pred: %s", pred.c_str());

    this->_pred = pred;
}
