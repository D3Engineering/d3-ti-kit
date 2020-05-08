#!/usr/bin/env python3

import reqs

import argparse
import os
import data
import code
import json
import sys

import pickle
import matplotlib.pyplot as plt

import config
import mobilenet_v2 as mv2

import numpy as np

def SaveHist(history, config=None):
    config_ls = config['load/save']

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    with open(os.path.join(config_ls['dir'], 'history.pkl'), 'wb') as fp:
        pickle.dump(history.history, fp)


def VizHistory(hist, plot_keys, config=None, title=""):
    plt.figure()
    plt.title(title)
    for k in plot_keys:
        if k in hist.history:
            plt.plot(hist.history[k])
        else:
            print("Warning: {} not in history".format(k))
    plt.legend(plot_keys)

    plt.savefig(os.path.join(config['load/save']['dir'], 'training.png'))
    

def TrainAndEval(cfg):
    config.SaveConfig(cfg)

    trainGen, valData, testData, info = data.GetData(cfg)
    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    # build model
    model = mv2.CreateModel(config=cfg)
    mv2.CompileModel(model, config=cfg)
    callbacks = mv2.CreateCallbacks(config=cfg)

    steps_per_epoch = info['train-datagen-len'] // cfg['training']['batch-size']

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    history = model.fit_generator(
        trainGen,
        steps_per_epoch=steps_per_epoch,
        epochs=cfg['training']['epochs'],
        callbacks=callbacks,
        validation_data=valData)
    
    # Visualize training
    VizHistory(history, ['loss', 'val_loss'], config=cfg, title="Training History")
    
    metrics = model.evaluate(
        testData[0], testData[1], 
        cfg['training']['batch-size'])

    print("{}: {}, {}: {}".format(model.metrics_names[0], metrics[0], model.metrics_names[1], metrics[1]))

    
    # Save model and history
    if cfg['local']:
        mv2.SaveModel(model, config=cfg)
        SaveHist(history, config=cfg)
        mv2.SaveFrozenGraphV1(model, config=cfg['load/save'])
    else:
        import txmodel
        model = txmodel.processModel(model, cfg, make_eval=True, batch_size=4)
        mv2.SaveFrozenGraphV1(model, config=cfg['load/save'])

    
def _parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('--epochs', type=int, default=None)
    parser.add_argument('--batch_size', type=int, default=None)
    parser.add_argument('--learning_rate', type=float, default=None)

    # input data and model directories
    parser.add_argument('--local', action='store_true')
    parser.add_argument('--model_dir', type=str)
    parser.add_argument('--sm-model-dir', type=str, default=os.environ.get('SM_MODEL_DIR'))
    parser.add_argument('--train', type=str, default=os.environ.get('SM_CHANNEL_TRAINING'))


    return parser.parse_args()



def _apply_args(args, cfg):
    if not args.local:
        cfg['load/save']['dir'] = args.sm_model_dir
        cfg['data']['path'] = args.train
    else:
        cfg['local'] = True


def _main():
    args = _parse_args()
    for c in config.GenConfigs():
        _apply_args(args, c)
        TrainAndEval(c)   


if __name__ == '__main__':
    _main()
