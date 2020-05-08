#!/usr/bin/env python3

import mobilenet_v2 as mv2
import config

def _main():

    cfg = None
    for c in config.GenConfigs():
        cfg = c
        break

    
    model = mv2.CreateModel(config=cfg)
    model_eval = mv2.CreateEvalModel(config=cfg)
    mv2.txWeights(model, model_eval)
    
    
    mv2.SaveFrozenGraphV1(model_eval, config=cfg['load/save'])

    
if __name__ == '__main__':
    _main()
