#!/usr/bin/env python3

import numpy as np
import os
import code
import matplotlib.pyplot as plt
import argparse
import pathlib
import imageio


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--view-only', action="store_true")

    return parser.parse_args()


def viewData():
    X, y = (np.load('data.npy'), np.load('labels.npy'))
    idx = np.random.permutation(X.shape[0])[0:100]

    imgs = X[idx]
    labels = y[idx]

    label_strs = ['pass' if l==0 else 'fail' for l in labels]

    plt.figure(figsize=(20,20))
    for i in range(imgs.shape[0]):
        plt.subplot(10,10,i+1)
        plt.xticks([])
        plt.yticks([])
        plt.title(label_strs[i])
        plt.imshow(imgs[i])

    plt.tight_layout()
    plt.savefig("sample.png")


def loadImg(path):
    if path.suffix in ['.png','.jpg']:
        return imageio.imread(path)
    elif path.suffix == '.npy':
        return np.load(path)


def loadDir(path):
    img_list = []
    for root, dirs, files in os.walk(path):
        for f in files:
            root = pathlib.Path(root)
            img = loadImg(root/f)
            if not img is None:
                img_list.append(img)

    try:
        return np.array(img_list)
    except:
        code.interact(local=dict(globals(), **locals()))
        exit(1)


def packData():
    dir_class_map = {
        'lens_screw': {'label': 'pass', 'crop': (10,10,224,224)},
        'nolens_screw': {'label': 'fail', 'crop': (10,10,224,224)},
        'cpp_demo_fail': {'label': 'fail'},
        'cpp_demo_pass': {'label': 'pass'},
        'demo_vid_pass': {'label': 'pass', 'crop': (10,10,224,224)},
        # 'cpp_pass_test': {'label': 'pass', 'prefix': 'cpp_pass_test'},
        # 'cpp_fail_test': {'label': 'fail', 'prefix': 'cpp_fail_test'}
    }

    class_label_map = {'pass': 0, 'fail': 1}

    data = None
    labels = None

    for dir in dir_class_map:
        path = pathlib.Path(dir)
        class_str = dir_class_map[dir]['label']
        imgs = loadDir(path)

        if 'crop' in dir_class_map[dir]:
            x,y,w,h  = dir_class_map[dir]['crop']
            imgs = imgs[:,y:y+h,x:x+w,:]

        if imgs.shape[-1] == 4:
            imgs = imgs[:,:,:,0:3]

        # Get or generate the correct label
        label = np.ones((imgs.shape[0])) * class_label_map[class_str]

        # save off if necessary
        if 'prefix' in dir_class_map[dir]:
            data_path = "data_{}.npy".format(dir_class_map[dir]['prefix'])
            label_path = "label_{}.npy".format(dir_class_map[dir]['prefix'])
            np.save(data_path, imgs)
            np.save(label_path, label)
        
        # Append data and labels from directory
        if data is None:
            data = imgs
            labels = label
        else:
            # code.interact(local=dict(globals(), **locals()))
            data = np.concatenate((data, imgs), axis=0)
            labels = np.concatenate((labels, label), axis=0)


    np.save('data.npy', data)
    np.save('labels.npy', labels)


def main():
    args = _parse_args()

    if args.view_only:
        viewData()
    else:
        packData()


if __name__ == '__main__':
    main()

