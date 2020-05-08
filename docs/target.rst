.. _target_ref:

######
Target
######

Overview
========
The target code consists of the demo, written in C++ which provides the
following.

* Control over the turn table
* On screen statistics
* Model switching/loading
* Interfacing with a v4l2 video device
* A GUI which exposes all above features

.. _target_ref_sdcard:

SD Card Setup
=============
See `here <http://software-dl.ti.com/processor-sdk-linux/esd/docs/06_01_00_08/linux/Overview_Getting_Started_Guide.html#linux-sd-card-creation-guide>`_ 
for tips

* 1st partition: 100MiB, FAT32, label="boot", boot flag
* 2nd partition: 8+GiB, ext3 or ext4, label="rootfs"

Deploy filesystem to rootfs:

.. code-block:: bash

    sudo tar pxf ${TIDL_PLSDK}/filesystem/tisdk-rootfs-image-am57xx-evm.tar.xz -C /path/to/mounted/rootfs/


Deploy pre-builts to boot:

.. code-block:: bash

    cp ${TIDL_PLSDK}/board-support/prebuilt-images/MLO-am57xx-evm /path/to/mounted/boot/MLO
    cp board-support/prebuilt-images/uEnv.txt /path/to/mounted/boot/uEnv.txt
    cp board-support/prebuilt-images/u-boot-am57xx-evm.img /path/to/mounted/boot/u-boot.img

Architecture
============
TODO: Figure out if this is needed. It would go over how demo code is split up
among files and classes.

Demo software consists of a GUI based on QT5, as well as turn table control
software. Display, capturing, and inference all leverage TI accelerated hardware

.. _build_install:

Building for Target
===================

Environment Setup
-----------------
To cross-compile for the am57xx the TI processor SDK must be installed.
target/Makefile expects it to be extracted to
target/ti-processor-sdk-linux-am57xx-evm-06.02.00.81. This can be its actual
install location, or a symlink

To add all the cross-compile items (libs, compilers, etc) source env.sh


Cross-Compiling libphiget (target only)
---------------------------------------
Since a phiget controller is used to control the turn table, libphiget is a
dependency regardless of if you are planning to use headless mode. It is
necessary to cross-compile libphiget, and install it in the sdk rootfs.

The libphiget source can be found
`here <https://www.phidgets.com/docs/Phidgets_Drivers>`_. TODO: figure out if the
specific version is important or if any libphidget22 will suffice.

.. code-block:: bash

    cd libphidget22-1.6.20200306

    ./configure --host=arm-linux-gnueabihf \
    --prefix=<path-to>/ti-processor-sdk-linux-am57xx-evm-06.02.00.81/linux-devkit/sysroots/armv7at2hf-neon-linux-gnueabi/usr/

    # sudo is required due to sdk folder permissions
    make -j4
    sudo "PATH=$PATH" make install


Building
--------
A makefile is provided to build the demo

.. code-block:: bash

    . env.sh  # if you haven't already
    make -j4

All libraries are linked statically, so the only file that needs to be copied
over to the target is the tvm-live binary.

.. code-block:: bash

    scp ./build-embedded/tvm-live <target-ip-or-hostname>:


.. _target_ref_usage:

Usage
=====

Configuration
-------------


GUI
---
