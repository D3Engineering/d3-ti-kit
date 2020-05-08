.. _index_ref:

#################
Table of Contents
#################

.. toctree::
    :maxdepth: 2

    sagemaker
    target
    host


AWS/TI Defect Detection Demo
============================
This documentation provides all necessary information and steps to reproduce the
defect detection demo developed by D3 Engineering for TI. This demo leverages
AWS Sagemaker and Sagemaker Neo for training and compiling of a MobilNetV2 based
defect detection model. AWS Green-grass is used to deploy the model on the
target.

Overview
========
The defect detection demo consists of model definition and training code, in the
sagemaker directory. TVM Compile scripts in the host directory. And, demo code in
target directory. To reproduce this example end-to-end access to Sagemaker would
be required. However a pre-trained, compiled model will be provided.

The physical setup consists of a Turn table (where test cameras are placed), a
usb webcam and HDMI display. All of these elements interface with a TI am67xx
evaluation board, which the demo runs on.


Installation
============
Create a bootable SD card for the target board following the instructions here:
:ref:`target_ref_sdcard`.

See :ref:`target_ref` for demo build and install
instructions

Model and Training
==================
The pre-trained weights are able to perform binary classification for
defect detection of D3 Camera modules. Given a 224x224x3 RGB image scaled
between 0 and 1 the network returns a prediction for pass fail, based on the
presence of a lens (the only defect currently detected).

To re-train the model, or make changes to the architecture see :ref:`sagemaker_ref_mods`

Demo Operation
==============
There are two options for running the demo.

1. Full Setup: demo software is run on the target. Using the GUI a model is
loaded onto the EVM hardware. The demo controls the turn table, bringing cameras
into view one-by-one, and providing a prediction of PASS or FAIL.

2. Headless: Only the evaluation board and a display are required. Using the
GUI, headless mode is selected, and the user chooses a video file to act as the
webcam stream. Embedded in this video file are time-steps, which tell the demo
when to perform inference.

For more detailed instructions see :ref:`target_ref_usage`

