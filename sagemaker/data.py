import numpy as np
import os
from tensorflow.keras.utils import to_categorical
from tensorflow.keras.preprocessing.image import ImageDataGenerator
import code

# from albumentations import (Flip, GaussNoise, )

def _prepare_data(X, y, category=False):

    # Zero mean
    # X = X - np.mean(X)

    # Contrast Norm
    x_min = np.min(X, axis=(1,2)).reshape(X.shape[0], 1, 1, X.shape[-1])
    x_max = np.max(X, axis=(1,2)).reshape(X.shape[0], 1, 1, X.shape[-1])

    X = (X - x_min) / (x_max - x_min)

    if category:
        y = to_categorical(y)

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    return X, y


def _get_random_augment(X_batch, y_batch):
    pass


def _data_gen(X, y, batch_size=None, augment=False):
    while True:
        batch_idx = np.random.permutation(X.shape[0])[0:batch_size]

        yield(X[batch_idx], y[batch_idx])


def KerasAugmentGen(X_train, methods=None):
    datagen = ImageDataGenerator(
        horizontal_flip=True,
        vertical_flip=True,
        width_shift_range=0.2,
        height_shift_range=0.2,
        brightness_range=(0.8, 1.2),
        zoom_range=0.2,
        channel_shift_range=0.2)

    datagen.fit(X_train)

    return datagen


def GetData(config):
    data_config = config['data']
    batch_size = config['training']['batch-size']
    path = data_config['path']
    out_dir = os.path.join(config['load/save']['dir'], 'data')

    X = np.load(os.path.join(path, 'data.npy'))
    y = np.load(os.path.join(path, 'labels.npy'))
    N = X.shape[0]

    X, y = _prepare_data(X, y, category=True)

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

    train_gen = _data_gen(X_train, y_train, batch_size=batch_size)
    if data_config['augment']['enabled']:
        train_gen = KerasAugmentGen(X_train).flow(X_train, y_train)


    return train_gen, \
        (X_valid, y_valid), \
        (X_test, y_test), \
        info

