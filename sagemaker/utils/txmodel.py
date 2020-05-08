#!/usr/bin/env python3

import argparse
import pathlib
import code
import os
import pprint
import json

from tensorflow import keras
from tensorflow.keras import layers
import tensorflow.keras.backend as K
import tensorflow.compat.v1 as tfv1
import numpy as np

import mobilenet_v2 as mv2

def ReadJsonConfig(path):
    with open(path, 'r') as fp:
        config = json.loads(fp.read())
        return config


def SaveFrozenGraphV1(model, config=None):
  # see: https://leimao.github.io/blog/Save-Load-Inference-From-TF-Frozen-Graph/

  pb_dir = os.path.join(config['dir'])
  
  sess = K.get_session()
  graph = tfv1.get_default_graph()
  input_graph_def = graph.as_graph_def()

  tfv1.graph_util.remove_training_nodes(input_graph_def)

  # code.interact(local=dict(globals(), **locals()))
  graph_def = tfv1.graph_util.extract_sub_graph(input_graph_def, [model.input.op.name, model.output.op.name])

  output_node_names = [model.output.op.name]
  output_graph_def = tfv1.graph_util.convert_variables_to_constants(sess, graph_def, output_node_names)

  # code.interact(local=dict(globals(), **locals()))
  
  layers = [op.name for op in graph.get_operations()]

  pprint.pprint(layers[0])

  tfv1.io.write_graph(output_graph_def, pb_dir, "model.pb", as_text=False)

  # with tf.io.gfile.GFile(pb_filepath, 'wb') as f:
  #     f.write(output_graph_def.SerializeToString())


def txLayer(layer_from, layer_to):
    from tensorflow.keras.layers import Conv2D, DepthwiseConv2D
    
    conv_layers = [Conv2D, DepthwiseConv2D]
    
    if type(layer_from) != type(layer_to):
        raise ValueError("To transfer layers must be of the same type")

    # Try to simply copy weights
    try:
        layer_to.set_weights(layer_from.get_weights())
        return
    except:
        pass

    # If the layers are a type of convolution, handle missing bias in source
    if type(layer_from) in conv_layers:
        conv_to_w = layer_to.get_weights()
        conv_from_w = layer_from.get_weights()
        conv_to_w[0] = conv_from_w[0]

        layer_to.set_weights(conv_to_w)
        
    # if not, something else is wrong, raise an error.
    else:
        raise ValueError("Couldn't transfer weights")
    
    
def txWeights(model_from, model_to):
    tx_count = 0

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    for layer in model_from.layers:
        print("tx {}: ".format(layer.name), end='')

        layer_to = None
        try:
            layer_to = model_to.get_layer(layer.name)
        except:
            print("fail - can't get layer")
            continue

        txLayer(layer, layer_to)
        tx_count += 1
        print("success")

    print("transfered {} layers".format(tx_count))


def _get_conv2d_folded_weights(bn_layer, conv_layer):
    gamma, beta, mean, var = bn_layer.get_weights()
    conv_w, conv_b = conv_layer.get_weights()

    gamma = gamma.reshape((1,1,1,-1))
    var = var.reshape((1,1,1,-1))

    new_weights = (
        conv_w * gamma / np.sqrt(var + 1e-3),
        beta + (conv_b - mean) * gamma.reshape(-1) / np.sqrt(var.reshape(-1) + 1e-3)
    )

    return new_weights


def _get_depth_conv2d_folded_weights(bn_layer, depth_layer):
    gamma, beta, mean, var = bn_layer.get_weights()
    conv_w, conv_b = depth_layer.get_weights()

    gamma = gamma.reshape((1,1,-1,1))
    var = var.reshape((1,1,-1,1))

    new_weights = (
        conv_w * gamma / np.sqrt(var + 1e-3),
        beta + (conv_b - mean) * gamma.reshape(-1) / np.sqrt(var.reshape(-1) + 1e-3)
    )

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    return new_weights
    
    
def foldBNAndTxWeights(model, model_to):
    from tensorflow.keras.layers import BatchNormalization, Conv2D, DepthwiseConv2D
    
    last_layer=None

    txWeights(model, model_to)

    for layer in model.layers:
        # Fold BN into Conv2D
        if type(layer) == BatchNormalization:
            conv_layer = model_to.get_layer(last_layer.name)
            new_weights = None

            if type(last_layer) == Conv2D:
                new_weights = _get_conv2d_folded_weights(layer, conv_layer)
                print("CONV -> ", end='')
                
            elif type(last_layer) == DepthwiseConv2D:
                new_weights = _get_depth_conv2d_folded_weights(layer, conv_layer)
                print("DW -> ", end='')
                
            else:
                continue

            print("BN")

            # code.interact(local=dict(globals(), **locals()))
            # exit(1)

            conv_layer.set_weights(new_weights)
            
        last_layer = layer


def processModel(model, cfg, make_eval=True, batch_size=4):
    if make_eval:
        model_eval = mv2.CreateEvalModel(config=cfg)
        # txWeights(model, model_eval)
        foldBNAndTxWeights(model, model_eval)

        model = model_eval

        
    if batch_size != -1:
        # code.interact(local=dict(globals(), **locals()))
        # exit(1)

        input_shape = [d.value for d in model.input.shape[1:4]]
        
        batch_input = layers.Input(shape=input_shape, batch_size=batch_size, name='input_eval')
        batch_output = model(batch_input)
        model = keras.Model(batch_input, batch_output)

    return model
    
        
def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('model', metavar='model', type=str)
    parser.add_argument('--batch-size', type=int, default=-1)
    parser.add_argument('--make-eval', action='store_true')
    parser.add_argument('--freeze-graph', action='store_true')

    args = parser.parse_args()

    return args


def _main():
    args = _parse_args()
    
    model = keras.models.load_model(args.model)
    path = pathlib.Path(args.model)

    cfg = ReadJsonConfig(path.parent/"config.json")
    cfg['load/save']['dir'] = path.parent
    cfg['model']['nchw'] = False

    model = processModel(model, cfg, make_eval=args.make_eval, batch_size=args.batch_size)

    if args.freeze_graph:
        print("saving model to {}".format(cfg['load/save']['dir']))
        SaveFrozenGraphV1(model, config=cfg['load/save'])
    
    model_path = pathlib.Path(cfg['load/save']['dir'])
    model.save(model_path/'model.h5')

    # code.interact(local=dict(globals(), **locals()))
    # exit(1)

    print("input: {}, output: {}".format(model.input.name, model.output.name))

    
if __name__ == '__main__':
    _main()
