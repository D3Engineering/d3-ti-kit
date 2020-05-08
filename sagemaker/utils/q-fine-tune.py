#!/usr/bin/env python3

import pathlib
import code
import os
import pprint
import json
import argparse

from tensorflow import keras
from tensorflow.keras import layers
import tensorflow.keras.backend as K
import tensorflow.compat.v1 as tfv1
import tensorflow_model_optimization as tfmot
import tensorflow as tf

import numpy as np

import data

def ReadJsonConfig(path):
    with open(path, 'r') as fp:
        config = json.loads(fp.read())
        return config


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('model', metavar='model', type=str)

    args = parser.parse_args()

    return args


def _main():
    args = _parse_args()
    
    model_path = pathlib.Path(args.model)
    model = keras.models.load_model(model_path)
    path = model_path.parent

    cfg = ReadJsonConfig(path/"config.json")
    cfg['training']['batch-size'] = 16
    
    trainGen, valData, testData, info = data.GetData(cfg)
    steps_per_epoch = info['train-datagen-len'] // cfg['training']['batch-size']

    quantize_model = tfmot.quantization.keras.quantize_model

    q_aware_model = quantize_model(model)

    q_aware_model.compile(optimizer='adam',
                          loss='categorical_crossentropy',
                          metrics=['acc'])
    q_aware_model.summary()

    q_aware_model.fit(
        trainGen,
        steps_per_epoch=steps_per_epoch,
        epochs = cfg['training']['epochs'] // 4,
        validation_data=valData)

    q_aware_model.save(path/model_path.stem + ".qtune.h5")

    code.interact(local=dict(globals(), **locals()))
    exit(1)


if __name__ == '__main__':
    _main()
