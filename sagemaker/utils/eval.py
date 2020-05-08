#!/usr/bin/env python3

import sys
import argparse

import tensorflow as tf
from tensorflow import keras
from tensorflow.keras.utils import to_categorical
import numpy as np
import pathlib
import code

import data


def _eval(model, X_eval, y_eval, prefix="training", cutoff=0.0):
    y_pred = model.predict(X_eval)
    y_pred_max = np.max(y_pred, axis=-1)

    y_eval_dec = np.argmax(y_eval, axis=-1)
    y_pred_dec = np.argmax(y_pred, axis=-1)

    acc = y_eval_dec == y_pred_dec
    acc = np.logical_and(acc, y_pred_max > cutoff)
    acc = np.mean(acc)
    
    print("{} accuracy: {}".format(prefix, acc))

          
def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('model', metavar='model', type=str)

    return parser.parse_args()


def _main():
    args  = _parse_args()
    model_path = pathlib.Path(args.model)
    cwd = pathlib.Path(".")
    data_path = model_path.parent/"data"

    X_train = np.load(data_path/"X_train.npy")
    X_valid = np.load(data_path/"X_valid.npy")
    X_test = np.load(data_path/"X_test.npy")
    y_train = np.load(data_path/"y_train.npy")
    y_valid = np.load(data_path/"y_valid.npy")
    y_test = np.load(data_path/"y_test.npy")
    
    mm = keras.models.load_model(model_path)

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    _eval(mm, X_train, y_train, prefix="training")
    _eval(mm, X_valid, y_valid, prefix="validation")
    _eval(mm, X_test, y_test, prefix="testing")

    _eval(mm, X_test, y_test, cutoff=0.9, prefix="testing 0.7 cutoff")


if __name__ == '__main__':
    _main()
