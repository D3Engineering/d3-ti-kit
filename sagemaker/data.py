import numpy as np
import os
from tensorflow.keras.utils import to_categorical
from tensorflow.keras.preprocessing.image import ImageDataGenerator
import code
import pathlib

from albumentations import (
    HorizontalFlip,
    VerticalFlip,
    GaussNoise,
    MultiplicativeNoise, 
    RandomBrightness,
    RandomContrast, 
    Rotate,
    ShiftScaleRotate,
    ChannelShuffle   
)

def _prepare_data(X, y, category=False):

    # Zero mean
    # X = X - np.mean(X)

    # Contrast Norm
    # x_min = np.min(X, axis=(1,2)).reshape(X.shape[0], 1, 1, X.shape[-1])
    # x_max = np.max(X, axis=(1,2)).reshape(X.shape[0], 1, 1, X.shape[-1])

    # X = (X - x_min) / (x_max - x_min)
    # X = X * 255

    X = X / 255.0

    if category:
        y = to_categorical(y)

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    return X, y


def _get_random_augment(X_batch, augment_ops):
    # code.interact(local=dict(globals(), **locals()))
    # exit(1)
    for i in range(X_batch.shape[0]):
        for aug in augment_ops:
            X_batch[i] = aug(image=X_batch[i])['image']

    return X_batch


def _get_augment_ops():
    augment_list = [
        HorizontalFlip,
        VerticalFlip,
        # GaussNoise,
        # MultiplicativeNoise, 
        RandomBrightness,
        RandomContrast, 
        # Rotate,
        ShiftScaleRotate]
    
    init_list = [aug(p=1.0/len(augment_list)/2) for aug in augment_list]

    return init_list


def _data_gen(X, y, batch_size=None, augment=False, nchw=False):
    augments = None
    if augment:
        augments = _get_augment_ops()
    
    while True:
        batch_idx = np.random.permutation(X.shape[0])[0:batch_size]
        X_batch = X[batch_idx]
        y_batch = y[batch_idx]

        if augment:
            X_batch = _get_random_augment(X_batch, augments)

        if nchw:
            X_batch = X_batch.transpose(0,3,1,2)
            
        yield(X_batch, y_batch)


def GetDataNew(config):
    data_config = config['data']
    model_config = config['model']
    batch_size = config['training']['batch-size']
    path = data_config['path']
    out_dir = pathlib.Path(config['load/save']['dir'])/"data"

    X = np.load(os.path.join(path, 'data.npy')).astype(np.float32)
    y = np.load(os.path.join(path, 'labels.npy')).astype(np.float32)

    if 'class' in data_config:
        # code.interact(local=dict(globals(), **locals()))
        X = X[np.where(y==data_config['class'])]
        y = y[np.where(y==data_config['class'])]
        print("grabbed {} images from class {}".format(X.shape[0], data_config['class']))

    X, y = _prepare_data(X, y, category=True)

    N = X.shape[0]
    train_len = int(N * data_config['train'])
    valid_len = int(N * data_config['valid'])
    test_len = int(N * data_config['test'])

    os.mkdir(out_dir)

    idx = np.random.permutation(N)
    X_train = X[idx[0:train_len]]
    y_train = y[idx[0:train_len]]
    np.save(os.path.join(out_dir, 'X_train.npy'), X_train)
    np.save(os.path.join(out_dir, 'y_train.npy'), y_train)

    X_valid = X[idx[train_len:train_len+valid_len]]
    y_valid = y[idx[train_len:train_len+valid_len]]
    np.save(os.path.join(out_dir, 'X_valid.npy'), X_valid)
    np.save(os.path.join(out_dir, 'y_valid.npy'), y_valid)

    X_test = X[idx[train_len+valid_len:train_len+valid_len+test_len]]
    y_test = y[idx[train_len+valid_len:train_len+valid_len+test_len]]
    np.save(os.path.join(out_dir, 'X_test.npy'), X_test)
    np.save(os.path.join(out_dir, 'y_test.npy'), y_test)

    info = {'train-orig-len': train_len, 'train-datagen-len': train_len}

    gan_pass = None
    gan_fail = None
    if 'gan-augment' in data_config and data_config['gan-augment']['enabled']:
        gan_pass = models.load_model(data_config['gan-augment']['gan-pass-path'])
        gan_fail = models.load_model(data_config['gan-augment']['gan-fail-path'])


    train_gen = _data_gen(X_train, y_train, 
                          batch_size=batch_size, 
                          augment=data_config['augment']['enabled'],
                          nchw=model_config['nchw'])

    if model_config['nchw']:
        X_test = X_test.transpose(0,3,1,2)
        X_valid = X_valid.transpose(0,3,1,2)
                          

    return train_gen, \
        (X_valid, y_valid), \
        (X_test, y_test), \
        info


def GetDataLoad(config):
    data_config = config['data']
    model_config = config['model']
    batch_size = config['training']['batch-size']
    data_dir = pathlib.Path(config['load/save']['dir'])/"data"

    X_train = np.load(data_dir/"X_train.npy")
    y_train = np.load(data_dir/"y_train.npy")
    train_len = X_train.shape[0]

    X_test = np.load(data_dir/"X_test.npy")
    y_test = np.load(data_dir/"y_test.npy")

    X_valid = np.load(data_dir/"X_valid.npy")
    y_valid = np.load(data_dir/"y_valid.npy")

    info = {'train-orig-len': train_len, 'train-datagen-len': train_len}

    train_gen = _data_gen(X_train, y_train, 
                          batch_size=batch_size, 
                          augment=data_config['augment']['enabled'],
                          nchw=model_config['nchw'])

    if model_config['nchw']:
        X_test = X_test.transpose(0,3,1,2)
        X_valid = X_valid.transpose(0,3,1,2)

        
    return train_gen, \
        (X_valid, y_valid), \
        (X_test, y_test), \
        info
    
    
def GetData(config):
    out_dir = pathlib.Path(config['load/save']['dir'])/"data"

    if out_dir.exists():
        return GetDataLoad(config)
    else:
        return GetDataNew(config)

                          


