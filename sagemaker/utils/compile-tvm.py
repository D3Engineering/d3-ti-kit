#!/usr/bin/env python3

import numpy
import tensorflow as tf
import tvm
from tvm import te
from tvm import relay
from tvm import contrib
import tvm.relay.testing.tf as tf_testing

import argparse
import os
from pathlib import Path

import code


def _parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('path', metavar='path', type=str)
    parser.add_argument('input', metavar='input', type=str, default='input_eval')
    parser.add_argument('output', metavar='output', type=str, default='model/softmax/Softmax')

    args = parser.parse_args()

    return args


def importTFModel(model_path, output_name):
    with tf.compat.v1.gfile.GFile(model_path, 'rb') as f:
        graph_def = tf.compat.v1.GraphDef()
        graph_def.ParseFromString(f.read())
        graph = tf.import_graph_def(graph_def, name='')
        
        # Call the utility to import the graph definition into default graph.
        graph_def = tf_testing.ProcessGraphDefParam(graph_def)
        
        # Add shapes to the graph.
        with tf.compat.v1.Session() as sess:
            graph_def = tf_testing.AddShapesToGraphDef(sess, output_name)

            return graph_def

        
def relayImport(graph_def, layout, input_name, input_shape):
    shape_dict = {input_name: input_shape}
    dtype_dict = {input_name: 'float32'}
    mod, params = relay.frontend.from_tensorflow(graph_def,
                                                 layout=layout,
                                                 shape=shape_dict)
    return mod, params


def relayBuild(mod, params, target):
    with relay.build_config(opt_level=3):
        graph, lib, params = relay.build(mod,
                                         target=target,
                                         params=params)
        return graph, lib, params
    

def saveBuild(path, graph, lib, params):
    cc = "../target/ti-processor-sdk/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/arm-linux-gnueabihf-g++"
    
    lib.export_library(path/"compiled.so", contrib.cc.create_shared, cc=cc)
    
    with open(path/"compiled_model.json", "w") as fo:
        fo.write(graph)
    with open(path/"compiled.params", "wb") as fo:
        fo.write(relay.save_param_dict(params))
        
    
def _main():
    args = _parse_args()

    if not os.path.exists(args.path):
        raise ValueError("path must exist")

    target = tvm.target.create('llvm -device=arm_cpu -target=armv7l-linux-gnueabihf -mattr=+neon')
    layout = 'NCHW'

    # For Mobilenet Pretrained
    # input_name = 'input'
    # output_name = 'MobilenetV2/Predictions/Reshape_1'
    # input_shape = (1, 224, 224, 3)

    # For My Mobilenet
    input_name = args.input
    output_name = args.output
    input_shape = (4,224,224,3)

    # For pre-trained inception model
    # input_name = 'DecodeJpeg/contents'
    # output_name = 'softmax'
    # input_shape = (299,299,3)

    graph_def = importTFModel(args.path, output_name)

    mod, params = relayImport(graph_def, layout, input_name, input_shape)
    graph, lib, params = relayBuild(mod, params, target)

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)
    
    build_dir = Path(args.path).parent / "compiled_model"
    if not build_dir.exists():
        build_dir.mkdir()

    saveBuild(build_dir, graph, lib, params)

    
if __name__ == '__main__':
    _main()
