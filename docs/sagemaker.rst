.. _sagemaker_ref:

#########
Sagemaker
#########

Overview
========

tvm_demo/sagemaker folder contains the following files

* config.py - Configure the training, including hyperparameters and training data location
* data.py - Code to load, pre-process and augment data
* mobilenet.py - Implements liner bottleneck block along with other helpers
* mobilenet_v2.py - Interface to create and export MobilenetV2 model
* train.py - entry point to training

Training Locally
================

Environment
-----------
Create a virtual environment with the provided requirements.txt. This will
install tensorflow 1.14.0, along with other dependencies. If you require GPU
support please install tensorflow-gpu==1.14.0, along with cuda. See instructions
for GPU support `here <https://www.tensorflow.org/install/gpu>`_

Data
----
The default configuration expects two files for training.

* data/data.npy - should contain N images in NWHC format
* data/labels.npy - should contain N labels. Each element on [0,1]

.. _sagemaker_ref_mods:

Modifications
=============
TODO: Details on how to modify hyperparameters and network architecture
